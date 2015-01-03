/*
# Created by Fabrizio Di Vittorio (fdivitto@gmail.com)
# Copyright (c) 2015 Fabrizio Di Vittorio.
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


#ifndef FDV_SOFTSERIAL_H_
#define FDV_SOFTSERIAL_H_


#include <inttypes.h>

#include "fdv_pin.h"
#include "fdv_interrupt.h"
#include "fdv_timers.h"
#include "fdv_serial.h"


namespace fdv
{



// Only one instance at the time can be enabled to receive (to enable call listen(), to disable listen(false)). 
// output pins: everyone
// input pins:  everyone
template <typename RXPIN_T, typename TXPIN_T, uint8_t RXBUFSIZE_V = 32>
class SoftwareSerial : public Serial<RXBUFSIZE_V>, public PCExtInterrupt::IExtInterruptCallable
{
	
	
public:
	
	using Serial<RXBUFSIZE_V>::peek;
	using Serial<RXBUFSIZE_V>::read;
	using Serial<RXBUFSIZE_V>::available;
  using Serial<RXBUFSIZE_V>::isBufferOverflow;
	using Serial<RXBUFSIZE_V>::put;
	using Serial<RXBUFSIZE_V>::flush;


	// public methods
	explicit SoftwareSerial(uint32_t baudRate) :
		Serial<RXBUFSIZE_V>()
	{		

		TXPIN_T::modeOutput();
		TXPIN_T::writeHigh();

		RXPIN_T::modeInput();
		RXPIN_T::writeHigh(); // pullup

		uint16_t const prescalers[3] = {1, 8, 64};
		uint16_t timer_prescaler = 0;
		for (uint8_t i = 0; i != sizeof(prescalers); ++i)
		{
			timer_prescaler = prescalers[i];
			m_symbolTicks = F_CPU / timer_prescaler / baudRate;
			if ((uint32_t)m_symbolTicks * 10 < 65535)
			break;
		}
		m_symbolTicksHalf = m_symbolTicks >> 1;
		m_delay1 = 97 * (F_CPU / timer_prescaler / 100000) / 100;  // 97 = 9.7us (measured from RX pin falling and the end of timer1Reset()
		m_delay2 = 23 * (F_CPU / timer_prescaler / 100000) / 100;  // 23 = 2.3us (measured from RX pin falling and the end of timer1Reset()
		timer1Setup(timer_prescaler);
		listen();
	}
	
	
	~SoftwareSerial()
	{
		listen(false);
	}
	
	
	// This function sets the current object as the "listening" one
	void listen(bool enable = true)
	{
		PCExtInterrupt::attach(RXPIN_T::PCEXT_INT, NULL);
		if (enable)
		{
			flush();
			PCExtInterrupt::attach(RXPIN_T::PCEXT_INT, this);
		}
	}
		

	void write(uint8_t b)
	{
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
		{
			// Write the start bit
			timer1Reset(0);
			TXPIN_T::writeLow();
			uint16_t t = m_symbolTicks;
			timer1WaitA(t);

			// Write each of the 8 bits
			for (uint8_t mask = 0x01; mask; mask <<= 1)
			{
				TXPIN_T::write(b & mask);
				t += m_symbolTicks;
				timer1WaitA(t);
			}

			// stop bit
			TXPIN_T::writeHigh();
			t += m_symbolTicks;
			timer1WaitA(t);
		}
	}
			

private:

	// The receive routine called by the interrupt handler
	// Assume interrupts disabled 
	void extInterrupt()
	{				
		
		if (RXPIN_T::read() == 0)
		{

			timer1Reset(m_delay1);

			while (true)
			{				

				uint16_t t = m_symbolTicksHalf;

				timer1WaitA(t);				
			
				uint8_t d = 0;
				for (uint8_t i = 0; i != 8; ++i)
				{
					t += m_symbolTicks;
					timer1WaitA(t);
					d |= RXPIN_T::read() << i;
				}

				// stop bit
				t += m_symbolTicks;
				timer1WaitA(t);
				
				// reset pin change int flag
				PCIFR = (1 << PCIF0) | (1 << PCIF1) | (1 << PCIF2);

				if (RXPIN_T::read() == 0)
					return;	// invalid stop bit
			
				put(d);
				if (isBufferOverflow())
					break;

				// wait for another start bit
				t += m_symbolTicks << 1;	// double times
				timer1SetCheckPointA(t);
				while ((TIFR1 & (1 << OCF1A)) == 0 && RXPIN_T::read() != 0)
					;
				if (RXPIN_T::read())	// is high?
					break;	// yes, no another start bit, then exit
				timer1Reset(m_delay2);
			}			
		}
	}


private:

	uint16_t m_symbolTicks;
	uint16_t m_symbolTicksHalf;
	uint16_t m_delay1;
	uint16_t m_delay2;
	
	
};


}


#endif /* FDV_SOFTSERIAL_H_ */
