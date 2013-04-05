// 2010 by Fabrizio Di Vittorio (fdivitto@tiscali.it)


#ifndef FDV_WIRELESSRPC_H
#define FDV_WIRELESSRPC_H

#include <stdlib.h>
#include <inttypes.h>

#include <avr/eeprom.h>



#include "../fdv_generic/fdv_memory.h"
#include "../fdv_generic/fdv_random.h"
#include "../fdv_generic/fdv_pin.h"
#include "../fdv_generic/fdv_debug.h"


#include "VirtualWire.h"


namespace fdv
{



  ////////////////////////////////////////////////////////////////
  // RPC (Remote Procedure Call) using VirtualWire

  struct WirelessRPC
  {

    
    static uint16_t const  BITSPERSEC      = 900;
    static uint16_t const  ACK_TIMEOUT     = 400;
    static uint16_t const  REPLY_TIMEOUT   = 2000;
    static uint8_t const   MAX_RESENDS     = 4;
    static uint16_t const  MAX_RANDOM_WAIT = 1000;
    static uint16_t const  SEND_DELAY      = 130;
    
    
    typedef uint8_t Method;
    

    // methods
    static Method const METHOD_GET_TEMPERATURE     = 200;
    static Method const METHOD_BOILER_ACTIVE       = 201;
    static Method const METHOD_BOILER_QUERY        = 202;
    static Method const METHOD_SYSTEM_UPTIME       = 203;
    static Method const METHOD_GET_CARBON_MONOXIDE = 204;
    static Method const METHOD_BUZZER_ALARM        = 205;

        
    // flags
    static uint8_t const FLAG_ACK        = 0b00000010;  // sending ACK (mutually exclusive with FLAG_PACKET)
    static uint8_t const FLAG_FIN        = 0b00001000;  // empty frame to signal ending (do not require ACK)
  
    
    struct MessageHeader
    {
      uint8_t srcDeviceID;
      uint8_t dstDeviceID;
      uint8_t packetSize;
      Method  method;
    };
    
    
    // ACK/FIN message
    struct CTRLMessage
    {
      uint16_t code;
    };
    
    
    struct IDispatcher
    {
      virtual void dispatch(uint8_t remoteDeviceID, WirelessRPC::Method method, uint8_t const* parameters, uint8_t parametersSize, uint8_t** results, uint8_t* resultsSize) = 0;
    };


    static uint8_t const MAX_FRAME_PAYLOAD = VW_MAX_PAYLOAD - sizeof(MessageHeader);  // MAX_FRAME_PAYLOAD=27
        
    
    // counters
    static uint32_t ACKTimeouts;
    static uint32_t SuccessCalls;
    static uint32_t FailedCalls;
    
    
    
    static void init(uint8_t localDeviceID_, bool serverMode, Pin const* tx_pin, Pin const* rx_pin)
    {
      s_localDeviceID = localDeviceID_;
      s_serverMode = serverMode;
      
      ACKTimeouts = 0;
      SuccessCalls = 0;
      FailedCalls = 0;
      
      vw_set_tx_pin(tx_pin);
      vw_set_rx_pin(rx_pin);
      vw_setup(BITSPERSEC);      
      if (s_serverMode)
        vw_rx_start();
    }
    
    
    static uint8_t localDeviceID()
    {
      return s_localDeviceID;
    }
    
    
    ///// Client mode
    
    // Returns "false" on timeout (no reply fromt the called)
    // There isn't limit (except for available memory) for "parameters" and "results" buffers size.
    // *results is always allocated (malloc) by call(). You have to free it.
    static bool call(uint8_t srcDeviceID, uint8_t dstDeviceID, Method method, 
                     uint8_t const* parameters, uint8_t parametersSize, 
                     uint8_t** results, uint8_t* resultsSize)
    {    
      //cout << "call\n";
      if (!s_serverMode)
        vw_rx_start();
      bool r = sendMethod(srcDeviceID, dstDeviceID, method, parameters, parametersSize) &&
               receiveMethod(true, &dstDeviceID, srcDeviceID, &method, results, resultsSize);
      if (!s_serverMode)
        vw_rx_stop();
      
      if (r)
        ++SuccessCalls;
      else
        ++FailedCalls;

      return r;
    }
    
    
    ////// Server mode
    
    // IDispatcher class must have:
    //    void dispatch(uint8_t remoteDeviceID, WirelessRPC::Method method, uint8_t const* parameters, uint8_t parametersSize, uint8_t** results, uint8_t* resultsSize)
    //    note: results must be allocated by dispatch() using "malloc" (when a result is returned)
    // Returns after a timeout if no messages are processed, so you have to call listen() in a loop
    static void listen(uint8_t localDeviceID, IDispatcher* dispatcher)
    {
      uint8_t remoteDeviceID;   // will be assigned by receiveMethod()
      Method method;            // will be assigned by receiveMethod()
      uint8_t* parameters;      // will be assigned by receiveMethod()
      uint8_t parametersSize;  // will be assigned by receiveMethod()
      if (vw_have_message() && receiveMethod(false, &remoteDeviceID, localDeviceID, &method, &parameters, &parametersSize))
      {
        // received a call
        //debug("listen: received a call");
        uint8_t* results = NULL;  // will be assigned by Dispatcher()()
        uint8_t resultsSize = 0; // will be assigned by Dispatcher()()
        dispatcher->dispatch(remoteDeviceID, method, parameters, parametersSize, &results, &resultsSize);
        freeEx(parameters);
        //cout << "listen: sending results\n";
        //cout << "   " << (char*)results << endl;
        //cout << "   size = " << (int)resultsSize << endl;
        sendMethod(localDeviceID, remoteDeviceID, method, results, resultsSize);
        freeEx(results);
      }
    }
    
    
  private:

    
    // parametersSize max is MAX_FRAME_PAYLOAD
    static bool sendMethod(uint8_t srcDeviceID, uint8_t dstDeviceID, Method method, uint8_t const* parameters, uint8_t parametersSize)
    {
      
      //cout << "sendMethod\n";
      uint8_t buffer[VW_MAX_PAYLOAD];
      
      // header
      MessageHeader* msg_header = (MessageHeader*)&buffer[0];
      msg_header->srcDeviceID = srcDeviceID;
      msg_header->dstDeviceID = dstDeviceID;
      msg_header->packetSize  = parametersSize;
      msg_header->method      = method;
      // payload
      if (parametersSize > 0)
        memcpy(&buffer[sizeof(MessageHeader)], parameters, parametersSize);

      // send and wait ACK
      for (uint8_t resendCount=0; ;++resendCount)
      {
        if (resendCount == MAX_RESENDS)
          return false; // abort!
        delay_ms(SEND_DELAY);
        vw_send(&buffer[0], sizeof(MessageHeader) + parametersSize);
        vw_wait_tx();          
        if (waitCTRL(srcDeviceID, dstDeviceID, method, FLAG_ACK))
          break;  // ok!
        // fail! Try to resend this frame waiting a random time
        delay_ms(Random::nextUInt16(MAX_RANDOM_WAIT/2, MAX_RANDOM_WAIT));
      }

      // send FIN (means "ready to receive your data")
      sendCTRL(srcDeviceID, dstDeviceID, method, FLAG_FIN);
      return true;
    }

    
    // Parameters buffer is always allocated in receiveMethod (you have to free it using free())
    // If expect=true, then accept only messages with the same value of *method and removeDeviceID
    static bool receiveMethod(bool expect, uint8_t* remoteDeviceID, uint8_t localDeviceID, Method* method, 
                              uint8_t** parameters, uint8_t* parametersSize)
    {
      //cout << "receiveMethod\n";
      *parameters = NULL;
      *parametersSize = 0;
            
      Buffer<uint8_t> params;
      
      uint8_t buffer[VW_MAX_PAYLOAD];
      MessageHeader const* msg_header = (MessageHeader*)&buffer[0];
      
      uint32_t t1 = millis();
      
      while (true)
      {
        if (millisDiff(t1, millis()) > REPLY_TIMEOUT)
          return false;
        
        if (vw_have_message())
        {        
          uint8_t messageLen = VW_MAX_PAYLOAD;                
          bool msgValid = vw_get_message(&buffer[0], &messageLen);
                        
          if ( msgValid &&
               msg_header->dstDeviceID == localDeviceID && 
               (msg_header->packetSize <= MAX_FRAME_PAYLOAD) &&
               (!expect || (expect && msg_header->method == *method)) &&
               (!expect || (expect && msg_header->srcDeviceID == *remoteDeviceID))            
             )
          {          
            if (msg_header->packetSize > params.size() && !params.resize(msg_header->packetSize))
              return false; // out of memory
            
            *remoteDeviceID = msg_header->srcDeviceID;
            *method = msg_header->method;
            
            if (msg_header->packetSize > 0)
              memcpy(&params[0], &buffer[sizeof(MessageHeader)], msg_header->packetSize);
                      
            // send ACK
            sendCTRL(localDeviceID, *remoteDeviceID, *method, FLAG_ACK);
            // exit
            break;
          }
        }
      }      
      // wait FIN
      if (!waitCTRL(localDeviceID, *remoteDeviceID, *method, FLAG_FIN))
        return false;
      // success
      *parametersSize = msg_header->packetSize;
      *parameters     = params.release();
      return true;
    }
    

    // wait ACK or FIN
    static bool waitCTRL(uint8_t localDeviceID, uint8_t remoteDeviceID, Method method, uint8_t flags)
    {
      uint32_t t1 = millis();
      while (millisDiff(t1, millis()) < ACK_TIMEOUT)
      {
        if (vw_have_message())
        {
          // read ACK/FIN
          CTRLMessage msg;
          uint8_t msglen = sizeof(CTRLMessage);
          bool msgValid = vw_get_message((uint8_t*)&msg, &msglen);
          uint16_t code = remoteDeviceID;
          code += localDeviceID;
          code += method;
          code += flags;
          if (msgValid && msglen == sizeof(CTRLMessage) && code == msg.code)
            return true;
          // otherwise wrong len, CRC, destination or message: retry to receive ACK/FIN
        }
      }  // end of while loop
      ++ACKTimeouts;
      return false;
    }
    
    
    // sends ACK/FIN
    static void sendCTRL(uint8_t srcDeviceID, uint8_t dstDeviceID, Method method, uint8_t flags)
    {
      CTRLMessage msg;
      msg.code  = srcDeviceID;
      msg.code += dstDeviceID;
      msg.code += method;
      msg.code += flags;
      delay_ms(SEND_DELAY);
      vw_send((uint8_t*)&msg, sizeof(CTRLMessage));
      vw_wait_tx();
    }

    
  private:
      
    static uint8_t s_localDeviceID;
    static bool s_serverMode;    
    
  };
  
  

  
} // end of fdv namespace


#endif  // FDV_WIRELESSRPC_H
