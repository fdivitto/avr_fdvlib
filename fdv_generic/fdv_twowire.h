/*
  TwoWire.cpp - TWI/I2C library for Wiring & Arduino
  Copyright (c) 2006 Nicholas Zambetti.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

  2010 modified by Fabrizio Di Vittorio (fdivitto@tiscali.it)
*/




#ifndef FDV_TWOWIRE_H_
#define FDV_TWOWIRE_H_


#include <inttypes.h>

#include <util/atomic.h>


namespace fdv
{



  // I2C/TWI support (one instance maximum)
  class TwoWireShared
  {

  private:

    // private: only TwoWireShared can create itself
    TwoWireShared()
    : m_onSlaveRequest(NULL), m_onSlaveReceive(NULL)
    {
    }


  public:

    typedef void (*SlaveReceiveFunc)(uint8_t const*, uint8_t);
    typedef void (*SlaveRequestFunc)();


    void initMaster()
    {
      twi_init();
    }


    void initSlave(uint8_t slaveAddress, SlaveRequestFunc slaveRequestFunc, SlaveReceiveFunc slaveReceiveFunc)
    {
      m_onSlaveRequest = slaveRequestFunc;
      m_onSlaveReceive = slaveReceiveFunc;
      twi_setAddress(slaveAddress);
      twi_init();
    }


    // master write
    bool writeTo(uint8_t address, uint8_t const* buffer, uint8_t bufferLen)
    {
      return twi_writeTo(address, buffer, bufferLen, 1) == 0;
    }


    // slave write
    void write(uint8_t const* buffer, uint8_t bufferLen)
    {
      twi_transmit(buffer, bufferLen);
    }


    // master read
    uint8_t readFrom(uint8_t address, uint8_t* buffer, uint8_t bufferLen)
    {
      return twi_readFrom(address, buffer, bufferLen);
    }


    static TwoWireShared* getInstance()
    {
      return s_instance;
    }


    static void createInstance()
    {
      if (!s_instance)
        s_instance = new TwoWireShared;
      ++s_instanceCount;
    }


    static void releaseInstance()
    {
      --s_instanceCount;
      if (s_instanceCount==0)
      ATOMIC_BLOCK(ATOMIC_RESTORESTATE) // avoid ISR usage
      {
        delete s_instance;
        s_instance = NULL;
      }
    }


    void handleInterrupt();


  private:

    static TwoWireShared* s_instance;       // access to the unique instance (to call getInstance())
    static uint8_t        s_instanceCount;  // number of shares

    static uint8_t const BUFFER_LENGTH = 32;

    SlaveRequestFunc m_onSlaveRequest;
    SlaveReceiveFunc m_onSlaveReceive;
    uint8_t volatile m_state;
    uint8_t          m_slarw;
    uint8_t          m_masterBuffer[BUFFER_LENGTH];
    uint8_t volatile m_masterBufferIndex;
    uint8_t          m_masterBufferLength;
    uint8_t          m_txBuffer[BUFFER_LENGTH];
    uint8_t volatile m_txBufferIndex;
    uint8_t volatile m_txBufferLength;
    uint8_t          m_rxBuffer[BUFFER_LENGTH];
    uint8_t volatile m_rxBufferIndex;
    uint8_t volatile m_error;

    void twi_init();
    void twi_setAddress(uint8_t address);
    uint8_t twi_readFrom(uint8_t address, uint8_t* data, uint8_t length);
    uint8_t twi_writeTo(uint8_t address, uint8_t const* data, uint8_t length, uint8_t wait);
    uint8_t twi_transmit(uint8_t const* data, uint8_t length);
    void twi_reply(uint8_t ack);
    void twi_stop();
    void twi_releaseBus();

  };


  // I2C/TWI support (multiple instances share one TwoWireShared)
  // Note: slave can have only one instance (master not allowed in this case)
  class TwoWire
  {
  public:

    // master
    TwoWire()
    {
      TwoWireShared::createInstance();
      TwoWireShared::getInstance()->initMaster();
    }


    // slave
    TwoWire(uint8_t slaveAddress, TwoWireShared::SlaveRequestFunc slaveRequestFunc, TwoWireShared::SlaveReceiveFunc slaveReceiveFunc)
    {
      TwoWireShared::createInstance();
      TwoWireShared::getInstance()->initSlave(slaveAddress, slaveRequestFunc, slaveReceiveFunc);
    }


    ~TwoWire()
    {
      TwoWireShared::releaseInstance();
    }


    // master write
    bool writeTo(uint8_t address, uint8_t const* buffer, uint8_t bufferLen)
    {
      return TwoWireShared::getInstance()->writeTo(address, buffer, bufferLen);
    }


    // slave write
    void write(uint8_t const* buffer, uint8_t bufferLen)
    {
      TwoWireShared::getInstance()->write(buffer, bufferLen);
    }


    // master read
    uint8_t readFrom(uint8_t address, uint8_t* buffer, uint8_t bufferLen)
    {
      return TwoWireShared::getInstance()->readFrom(address, buffer, bufferLen);
    }


  };





}  // end of fdv namespace

#endif /* FDV_TWOWIRE_H_ */
