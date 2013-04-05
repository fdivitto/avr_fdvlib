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



#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <math.h>

#include <util/atomic.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <compat/twi.h>


#include "fdv_twowire.h"
#include "fdv_platform.h"


namespace fdv
{

  // TwoWireShared class storage
  TwoWireShared* TwoWireShared::s_instance = NULL;
  uint8_t        TwoWireShared::s_instanceCount = 0;



  uint8_t const TWI_READY = 0;
  uint8_t const TWI_MRX   = 1;
  uint8_t const TWI_MTX   = 2;
  uint8_t const TWI_SRX   = 3;
  uint8_t const TWI_STX   = 4;



  /*
   * Function twi_init
   * Desc     readys twi pins and sets twi bitrate
   * Input    none
   * Output   none
   */
  void TwoWireShared::twi_init()
  {
    // initialize state
    m_state = TWI_READY;

    #if defined(FDV_ATMEGA88_328)
      // activate internal pull-ups for twi
      // as per note from atmega8 manual pg167
      PORTC |= _BV(PC4);
      PORTC |= _BV(PC5);
    #elif defined(FDV_ATMEGA1280_2560)
      // activate internal pull-ups for twi
      // as per note from atmega128 manual pg204
      PORTD |= _BV(PD0);
      PORTD |= _BV(PD1);
    #endif

    // initialize twi prescaler and bit rate
    TWSR &= ~_BV(TWPS0);
    TWSR &= ~_BV(TWPS1);
    TWBR = ((F_CPU / 100000L) - 16) / 2;

    /* twi bit rate formula from atmega128 manual pg 204
    SCL Frequency = CPU Clock Frequency / (16 + (2 * TWBR))
    note: TWBR should be 10 or higher for master mode
    It is 72 for a 16mhz Wiring board with 100kHz TWI */

    // enable twi module, acks, and twi interrupt
    TWCR = _BV(TWEN) | _BV(TWIE) | _BV(TWEA);
  }


  /*
   * Function twi_slaveInit
   * Desc     sets slave address and enables interrupt
   * Input    none
   * Output   none
   */
  void TwoWireShared::twi_setAddress(uint8_t address)
  {
    // set twi slave address (skip over TWGCE bit)
    TWAR = address << 1;
  }


  /*
   * Function twi_readFrom
   * Desc     attempts to become twi bus master and read a
   *          series of bytes from a device on the bus
   * Input    address: 7bit i2c device address
   *          data: pointer to byte array
   *          length: number of bytes to read into array
   * Output   number of bytes read
   */
  uint8_t TwoWireShared::twi_readFrom(uint8_t address, uint8_t* data, uint8_t length)
  {
    // ensure data will fit into buffer
    if (BUFFER_LENGTH < length)
      return 0;

    // wait until twi is ready, become master receiver
    while (TWI_READY != m_state);

    m_state = TWI_MRX;

    // reset error state (0xFF.. no error occured)
    m_error = 0xFF;

    // initialize buffer iteration vars
    m_masterBufferIndex = 0;
    m_masterBufferLength = length-1;  // This is not intuitive, read on...
    // On receive, the previously configured ACK/NACK setting is transmitted in
    // response to the received byte before the interrupt is signalled.
    // Therefor we must actually set NACK when the _next_ to last byte is
    // received, causing that NACK to be sent in response to receiving the last
    // expected byte of data.

    // build sla+w, slave device address + w bit
    m_slarw = TW_READ;
    m_slarw |= address << 1;

    // send start condition
    TWCR = _BV(TWEN) | _BV(TWIE) | _BV(TWEA) | _BV(TWINT) | _BV(TWSTA);

    // wait for read operation to complete
    while (TWI_MRX == m_state);

    if (m_masterBufferIndex < length)
      length = m_masterBufferIndex;

    // copy twi buffer to data
    for (uint8_t i = 0; i < length; ++i)
      data[i] = m_masterBuffer[i];

    return length;
  }


  /*
   * Function twi_writeTo
   * Desc     attempts to become twi bus master and write a
   *          series of bytes to a device on the bus
   * Input    address: 7bit i2c device address
   *          data: pointer to byte array
   *          length: number of bytes in array
   *          wait: boolean indicating to wait for write or not
   * Output   0 .. success
   *          1 .. length to long for buffer
   *          2 .. address send, NACK received
   *          3 .. data send, NACK received
   *          4 .. other twi error (lost bus arbitration, bus error, ..)
   */
  uint8_t TwoWireShared::twi_writeTo(uint8_t address, uint8_t const* data, uint8_t length, uint8_t wait)
  {
    // ensure data will fit into buffer
    if (BUFFER_LENGTH < length)
      return 1;

    // wait until twi is ready, become master transmitter
    while (TWI_READY != m_state);

    m_state = TWI_MTX;

    // reset error state (0xFF.. no error occured)
    m_error = 0xFF;

    // initialize buffer iteration vars
    m_masterBufferIndex = 0;
    m_masterBufferLength = length;

    // copy data to twi buffer
    for (uint8_t i = 0; i < length; ++i)
      m_masterBuffer[i] = data[i];

    // build sla+w, slave device address + w bit
    m_slarw = TW_WRITE;
    m_slarw |= address << 1;

    // send start condition
    TWCR = _BV(TWEN) | _BV(TWIE) | _BV(TWEA) | _BV(TWINT) | _BV(TWSTA);

    // wait for write operation to complete
    while (wait && (TWI_MTX == m_state));

    if (m_error == 0xFF)
      return 0;   // success
    else if (m_error == TW_MT_SLA_NACK)
      return 2;   // error: address send, nack received
    else if (m_error == TW_MT_DATA_NACK)
      return 3;   // error: data send, nack received
    else
      return 4;   // other twi error
  }


  /*
   * Function twi_transmit
   * Desc     fills slave tx buffer with data
   *          must be called in slave tx event callback
   * Input    data: pointer to byte array
   *          length: number of bytes in array
   * Output   1 length too long for buffer
   *          2 not slave transmitter
   *          0 ok
   */
  uint8_t TwoWireShared::twi_transmit(uint8_t const* data, uint8_t length)
  {
    // ensure data will fit into buffer
    if (BUFFER_LENGTH < length)
      return 1;

    // ensure we are currently a slave transmitter
    if (TWI_STX != m_state)
      return 2;

    // set length and copy data into tx buffer
    m_txBufferLength = length;
    for (uint8_t i = 0; i < length; ++i)
      m_txBuffer[i] = data[i];

    return 0;
  }


  /*
   * Function twi_reply
   * Desc     sends byte or readys receive line
   * Input    ack: byte indicating to ack or to nack
   * Output   none
   */
  void TwoWireShared::twi_reply(uint8_t ack)
  {
    // transmit master read ready signal, with or without ack
    TWCR = ack? (_BV(TWEN) | _BV(TWIE) | _BV(TWINT) | _BV(TWEA)) : (_BV(TWEN) | _BV(TWIE) | _BV(TWINT));
  }


  /*
   * Function twi_stop
   * Desc     relinquishes bus master status
   * Input    none
   * Output   none
   */
  void TwoWireShared::twi_stop()
  {
    // send stop condition
    TWCR = _BV(TWEN) | _BV(TWIE) | _BV(TWEA) | _BV(TWINT) | _BV(TWSTO);

    // wait for stop condition to be exectued on bus
    // TWINT is not set after a stop condition!
    while (TWCR & _BV(TWSTO));

    // update twi state
    m_state = TWI_READY;
  }


  /*
   * Function twi_releaseBus
   * Desc     releases bus control
   * Input    none
   * Output   none
   */
  void TwoWireShared::twi_releaseBus()
  {
    // release bus
    TWCR = _BV(TWEN) | _BV(TWIE) | _BV(TWEA) | _BV(TWINT);

    // update twi state
    m_state = TWI_READY;
  }


  void TwoWireShared::handleInterrupt()
  {
    switch (TW_STATUS)
    {
      // All Master
      case TW_START:     // sent start condition
      case TW_REP_START: // sent repeated start condition
        // copy device address and r/w bit to output register and ack
        TWDR = m_slarw;
        twi_reply(1);
        break;

      // Master Transmitter
      case TW_MT_SLA_ACK:  // slave receiver acked address
      case TW_MT_DATA_ACK: // slave receiver acked data
        // if there is data to send, send it, otherwise stop
        if (m_masterBufferIndex < m_masterBufferLength)
        {
          // copy data to output register and ack
          TWDR = m_masterBuffer[m_masterBufferIndex++];
          twi_reply(1);
        }
        else
        {
          twi_stop();
        }
        break;
      case TW_MT_SLA_NACK:  // address sent, nack received
        m_error = TW_MT_SLA_NACK;
        twi_stop();
        break;
      case TW_MT_DATA_NACK: // data sent, nack received
        m_error = TW_MT_DATA_NACK;
        twi_stop();
        break;
      case TW_MT_ARB_LOST: // lost bus arbitration
        m_error = TW_MT_ARB_LOST;
        twi_releaseBus();
        break;

      // Master Receiver
      case TW_MR_DATA_ACK: // data received, ack sent
        // put byte into buffer
        m_masterBuffer[m_masterBufferIndex++] = TWDR;
      case TW_MR_SLA_ACK:  // address sent, ack received
        // ack if more bytes are expected, otherwise nack
        if (m_masterBufferIndex < m_masterBufferLength)
        {
          twi_reply(1);
        }
        else
        {
          twi_reply(0);
        }
        break;
      case TW_MR_DATA_NACK: // data received, nack sent
        // put final byte into buffer
        m_masterBuffer[m_masterBufferIndex++] = TWDR;
      case TW_MR_SLA_NACK: // address sent, nack received
        twi_stop();
        break;
      // TW_MR_ARB_LOST handled by TW_MT_ARB_LOST case

      // Slave Receiver
      case TW_SR_SLA_ACK:   // addressed, returned ack
      case TW_SR_GCALL_ACK: // addressed generally, returned ack
      case TW_SR_ARB_LOST_SLA_ACK:   // lost arbitration, returned ack
      case TW_SR_ARB_LOST_GCALL_ACK: // lost arbitration, returned ack
        // enter slave receiver mode
        m_state = TWI_SRX;
        // indicate that rx buffer can be overwritten and ack
        m_rxBufferIndex = 0;
        twi_reply(1);
        break;
      case TW_SR_DATA_ACK:       // data received, returned ack
      case TW_SR_GCALL_DATA_ACK: // data received generally, returned ack
        // if there is still room in the rx buffer
        if (m_rxBufferIndex < BUFFER_LENGTH)
        {
          // put byte in buffer and ack
          m_rxBuffer[m_rxBufferIndex++] = TWDR;
          twi_reply(1);
        }
        else
        {
          // otherwise nack
          twi_reply(0);
        }
        break;
      case TW_SR_STOP: // stop or repeated start condition received
        // put a null char after data if there's room
        if (m_rxBufferIndex < BUFFER_LENGTH)
        {
          m_rxBuffer[m_rxBufferIndex] = '\0';
        }
        // sends ack and stops interface for clock stretching
        twi_stop();

        // callback to user defined callback
        //twi_onSlaveReceive(twi_rxBuffer, twi_rxBufferIndex);
        if (m_onSlaveReceive)
          m_onSlaveReceive(m_rxBuffer, m_rxBufferIndex);

        // since we submit rx buffer to "wire" library, we can reset it
        m_rxBufferIndex = 0;
        // ack future responses and leave slave receiver state
        twi_releaseBus();
        break;
      case TW_SR_DATA_NACK:       // data received, returned nack
      case TW_SR_GCALL_DATA_NACK: // data received generally, returned nack
        // nack back at master
        twi_reply(0);
        break;

      // Slave Transmitter
      case TW_ST_SLA_ACK:          // addressed, returned ack
      case TW_ST_ARB_LOST_SLA_ACK: // arbitration lost, returned ack
        // enter slave transmitter mode
        m_state = TWI_STX;
        // ready the tx buffer index for iteration
        m_txBufferIndex = 0;
        // set tx buffer length to be zero, to verify if user changes it
        m_txBufferLength = 0;

        // request for txBuffer to be filled and length to be set
        // note: user must call twi_transmit(bytes, length) to do this
        //twi_onSlaveTransmit();
        if (m_onSlaveRequest)
          m_onSlaveRequest();

        // if they didn't change buffer & length, initialize it
        if (0 == m_txBufferLength)
        {
          m_txBufferLength = 1;
          m_txBuffer[0] = 0x00;
        }
        // transmit first byte from buffer, fall
      case TW_ST_DATA_ACK: // byte sent, ack returned
        // copy data to output register
        TWDR = m_txBuffer[m_txBufferIndex++];
        // if there is more to send, ack, otherwise nack
        if (m_txBufferIndex < m_txBufferLength)
        {
          twi_reply(1);
        }
        else
        {
          twi_reply(0);
        }
        break;
      case TW_ST_DATA_NACK: // received nack, we are done
      case TW_ST_LAST_DATA: // received ack, but we are done already!
        // ack future responses
        twi_reply(1);
        // leave slave receiver state
        m_state = TWI_READY;
        break;

      // All
      case TW_NO_INFO:   // no state information
        break;
      case TW_BUS_ERROR: // bus error, illegal stop/start
        m_error = TW_BUS_ERROR;
        twi_stop();
        break;
    }
  }


  // interrupt service routine
  ISR(TWI_vect)
  {
    TwoWireShared* instance = TwoWireShared::getInstance();
    if (instance)
      instance->handleInterrupt();
  }


}  // end of fdv namespace
