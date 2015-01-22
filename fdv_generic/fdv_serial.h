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


	//////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////
	// Abstract base class to allow non-template dependent calls

	class SerialBase
	{
		
		public:
			virtual void put(uint8_t value) = 0;
			virtual void write(uint8_t b) = 0;
			virtual bool isBufferOverflow() = 0;
			virtual int16_t peek() = 0;
			virtual int16_t read() = 0;
			virtual uint16_t available() = 0;
			virtual void flush() = 0;
			virtual void setForward(SerialBase* forwardTo) = 0;
					
			
			uint16_t read(uint8_t* buffer, uint16_t bufferLen)
			{
				uint16_t ret = 0;
				for (int16_t c; bufferLen > 0 && (c = read()) > -1; --bufferLen, ++ret)
				{
					*buffer++ = c;
				}
				return ret;
			}			
			

			// timeout in ms
			string const readLine(uint32_t timeOutMs = 2000)
			{
				string out;
				SoftTimeOut timeOut(timeOutMs);
				while (!timeOut)
				{
					if (available() > 0)
					{
						uint8_t c = read();
						if (c == 0x0A)  // LF
							break;
						if (c == 0x0D)  // CR+LF
						{
							read();  // bypass LF
							break;
						}
						out.push_back(c);
						timeOut.reset(timeOutMs);
					}
				}
				return out;
			}
			
			
			uint32_t readUInt32()
			{
				uint32_t result = 0;
				uint8_t buf[10];
				for (int8_t i = 0; i != 10; ++i)
				{					
					if (!waitForData())
						break;	// timeout
					char c = peek();
					if (c < '0' || c > '9')
					{
						uint32_t mul = 1;
						for (int8_t j = i - 1; j >= 0; --j)
						{
							result += buf[j] * mul;
							mul *= 10;
						}
						break;
					}
					buf[i] = read() - '0';					
				}
				return result;
			}
			
			
			bool waitForData(uint32_t timeOutMs = 500)
			{
				SoftTimeOut timeOut(timeOutMs);
				while (!timeOut)
					if (available() > 0)
						return true;
				return false;
			}
			
			
			bool waitFor(PGM_P waitStr, uint32_t timeOutMs = 500)
			{				
				SoftTimeOut timeOut(timeOutMs);
				uint8_t waitStrLen = strlen_P(waitStr);
				while (!timeOut)
				{
					if (available() >= waitStrLen)
					{
						PGM_P ws = waitStr;
						for (uint8_t i = 0; i != waitStrLen; ++i)
						{
							if (read() != pgm_read_byte(ws++))
								break;
							if (i == waitStrLen - 1)	// last char?
								return true;
						}
					}
				}
				return false;
			}
			
			
			bool waitForNewLine(uint32_t timeOutMs = 500)
			{
				while (true)
				{
					if (!waitForData(timeOutMs))
						return false;	// timeout
					if (read() == 0x0A) // LF (bypass CR and other chars)
						return true;
				}
			}
			
			
			// consume serial input (up to charCount characters) until no chars received for timeOutMs
			void consume(uint32_t timeOutMs = 500, uint16_t charCount = 65535)
			{
				while (true)
				{
					if (!waitForData(timeOutMs))
						return;
					read();
					if (--charCount == 0)
						break;
				}
			}
			
			
			void writeNewLine()
			{
				write(0x0D);
				write(0x0A);	
			}
			
			
			void write(uint8_t const* buffer, uint16_t bufferLen)
			{
				for (;bufferLen > 0; --bufferLen)
					write(*buffer++);
			}


			void write(char const* str)
			{
				write((uint8_t const*)str, strlen(str));
			}


			void write_P(PGM_P str)
			{
				uint8_t c;
				while ( (c = pgm_read_byte(str++)) )
					write(c);
			}


			void writeStrBytes(uint8_t const* buffer, uint16_t len, char sep)
			{
				for (uint8_t i = 0; i != len; ++i)
				{
					writeUInt32(*buffer++);
					if (i != len - 1)
						write(sep);
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
						write(c);
						printZero = true;
					}
					value = value - v * d;
				}
			}			
			
	};


	//////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////
	// Template base class for all serials (soft and hardware)
	// Implements circular buffer

	template <uint8_t RXBUFSIZE_V>
	class Serial : public SerialBase
	{
		public:

			Serial() :
				m_RXBufferTail(0),
				m_RXBufferHead(0),
				m_RXBufferOverflow(false),
				m_forwardTo(NULL)
			{
			}


			bool isBufferOverflow()
			{
				return m_RXBufferOverflow;
			}


			void put(uint8_t value)
			{
				ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
				{
					// if buffer full, set the overflow flag and return
					if ((m_RXBufferTail + 1) % RXBUFSIZE_V != m_RXBufferHead)
					{
						// save new data in buffer: tail points to where byte goes
						m_RXBuffer[m_RXBufferTail] = value; // save new byte
						m_RXBufferTail = (m_RXBufferTail + 1) % RXBUFSIZE_V;
					}
					else
					{
						m_RXBufferOverflow = true;
					}					
				}
			}


			int16_t peek()
			{
				int16_t r = -1;
				ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
				{					
					if (m_RXBufferHead == m_RXBufferTail) // Empty buffer?
						r = -1;
					else
						// Read from head
						r = m_RXBuffer[m_RXBufferHead];
				}
				return r;
			}


			int16_t read()
			{
				int16_t r = -1;

				ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
				{					
					if (m_RXBufferHead == m_RXBufferTail)  // Empty buffer?
						r = -1;
					else
					{
						// Read from head
						r = m_RXBuffer[m_RXBufferHead];
						m_RXBufferHead = (m_RXBufferHead + 1) % RXBUFSIZE_V;
					}
				}
				
				if (m_forwardTo && r > -1)
					m_forwardTo->write(r);
				
				return r;
			}


			uint16_t available()
			{
				uint16_t r = 0;
				ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
				{
					r = (m_RXBufferTail + RXBUFSIZE_V - m_RXBufferHead) % RXBUFSIZE_V;
				}
				return r;
			}


			void flush()
			{
				ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
				{
					m_RXBufferOverflow = false;
					m_RXBufferHead = m_RXBufferTail = 0;
				}
			}
			
			
			// each char received will be sent to m_forwardTo
			void setForward(SerialBase* forwardTo)
			{
				m_forwardTo = forwardTo;
			}



	private:

		char             m_RXBuffer[RXBUFSIZE_V];
		volatile uint8_t m_RXBufferTail;
		volatile uint8_t m_RXBufferHead;
		bool             m_RXBufferOverflow;
  	SerialBase*      m_forwardTo;	
		
	};


	//////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////

  template <typename SerialDesc_T, uint8_t RXBUFSIZE_V = 32>
  class HardwareSerial : public Serial<RXBUFSIZE_V>
  {

  public:

		using Serial<RXBUFSIZE_V>::peek;
		using Serial<RXBUFSIZE_V>::read;
		using Serial<RXBUFSIZE_V>::available;
		using Serial<RXBUFSIZE_V>::isBufferOverflow;
		using Serial<RXBUFSIZE_V>::put;
		using Serial<RXBUFSIZE_V>::flush;
		using Serial<RXBUFSIZE_V>::write;


    explicit HardwareSerial(uint32_t baud) :
			Serial<RXBUFSIZE_V>()
    {
			ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
			{
				SerialDesc_T::hardwareSerialInstance = this;
				*SerialDesc_T::UCSRnA() = _BV(SerialDesc_T::U2Xn);
				uint16_t const baud_setting = (F_CPU / 4 / baud - 1) / 2;
				*SerialDesc_T::UBRRnH() = baud_setting >> 8;
				*SerialDesc_T::UBRRnL() = baud_setting & 0xFF;
				*SerialDesc_T::UCSRnB() |= _BV(SerialDesc_T::RXENn) | _BV(SerialDesc_T::TXENn) | _BV(SerialDesc_T::RXCIEn);
				*SerialDesc_T::UCSRnC() = _BV(UCSZ01) | _BV(UCSZ00); 
			}
    }
			
	
		void write(uint8_t b)
		{
      while (!((*SerialDesc_T::UCSRnA()) & _BV(SerialDesc_T::UDREn)))
        ;
      *SerialDesc_T::UDRn() = b;
		}

  };



	struct DescSerial0
	{
		static uint8_t const RXENn  = RXEN0;
		static uint8_t const TXENn  = TXEN0;
		static uint8_t const RXCIEn = RXCIE0;
		static uint8_t const UDREn  = UDRE0;
		static uint8_t const U2Xn   = U2X0;
		static volatile uint8_t* UBRRnH()  { return &UBRR0H; }
		static volatile uint8_t* UBRRnL()  { return &UBRR0L; }
		static volatile uint8_t* UCSRnA()  { return &UCSR0A; }
		static volatile uint8_t* UCSRnB()  { return &UCSR0B; }
		static volatile uint8_t* UCSRnC()  { return &UCSR0C; }
		static volatile uint8_t* UDRn()    { return &UDR0; }
		static SerialBase* hardwareSerialInstance;
	};
	
	
	
	#if defined(FDV_ATMEGA1280_2560)

	struct DescSerial1
	{
		static uint8_t const RXENn  = RXEN1;
		static uint8_t const TXENn  = TXEN1;
		static uint8_t const RXCIEn = RXCIE1;
		static uint8_t const UDREn  = UDRE1;
		static uint8_t const U2Xn   = U2X1;
		static volatile uint8_t* UBRRnH()  { return &UBRR1H; }
		static volatile uint8_t* UBRRnL()  { return &UBRR1L; }
		static volatile uint8_t* UCSRnA()  { return &UCSR1A; }
		static volatile uint8_t* UCSRnB()  { return &UCSR1B; }
		static volatile uint8_t* UCSRnC()  { return &UCSR1C; }
		static volatile uint8_t* UDRn()    { return &UDR1; }
		static SerialBase* hardwareSerialInstance;
	};

	struct DescSerial2
	{
		static uint8_t const RXENn  = RXEN2;
		static uint8_t const TXENn  = TXEN2;
		static uint8_t const RXCIEn = RXCIE2;
		static uint8_t const UDREn  = UDRE2;
		static uint8_t const U2Xn   = U2X2;
		static volatile uint8_t* UBRRnH()  { return &UBRR2H; }
		static volatile uint8_t* UBRRnL()  { return &UBRR2L; }
		static volatile uint8_t* UCSRnA()  { return &UCSR2A; }
		static volatile uint8_t* UCSRnB()  { return &UCSR2B; }
		static volatile uint8_t* UCSRnC()  { return &UCSR2C; }
		static volatile uint8_t* UDRn()    { return &UDR2; }
		static SerialBase* hardwareSerialInstance;
	};

	struct DescSerial3
	{
		static uint8_t const RXENn  = RXEN3;
		static uint8_t const TXENn  = TXEN3;
		static uint8_t const RXCIEn = RXCIE3;
		static uint8_t const UDREn  = UDRE3;
		static uint8_t const U2Xn   = U2X3;
		static volatile uint8_t* UBRRnH()  { return &UBRR3H; }
		static volatile uint8_t* UBRRnL()  { return &UBRR3L; }
		static volatile uint8_t* UCSRnA()  { return &UCSR3A; }
		static volatile uint8_t* UCSRnB()  { return &UCSR3B; }
		static volatile uint8_t* UCSRnC()  { return &UCSR3C; }
		static volatile uint8_t* UDRn()    { return &UDR3; }
		static SerialBase* hardwareSerialInstance;
	};


	#endif



	typedef HardwareSerial<DescSerial0, 16> HardwareSerial0;
	#if defined(FDV_ATMEGA1280_2560)
	typedef HardwareSerial<DescSerial1, 16> HardwareSerial1;
	typedef HardwareSerial<DescSerial2, 16> HardwareSerial2;
	typedef HardwareSerial<DescSerial3, 16> HardwareSerial3;
	#endif


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



//#ifndef cout
//#  define cout serial
//#endif


}


#endif /* FDV_SERIAL_H_ */
