/*
# Created by Fabrizio Di Vittorio (fdivitto@gmail.com)
# Copyright (c) 2013 Fabrizio Di Vittorio.
# All rights reserved.
 
# GNU GPL LICENSE
#
# This module is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; latest version thereof,
# available at: <http://www.gnu.org/licenses/gpl.txt>.
#
# This module is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this module; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA
*/





#ifndef FDV_KS898RF_H
#define FDV_KS898RF_H

#include <stdlib.h>
#include <inttypes.h>
#include <stdarg.h>

#include <avr/interrupt.h>
#include <util/atomic.h>

// fdv includes
#include "../fdv_generic/fdv_memory.h"
#include "../fdv_generic/fdv_timesched.h"
#include "../fdv_generic/fdv_pin.h"
#include "../fdv_generic/fdv_spi.h"
#include "../fdv_generic/fdv_interrupt.h"
#include "../fdv_generic/fdv_random.h"


namespace fdv
{


  ////////////////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////////////////
  // KS898_Buffer
  // Contains 4 times (saving high/low state for each time)

  class KS898_Buffer
  {

  public:

    void clear() 
    {
      m_data.clear();
    }

    uint8_t size() const
    {
      return m_data.size();
    }

    uint8_t maxSize() const
    {
      return m_data.maxSize();
    }

    void add(uint16_t const& time, bool state) 
    {
      m_data.add((time > 0x7FFF ? 0x7FFF : time) | (state? 0x8000 : 0x0000));
    }

    uint16_t getTime(uint8_t index) const
    {
      return m_data[index] & 0x7FFF;
    }

    bool getState(uint8_t index) const
    {
      return (m_data[index] & 0x8000)? true : false;
    }

    void del_front(uint8_t n) 
    {
      m_data.del_front(n);
    }

  private:
    
    CircularBuffer<uint16_t, 4> m_data;
  };


  ////////////////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////////////////
  // KS898_Sensor

  class KS898_Sensor
  {

  private:

    static uint8_t const MAXSIZE = 20;  // must be a multiple of 4

  public:

    KS898_Sensor()
    {
      clear();
    }

    KS898_Sensor(uint8_t packedSize_, uint8_t const* packedData_)
    {
      clear();
      setPacked(packedSize_, packedData_);
    }

    KS898_Sensor(PGM_P codeStr)
    {
      clear();
      char c;
      while ( (c = pgm_read_byte(codeStr++)) )
        push_back(c);
    }

    void setPacked(uint8_t packedSize_, uint8_t const* packedData_)
    {
      m_size = packedData_[0];
      for (uint8_t i = 1; i != packedSize_; ++i)
        m_data[i - 1] = packedData_[i];
    }

    void clear()
    {
      m_size = 0;
      for (uint8_t i = 0; i != MAXSIZE / 4; ++i)
        m_data[i] = 0;
    }

    uint8_t size() const
    {
      return m_size;
    }

    uint8_t maxSize() const
    {
      return MAXSIZE;
    }

    // ret number of bytes necessary (includes first byte which is the words count)
    uint8_t packedSize() const
    {
      return m_size / 4 + (m_size % 4 == 0? 0 : 1) + 1;
    }

    // ret packed value (0..packedSize()-1)
    uint8_t packedValue(uint8_t index) const
    {
      return index == 0? m_size : m_data[index-1];
    }

    // symbol can be: '0', '1', 'F', 'G'
    void push_back(char symbol)
    {
      uint8_t index = m_size++;
      m_data[index / 4] |= symbolToValue(symbol) << (2 * (index % 4));
    }

    // can return: '0', '1', 'F', 'G'
    char operator[] (uint8_t index) const
    {
      return valueToSymbol(m_data[index / 4] >> (2 * (index % 4)) & 0b11);
    }

    bool operator== (KS898_Sensor const& rhs) const
    {
      if (rhs.m_size == m_size)
      {
        for (uint8_t i = 0; i != MAXSIZE / 4; ++i)
          if (rhs.m_data[i] != m_data[i])
            return false;
        return true;
      }
      return false;
    }

    bool operator!= (KS898_Sensor const& rhs) const
    {
      return !(rhs == *this);
    }


  private:

    uint8_t symbolToValue(char symbol) const
    {
      switch (symbol)
      {
        case '1':
          return 0b01;
        case 'F':
          return 0b10;
        case 'G':
          return 0b11;
        default:  // error or '0'
          return 0b00;
      }
    }

    char valueToSymbol(uint8_t value) const
    {
      switch (value)
      {
        case 0b01:
          return '1';
        case 0b10:
          return 'F';
        case 0b11:
          return 'G';
        default:  // error or '0'
          return '0';
      }
    }

    // Two bits up to 4 per byte:
    //   00 = 0
    //   01 = 1
    //   10 = F
    //   11 = G
    uint8_t volatile m_data[MAXSIZE / 4];
    uint8_t volatile m_size;

  };



  ////////////////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////////////////
  // KS898_SensorHandler

  class KS898_SensorReceiver : private PCExtInterrupt::IExtInterruptCallable
  {

  private:

    static uint16_t const LARGERMUL             = 2;     // multiplier to compare two values
    
    static uint16_t const MIN_LITTLE_SYNC_PULSE = 100;   // us
    static uint16_t const MAX_LITTLE_SYNC_PULSE = 2000;  // us
    static uint16_t const MIN_LARGE_SYNC_PULSE  = 4000;  // us
    static uint16_t const MAX_LARGE_SYNC_PULSE  = 50000; // us

    static uint16_t const MAX_PULSE             = 1200;  // us
    //static uint16_t const MAX_PULSE             = 1600;  // us (1600 to allow SC2262 devices - from ebay...)

  public:

    static uint8_t const STATUS_DATAREADY = 0b00000001;
    static uint8_t const STATUS_SYNCFOUND = 0b00000010;
    static uint8_t const STATUS_SIZE12    = 0b00000100;
    static uint8_t const STATUS_SIZE20    = 0b00001000;

 private:

    Pin const*       m_pin;
    KS898_Buffer     m_buffer;
    KS898_Sensor     m_sensor;
    uint8_t volatile m_status;


  public:

    KS898_SensorReceiver(PCExtInterrupt::ExtInt extint, Pin const* pin)
      : m_pin(pin), m_status()
    {
      setStatus(STATUS_SIZE12, true);
      setStatus(STATUS_DATAREADY | STATUS_SIZE20 | STATUS_SYNCFOUND, false);
      
      // setup timer 1 (16 bit)
      TCCR1A = 0;
      TCCR1B = 1 << CS11; // clkIO / 8. At 16Mhz, one increment every 0.5us (500ns)
      TCCR1C = 0;
      TIMSK1 = 0;

      // setup interrupt
      PCExtInterrupt::attach(extint, this);
    }

    KS898_Sensor const& ReceivedSensor()
    {
      return m_sensor;
    }
    
    bool getStatus(uint8_t flag) const
    {
      return m_status & flag;
    }
    
    void setStatus(uint8_t flag, bool value)
    {
      m_status = value? (m_status | flag) : (m_status & ~flag);
    }
            
    void discardData()
    {
      m_sensor.clear();
      setStatus(STATUS_DATAREADY | STATUS_SYNCFOUND | STATUS_SIZE20, false);
      setStatus(STATUS_SIZE12, true);
    }


  private:

    bool IsLessThan(uint8_t idx_a, uint8_t idx_b)
    {
      return m_buffer.getTime(idx_a) * LARGERMUL <= m_buffer.getTime(idx_b);
    }


    uint8_t maxSensorSize()
    {
      return getStatus(STATUS_SIZE12)? 12 : 20;
    }


    // interrupt occurs on m_pin state change (low->high or high->low)
    virtual void extInterrupt()
    {
      // get state
      uint8_t st = m_pin->read();

      // get time
      uint16_t t = static_cast<uint16_t>(TCNT1);

      // reset timer, start a new measurement
      TCNT1 = 0;
      
      // remove first time if LOW
      if (m_buffer.getState(0) == false)
        m_buffer.del_front(1);
      
      // m_sensor is available for new sensor data?
      if (!getStatus(STATUS_DATAREADY))
      {
        // check buffer for synch
        if (m_buffer.size() > 1 &&
            m_buffer.getTime(0) >= MIN_LITTLE_SYNC_PULSE * 2 && 
            m_buffer.getTime(1) >= MIN_LARGE_SYNC_PULSE * 2 &&
            m_buffer.getTime(0) <= MAX_LITTLE_SYNC_PULSE * 2 && 
            m_buffer.getTime(1) <= MAX_LARGE_SYNC_PULSE * 2)
        {
          discardData();
          setStatus(STATUS_SYNCFOUND, true);
          m_buffer.del_front(2);
        }          
      
        // check buffer for data
        if (m_buffer.size() == 4 && getStatus(STATUS_SYNCFOUND))
        {
          char symbol = ' ';          
          if (m_buffer.getTime(0) <= 2 * MAX_PULSE && m_buffer.getTime(1) <= 2 * MAX_PULSE && m_buffer.getTime(2) <= 2 * MAX_PULSE && m_buffer.getTime(3) <= 2 * MAX_PULSE)  // too large?
          {            
            if (IsLessThan(0, 1) && IsLessThan(2, 3) && IsLessThan(0, 3) && IsLessThan(2, 1))        // 0?
              symbol = '0';            
            else if (IsLessThan(1, 0) && IsLessThan(3, 2) && IsLessThan(3, 0) && IsLessThan(1, 2))   // 1?
              symbol = '1';            
            else if (IsLessThan(0, 1) && IsLessThan(3, 2) && IsLessThan(0, 2) && IsLessThan(3, 1))   // F?
              symbol = 'F';            
            else if (IsLessThan(1, 0) && IsLessThan(2, 3) && IsLessThan(1, 3) && IsLessThan(2, 0))   // G?
              symbol = 'G';
          } 
          if (symbol == ' ' || m_sensor.size() >= maxSensorSize())
          {
            // invalid symbol or error
            discardData();
            m_buffer.del_front(2);            
          }           
          else
          {
            // valid symbol
            m_sensor.push_back(symbol);
            m_buffer.clear(); 
            // is 20 bit code?
            if (symbol == 'G')   
            {        
              setStatus(STATUS_SIZE20, true);
              setStatus(STATUS_SIZE12, false);
            }              
          }
          
          // sensor valid?
          if (m_sensor.size() == maxSensorSize())
          {
            setStatus(STATUS_DATAREADY, true);
          }
        }
      }        
      
      // save measurement
      m_buffer.add(t, st? false : true);      
    }
    
  };



} // end of fdv namespace



#endif /* FDV_KS898RF_H */


