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


namespace fdv
{



#define _SS_MAX_RX_BUFF 64 // RX buffer size



// start timer 1 (no prescaler, no interrupt)
#define TIMER1_START \
	TIMSK1 = 0; \
	TCCR1A = 0; \
	TCNT1  = 0; \
	OCR1A  = 0xFFFF; \
	TIFR1 |= (1 << OCF1A); \
	TCCR1B = 1 << CS10;
	
#define TIMER1_SETCHECKPOINT_A(value) \
	TIFR1 |= (1 << OCF1A); \
  OCR1A = value;
	
#define TIMER1_WAITCHECKPOINT_A \
	while ((TIFR1 & (1 << OCF1A)) == 0);	

#define TIMER1_WAIT(value) \
  TIMER1_SETCHECKPOINT_A(value); \
	TIMER1_WAITCHECKPOINT_A


// Only one instance at the time can be enabled to receive (to enable call listen(), to disable listen(false)). 
// output pins: everyone
// input pins:  everyone
class SoftwareSerial : public PCExtInterrupt::IExtInterruptCallable
{
private:

	char _receive_buffer[_SS_MAX_RX_BUFF];
	volatile uint8_t _receive_buffer_tail;
	volatile uint8_t _receive_buffer_head;

	Pin const* _receivePin;
	Pin const* _transmitPin;

	bool _buffer_overflow;
	
	uint16_t _symbol_ticks;
	

	// private methods
	
	//
	// The receive routine called by the interrupt handler
	// Assume interrupts disabled
	void recv()
	{
		if (_receivePin->read() == 0)
		{
			uint16_t intdelay = 256;
			bool doloop = true;
			while (doloop)
			{

				TIMER1_START;
				DEBUG_PIN.writeHigh();
				delayMicroseconds(4);
				DEBUG_PIN.writeLow();

				uint16_t t = (_symbol_ticks >> 1) - intdelay;
				TIMER1_WAIT(t);
				DEBUG_PIN.writeHigh();
				delayMicroseconds(4);
				DEBUG_PIN.writeLow();
			
				if (_receivePin->read() != 0)
					return;	// spurious start bit

				uint8_t d = 0;
				for (uint8_t i = 0; i != 8; ++i)
				{
					t += _symbol_ticks;
					TIMER1_WAIT(t);
					d |= _receivePin->read() << i;
					DEBUG_PIN.writeHigh();
					delayMicroseconds(8);
					DEBUG_PIN.writeLow();
				}

				// stop bit
				t += _symbol_ticks;
				TIMER1_WAIT(t);
				DEBUG_PIN.writeHigh();
				delayMicroseconds(4);
				DEBUG_PIN.writeLow();
			
				// reset pin change int flag
				PCIFR = (1 << PCIF0) | (1 << PCIF1) | (1 << PCIF2);

				if (_receivePin->read() == 0)
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
				TIMER1_SETCHECKPOINT_A(t);
				while ((TIFR1 & (1 << OCF1A)) == 0 && _receivePin->read() != 0)
					;
				doloop = _receivePin->read() == 0;				
				intdelay = 96;	// reduce interrupt delay
			}			
		}
	}

	
public:
	
	// public methods
	SoftwareSerial(Pin const* receivePin, Pin const* transmitPin) :
    _receive_buffer_tail(0),
    _receive_buffer_head(0),
		_buffer_overflow(false)
	{
		_transmitPin = transmitPin;
		_transmitPin->modeOutput();
		_transmitPin->writeHigh();

		_receivePin = receivePin;
		_receivePin->modeInput();
		_receivePin->writeHigh(); // pullup
	}
	
	
	~SoftwareSerial()
	{
		end();
	}
	
	
	void begin(long speed)
	{
		_symbol_ticks = F_CPU / speed;
		listen();
	}
	
	
	// This function sets the current object as the "listening" one
	void listen(bool enable = true)
	{
		PCExtInterrupt::attach(_receivePin->PCEXT_INT, NULL);
		if (enable)
		{
			ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
			{
				_buffer_overflow = false;
				_receive_buffer_head = _receive_buffer_tail = 0;
			}			
			PCExtInterrupt::attach(_receivePin->PCEXT_INT, this);
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
			TIMER1_START;
			_transmitPin->writeLow();
			uint16_t t = _symbol_ticks;
			TIMER1_WAIT(t);

			// Write each of the 8 bits
			for (uint8_t mask = 0x01; mask; mask <<= 1)
			{
				_transmitPin->write(b & mask);
				t += _symbol_ticks;
				TIMER1_WAIT(t);
			}

			// stop bit
			_transmitPin->writeHigh();
			t += _symbol_ticks;
			TIMER1_WAIT(t);
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
