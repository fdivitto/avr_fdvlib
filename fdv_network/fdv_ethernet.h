// 2010 by Fabrizio Di Vittorio (fdivitto@tiscali.it)



#ifndef FDV_ETHERNET_H
#define FDV_ETHERNET_H


#include <string.h>
#include <stdlib.h>
#include <alloca.h>
#include <inttypes.h>
#include <math.h>
#include <avr/pgmspace.h>

#include "w5100.h"
#include "socket.h"


#include "../fdv_generic/fdv_string.h"
#include "../fdv_generic/fdv_algorithm.h"
#include "../fdv_generic/fdv_datetime.h"
#include "../fdv_generic/fdv_timesched.h"


namespace fdv
{

  
  // the time (ms) of last operation (connection/data)
  extern uint32_t s_networkIdleTime;

  
  enum SockStatus
  {
    UNUSED,     // unused by this instance, could be used by others
    LISTENING,  // in listening state
    CONNECTED   // connected    
  };

  
  ////////////////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////////////////
  // Ethernet
  
  struct Ethernet
  {

    Ethernet(HardwareSPIMaster* spi, char const* MAC, char const* IP, char const* gateway, char const* subnet)
    {
      init(spi, MAC, IP, gateway, subnet);
    }


    void init(HardwareSPIMaster* spi, char const* MAC, char const* IP, char const* gateway, char const* subnet)
    {
      //debug << "Ethernet::init " << MAC << " " << IP << " " << gateway << " " << subnet << ENDL;

      uint8_t newMAC[6], newIP[4], newGateway[4], newSubnet[4];
      parseAddr(MAC, newMAC, 16, 6);
      parseAddr(IP, newIP, 10, 4);
      parseAddr(gateway, newGateway, 10, 4);
      parseAddr(subnet, newSubnet, 10, 4);
      
      for (SOCKET s=0; s != MAX_SOCK_NUM; ++s)
        sockStatus(s, UNUSED);
     
      TaskManager::init();

      m_W5100.init(spi);
      m_W5100.setMACAddress(newMAC);
      m_W5100.setIPAddress(newIP);
      m_W5100.setGatewayIp(newGateway);
      m_W5100.setSubnetMask(newSubnet);
      
      // verify
      uint8_t verIP[4];
      m_W5100.getIPAddress(&verIP[0]);
      m_available = (memcmp(&verIP[0], &newIP[0], 4) == 0);

      /*
      if (!m_available)
        debug << "Ethernet::init FAILED!" << ENDL;
        */

      s_networkIdleTime = millis();
    }

    
    void reset()
    {
      uint8_t MAC[6], IP[4], gateway[4], subnet[4];

      m_W5100.getMACAddress(MAC);
      m_W5100.getIPAddress(IP);
      m_W5100.getGatewayIp(gateway);
      m_W5100.getSubnetMask(subnet);

      m_W5100.init();

      m_W5100.setMACAddress(MAC);
      m_W5100.setIPAddress(IP);
      m_W5100.setGatewayIp(gateway);
      m_W5100.setSubnetMask(subnet);

      for (SOCKET s=0; s != MAX_SOCK_NUM; ++s)
        sockStatus(s, UNUSED);

      s_networkIdleTime = millis();
    }


    SockStatus sockStatus(SOCKET s)
    {
      return m_sockStatus[s];
    }


    void sockStatus(SOCKET s, SockStatus newValue)
    {
      m_sockStatus[s] = newValue;
    }


    uint8_t sockInternalStatus(SOCKET s)
    {
      return m_W5100.readSnSR(s);
    }

        
    // finds free/closed socket
    // 255 = not found
    SOCKET findFreeSocket()
    {
      for (SOCKET s=0; s!=MAX_SOCK_NUM; ++s)
        if (sockStatus(s)==UNUSED && sockInternalStatus(s)==SnSR::CLOSED)
          return s;
      return 255;
    }
    
    
    // look for a free source port
    // returns 0 on fail
    uint16_t findFreePort()
    {
      uint16_t port = 1024;
      for (bool found=false; !found && port!=0; ++port) // port!=0 : overflow to zero means not found
      {
        found = true;
        for (SOCKET s=0; s != MAX_SOCK_NUM; ++s)
          if (sockStatus(s)!=UNUSED && getLocalPort(s)==port)
          {
            found = false;
            break;
          }
      }
      return port;
    }
        
    
    
    // converts an ip string (DEC.DEC.DEC.DEC) or mac address (HEX:HEX:HEX...) to array
    static void parseAddr(char const* str, uint8_t* addr, uint8_t base, uint8_t count)
    {
      for (uint8_t i=0; i!=count; ++i, ++addr, ++str)
      {
        *addr = strtol(str, 0, base);
        for (; *str!=0 && *str!=':' && *str!='.'; ++str);
      };
    }
    
    
    // reads local socket port (listening port for server, source port for client)
    uint16_t getLocalPort(SOCKET s)
    {
      return m_W5100.readSnPORT(s);
    }    
    
    
    // reads remote port
    uint16_t getRemotePort(SOCKET s)
    {      
      return m_W5100.readSnDPORT(s);
    }
    
    
    // reads remote IP address (buf must be uint8_t array, 4 elements)
    void getRemoteIP(SOCKET s, uint8_t* buf)
    {
      m_W5100.readSnDIPR(s, buf);
    }

    
    // reads remote IP address as string
    string const getRemoteIP(SOCKET s, char sep = '.')
    {
      uint8_t buf[4];
      getRemoteIP(s, &buf[0]);
      return IP2String(buf, sep);
    }
    
    
    // reads local IP address (buf must be uint8_t array, 4 elements)
    void getLocalIP(uint8_t* buf)
    {
      m_W5100.readSIPR(buf);
    }
    
    
    string const getLocalIP(char sep = '.')
    {
      uint8_t buf[4];
      getLocalIP(&buf[0]);
      return IP2String(buf, sep);      
    }
            
    
    W5100Class* W5100()
    {
      return &m_W5100;
    }


    bool isAvailable()
    {
      return m_available;
    }

    
  private:

    static string const IP2String(uint8_t* buf, char sep = '.')
    {
      return toString(buf[0]) + sep + toString(buf[1]) + sep + toString(buf[2]) + sep + toString(buf[3]);
    }
    
    SockStatus m_sockStatus[MAX_SOCK_NUM];
    W5100Class m_W5100;
    bool       m_available;
    
  };
  
  
  


  
} // end of fdv namespace


#endif  // FDV_ETHERNET_H

