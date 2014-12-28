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


#ifndef FDV_SOFTSERIAL_H_
#define FDV_SOFTSERIAL_H_


#include <inttypes.h>

#include "fdv_pin.h"
#include "fdv_interrupt.h"
#include "fdv_timers.h"


namespace fdv
{



#define _SS_MAX_RX_BUFF 64 // RX buffer size




// Only one instance at the time can be enabled to receive (to enable call listen(), to disable listen(false)). 
// output pins: everyone
// input pins:  everyone
template <typename RXPIN_T, typename TXPIN_T>
class SoftwareSerial : public PCExtInterrupt::IExtInterruptCallable
{
private:

	char _receive_buffer[_SS_MAX_RX_BUFF];
	volatile uint8_t _receive_buffer_tail;
	volatile uint8_t _receive_buffer_head;

	bool _buffer_overflow;
	
	uint16_t _symbol_ticks;
	uint16_t _symbol_ticks_half;
	uint16_t _delay1;
	uint16_t _delay2;
	

	// private methods
	
	//
	// The receive routine called by the interrupt handler
	// Assume interrupts disabled 
	void recv()
	{				
		
		if (RXPIN_T::read() == 0)
		{

			timer1Reset(_delay1);
		// debug
		/*TPinD4::writeHigh();
		TPinD4::writeLow();
		TPinD4::writeHigh();
		TPinD4::writeLow();*/

			while (true)
			{				

				uint16_t t = _symbol_ticks_half;

				timer1WaitA(t);				
			
				uint8_t d = 0;
				for (uint8_t i = 0; i != 8; ++i)
				{
					t += _symbol_ticks;
					timer1WaitA(t);
					d |= RXPIN_T::read() << i;
		
		// debug
		//TPinD4::writeHigh();
		//TPinD4::writeLow();
		
				}

				// stop bit
				t += _symbol_ticks;
				timer1WaitA(t);
				
				// reset pin change int flag
				PCIFR = (1 << PCIF0) | (1 << PCIF1) | (1 << PCIF2);

				if (RXPIN_T::read() == 0)
					return;	// invalid stop bit
			
				// if buffer full, set the overflow flag and return
				if ((_receive_buffer_tail + 1) % _SS_MAX_RX_BUFF != _receive_buffer_head)
				{
					// save new data in buffer: tail points to where byte goes
					_receive_buffer[_receive_buffer_tail] = d; // save new byte
					_receive_buffer_tail = (_receive_buffer_tail + 1) % _SS_MAX_RX_BUFF;
				}
				else
				{
					_buffer_overflow = true;
				}
				
				// wait for another start bit
				t += _symbol_ticks;
				timer1SetCheckPointA(t);
				while ((TIFR1 & (1 << OCF1A)) == 0 && RXPIN_T::read() != 0)
					;
				if (RXPIN_T::read())	// is high?
					break;	// yes, no another start bit, then exit
				timer1Reset(_delay2);
			}			
		}
	}

	
public:
	
	// public methods
	SoftwareSerial() :
		_receive_buffer_tail(0),
		_receive_buffer_head(0),
		_buffer_overflow(false)
	{
		TXPIN_T::modeOutput();
		TXPIN_T::writeHigh();

		RXPIN_T::modeInput();
		RXPIN_T::writeHigh(); // pullup
	}
	
	
	~SoftwareSerial()
	{
		end();
	}
	
	
	void begin(long speed)
	{
		uint16_t const prescalers[3] = {1, 8, 64};
		uint16_t timer_prescaler = 0;
		for (uint8_t i = 0; i != sizeof(prescalers); ++i)
		{
			timer_prescaler = prescalers[i];
			_symbol_ticks = F_CPU / timer_prescaler / speed;
			if ((uint32_t)_symbol_ticks * 10 < 65535)
				break;
		}
		_symbol_ticks_half = _symbol_ticks >> 1;
		_delay1 = 97 * (F_CPU / timer_prescaler / 100000) / 100;  // 97 = 9.7us (measured from RX pin falling and the end of timer1Reset()
		_delay2 = 23 * (F_CPU / timer_prescaler / 100000) / 100;  // 23 = 2.3us (measured from RX pin falling and the end of timer1Reset()
		timer1Setup(timer_prescaler);
		listen();
	}
	
	
	// This function sets the current object as the "listening" one
	void listen(bool enable = true)
	{
		PCExtInterrupt::attach(RXPIN_T::PCEXT_INT, NULL);
		if (enable)
		{
			ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
			{
				_buffer_overflow = false;
				_receive_buffer_head = _receive_buffer_tail = 0;
			}			
			PCExtInterrupt::attach(RXPIN_T::PCEXT_INT, this);
		}
	}
	
	
	void end()
	{
		listen(false);
	}
	
		
	int peek()
	{
		int r = -1;
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
		{
			// Empty buffer?
			if (_receive_buffer_head == _receive_buffer_tail)
				r = -1;
			else
				// Read from "head"
				r = _receive_buffer[_receive_buffer_head];
		}
		return r;
	}
	

	size_t write(uint8_t b)
	{
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
		{
			// Write the start bit
			timer1Reset(0);
			TXPIN_T::writeLow();
			uint16_t t = _symbol_ticks;
			timer1WaitA(t);

			// Write each of the 8 bits
			for (uint8_t mask = 0x01; mask; mask <<= 1)
			{
				TXPIN_T::write(b & mask);
		// debug
		//TPinD4::writeHigh();
		//TPinD4::writeLow();
				t += _symbol_ticks;
				timer1WaitA(t);
			}

			// stop bit
			TXPIN_T::writeHigh();
			t += _symbol_ticks;
			timer1WaitA(t);
		}
		return 1;
	}
	
	
	int read()
	{
		int r = -1;

		ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
		{
			// Empty buffer?
			if (_receive_buffer_head == _receive_buffer_tail)
				r = -1;
			else
			{
				// Read from "head"
				r = _receive_buffer[_receive_buffer_head]; // grab next byte
				_receive_buffer_head = (_receive_buffer_head + 1) % _SS_MAX_RX_BUFF;
			}
		}
		return r;
	}
	
	
	int available()
	{
		int r = 0;
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
		{
			r = (_receive_buffer_tail + _SS_MAX_RX_BUFF - _receive_buffer_head) % _SS_MAX_RX_BUFF;
		}
		return r;
	}
	

	void flush()
	{
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
		{
			_receive_buffer_head = _receive_buffer_tail = 0;			
		}
	}
	
	
	// public only for easy access by interrupt handlers
	void extInterrupt()
	{
		recv();	
	}
};


}


#endif /* FDV_SOFTSERIAL_H_ */
