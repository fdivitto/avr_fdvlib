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




#ifndef FDV_SERIAL_H_
#define FDV_SERIAL_H_


#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include <alloca.h>

#include <avr/pgmspace.h>


#include "fdv_string.h"
#include "fdv_vector.h"
#include "fdv_timesched.h"


namespace fdv
{

#if defined(FDV_ATMEGA1280_2560)
  static uint8_t const SERIAL_COUNT = 4;
#else
  static uint8_t const SERIAL_COUNT = 1;
#endif


#if F_CPU == 16000000L
  // 16Mhz
  static uint16_t const BPS_TO_UBR[] PROGMEM = {416, 207, 103, 68, 51, 25, 16, 8};   // 2400, 4800, 9600, 14400, 19200, 38400, 57600, 115200
#elif F_CPU == 8000000L
  // 8Mhz
  static uint16_t const BPS_TO_UBR[] PROGMEM = {207, 103, 51, 34, 25, 12, 8, 3};     // 2400, 4800, 9600, 14400, 19200, 38400, 57600, 115200
#elif F_CPU == 20000000L
  // 20Mhz
  static uint16_t const BPS_TO_UBR[] PROGMEM = {520, 259, 129, 86, 64, 32, 21, 10};  // 2400, 4800, 9600, 14400, 19200, 38400, 57600, 115200
#else
#error Please implements other F_CPU values
#endif



  uint8_t volatile* const SERIAL_CONF[SERIAL_COUNT][5] =
  {
    {&UBRR0H, &UBRR0L, &UCSR0A, &UCSR0B, &UDR0},
#if defined(FDV_ATMEGA1280_2560)
    {&UBRR1H, &UBRR1L, &UCSR1A, &UCSR1B, &UDR1},
    {&UBRR2H, &UBRR2L, &UCSR2A, &UCSR2B, &UDR2},
    {&UBRR3H, &UBRR3L, &UCSR3A, &UCSR3B, &UDR3}
#endif
  };


  uint8_t const SERIAL_BITS[SERIAL_COUNT][5] =
  {
    {RXEN0, TXEN0, RXCIE0, UDRE0, U2X0},
#if defined(FDV_ATMEGA1280_2560)
    {RXEN1, TXEN1, RXCIE1, UDRE1, U2X1},
    {RXEN2, TXEN2, RXCIE2, UDRE2, U2X2},
    {RXEN3, TXEN3, RXCIE3, UDRE3, U2X3}
#endif
  };


  // TODO: support ATTiny84/85?
  template <uint8_t SerialIndexV>
  class HardwareSerial
  {

  public:

    static uint8_t const RX_BUFFER_SIZE = 16;

    enum BPS
    {
      BPS_2400   = 0,
      BPS_4800   = 1,
      BPS_9600   = 2,
      BPS_14400  = 3,
      BPS_19200  = 4,
      BPS_38400  = 5,
      BPS_57600  = 6,
      BPS_115200 = 7
    };

  private:

    struct RingBuffer
    {
      uint8_t buffer[RX_BUFFER_SIZE];
      uint8_t head;
      uint8_t tail;

			RingBuffer()
				: head(0),
          tail(0)
			{
			}

      void put(uint8_t c)
      {
        uint8_t i = (head + 1) % RX_BUFFER_SIZE;
        if (i != tail)
        {
          buffer[head] = c;
          head = i;
        }
      }
    };


  public:

    explicit HardwareSerial(BPS bps)
    {
			ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
			{
				s_buffer = &m_RXBuffer;    

				*SERIAL_CONF[SerialIndexV][UCSRnA] = 0;
				uint16_t const baud_setting = pgm_read_word(&BPS_TO_UBR[bps]);
				*SERIAL_CONF[SerialIndexV][UBRnH] = baud_setting >> 8;
				*SERIAL_CONF[SerialIndexV][UBRnL] = baud_setting & 0xFF;

				*SERIAL_CONF[SerialIndexV][UCSRnB] |= _BV(SERIAL_BITS[SerialIndexV][RXENn]) | _BV(SERIAL_BITS[SerialIndexV][TXENn]) | _BV(SERIAL_BITS[SerialIndexV][RXCIEn]);
			}
    }


    uint8_t available()
    {
      return (RX_BUFFER_SIZE + m_RXBuffer.head - m_RXBuffer.tail) % RX_BUFFER_SIZE;
    }


    uint8_t read(uint8_t* buffer, uint8_t bufferLen)
    {
      uint8_t ret = 0;
      for (;bufferLen>0 && m_RXBuffer.head != m_RXBuffer.tail; --bufferLen, ++ret)
      {
        *buffer++ = m_RXBuffer.buffer[m_RXBuffer.tail];
        m_RXBuffer.tail = (m_RXBuffer.tail + 1) % RX_BUFFER_SIZE;
      }
      return ret;
    }


    // timeout in ms
    string const readLine(uint32_t timeout = 2000)
    {
      uint32_t t1 = millis();
      string out;
      while (millisDiff(t1, millis()) < timeout)
      {
        if (available() > 0)
        {
          uint8_t c;
          read(&c, 1);
          if (c==0x0A)  // LF
            break;
          if (c==0x0D)  // CR+LF
          {
            read(&c, 1);  // bypass LF
            break;
          }
          out.push_back(c);
          t1 = millis();  // reset timout timer
        }
      }
      return out;
    }


    uint8_t readChar()
    {
      uint8_t c = 0;	// avoid unused warning
      read(&c, 1);
      return c;
    }


    void flush()
    {
      m_RXBuffer.head = m_RXBuffer.tail;
    }


    void write(uint8_t const* buffer, uint16_t bufferLen)
    {
      for (;bufferLen > 0; --bufferLen)
      {
        while (!((*SERIAL_CONF[SerialIndexV][UCSRnA]) & (1 << SERIAL_BITS[SerialIndexV][UDREn])));
        *SERIAL_CONF[SerialIndexV][UDRn] = *buffer++;
      }
    }
	
	
		void writeChar(uint8_t c)
		{
			write(&c, 1);
		}


    void write(char const* str)
    {
      write((uint8_t const*)str, strlen(str));
    }


    void write_P(PGM_P str)
    {
      uint8_t c;
      while ( (c = pgm_read_byte(str++)) )
        write(&c, 1);
    }


    void writeStrBytes(uint8_t const* buffer, uint16_t len, char sep)
    {
      for (uint8_t i = 0; i != len; ++i)
      {
        writeUInt32(*buffer++);
        if (i != len - 1)
          write((uint8_t const*)&sep, 1);
      }
    }


    void writeIPv4(uint8_t const* IP)
    {
      for (uint8_t i = 0; i != 4; ++i)
      {
        writeUInt32(IP[i]);
        if (i != 3)
          write_P(PSTR("."));        
      }
    }    


    void writeMAC(uint8_t const* MAC)
    {
      for (uint8_t i = 0; i != 6; ++i)
      {
        writeHEX(MAC[i]);
        if (i != 5)
          write_P(PSTR(":"));        
      }
    }


    void writeHEX(uint8_t value)
    {
      static char const hex[] PROGMEM = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};      
      char out[2] = { pgm_read_byte(&hex[value >> 4]), pgm_read_byte(&hex[value & 0x0F]) };
      write((uint8_t const*)&out[0], 2);
    }


    void writeUInt32(uint32_t value)
    {
      bool printZero = false;
      for (int8_t i = 9; i >= 0; --i)
      {
        uint32_t d = 1;
        for (int8_t j = 0; j != i; ++j)
          d *= 10;
        uint32_t v = value / d;
        if (v != 0 || printZero || i == 0)
        {
          uint8_t c = '0' + v;
          write(&c, 1);
          printZero = true;
        }
        value = value - v * d;
      }
    }



    static RingBuffer* getBuffer()
    {
      return s_buffer;
    }


  private:

    static RingBuffer* s_buffer;

    static uint8_t const UBRnH  = 0;
    static uint8_t const UBRnL  = 1;
    static uint8_t const UCSRnA = 2;
    static uint8_t const UCSRnB = 3;
    static uint8_t const UDRn   = 4;

    static uint8_t const RXENn  = 0;
    static uint8_t const TXENn  = 1;
    static uint8_t const RXCIEn = 2;
    static uint8_t const UDREn  = 3;
    static uint8_t const UZXn   = 4;

    RingBuffer m_RXBuffer;
  };


  template <uint8_t SerialIndexV>
  typename HardwareSerial<SerialIndexV>::RingBuffer* HardwareSerial<SerialIndexV>::s_buffer;



  template <typename T>
  T& operator<< (T& s, string const& str)
  {
    s.write(str.c_str());
    return s;
  }


  template <typename T>
  T& operator<< (T& s, char const* str)
  {
    s.write(str);
    return s;
  }


  template <typename T>
  T& operator<< (T& s, char str)
  {
    s.write((uint8_t const*)&str, 1);
    return s;
  }


  template <typename T>
  T& operator<< (T& s, uint8_t value)
  {
    s.writeUInt32(value);
    return s;
  }


  template <typename T>
  T& operator<< (T& s, uint16_t value)
  {
    s.writeUInt32(value);
    return s;
  }


  template <typename T>
  T& operator<< (T& s, uint32_t value)
  {
    s.writeUInt32(value);
    return s;
  }

  template <typename T>
  T& operator<< (T& s, uint64_t value)
  {
    s.write(toString(value).c_str());
    return s;
  }

  /*
  template <typename T>
  HardwareSerial& operator<< (HardwareSerial& s, T const& v)
  {
  s.write(toString(v).c_str());
  return s;
  }*/


  template <typename S, typename T>
  S& operator<< (S& s, vector<T> const& v)
  {
    s.write_P(PSTR("( "));
    for (typename fdv::vector<T>::const_iterator i=v.begin(); i!=v.end(); ++i)
      s << *i << ' ';
    s << ')';
    return s;
  }


  template <typename T>
  T& endl(T& s)
  {
    s.write_P(PSTR("\x0d\x0a"));
    return s;
  }


  // allow "endl"
  template <typename T>
  T& operator<< (T& s, T& (*pf)(T&))
  {
    return pf(s);
  }


#ifndef cout
#  define cout serial
#endif


}


#endif /* FDV_SERIAL_H_ */
