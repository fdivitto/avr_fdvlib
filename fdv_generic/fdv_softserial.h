/*
SoftwareSerial.h (formerly NewSoftSerial.h) - 
Multi-instance software serial library for Arduino/Wiring
-- Interrupt-driven receive and other improvements by ladyada
   (http://ladyada.net)
-- Tuning, circular buffer, derivation from class Print/Stream,
   multi-instance support, porting to 8MHz processors,
   various optimizations, PROGMEM delay tables, inverse logic and 
   direct port writing by Mikal Hart (http://www.arduiniana.org)
-- Pin change interrupt macros by Paul Stoffregen (http://www.pjrc.com)
-- 20MHz processor support by Garrett Mace (http://www.macetech.com)
-- ATmega1280/2560 support by Brett Hagman (http://www.roguerobotics.com/)

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

The latest version of this library can always be found at
http://arduiniana.org.
*/

/*
Adapted to fdvlib by Fabrizio Di Vittorio, fdivitto@gmail.com
*/

#ifndef FDV_SOFTSERIAL_H_
#define FDV_SOFTSERIAL_H_


#include <inttypes.h>

#include "fdv_pin.h"
#include "fdv_interrupt.h"


namespace fdv
{



#define _SS_MAX_RX_BUFF 64 // RX buffer size
#ifndef GCC_VERSION
#define GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#endif

//
// Lookup table
//
typedef struct _DELAY_TABLE
{
	long baud;
	unsigned short rx_delay_centering;
	unsigned short rx_delay_intrabit;
	unsigned short rx_delay_stopbit;
	unsigned short tx_delay;
} DELAY_TABLE;


#if F_CPU == 16000000

static const DELAY_TABLE PROGMEM table[] =
{
	//  baud    rxcenter   rxintra    rxstop    tx
	{ 115200,   1,         17,        17,       12,    },
	{ 57600,    10,        37,        37,       33,    },
	{ 38400,    25,        57,        57,       54,    },
	{ 31250,    31,        70,        70,       68,    },
	{ 28800,    34,        77,        77,       74,    },
	{ 19200,    54,        117,       117,      114,   },
	{ 14400,    74,        156,       156,      153,   },
	{ 9600,     114,       236,       236,      233,   },
	{ 4800,     233,       474,       474,      471,   },
	{ 2400,     471,       950,       950,      947,   },
	{ 1200,     947,       1902,      1902,     1899,  },
	{ 600,      1902,      3804,      3804,     3800,  },
	{ 300,      3804,      7617,      7617,     7614,  },
};

const int XMIT_START_ADJUSTMENT = 5;

#elif F_CPU == 8000000

static const DELAY_TABLE table[] PROGMEM =
{
	//  baud    rxcenter    rxintra    rxstop  tx
	{ 115200,   1,          5,         5,      3,      },
	{ 57600,    1,          15,        15,     13,     },
	{ 38400,    2,          25,        26,     23,     },
	{ 31250,    7,          32,        33,     29,     },
	{ 28800,    11,         35,        35,     32,     },
	{ 19200,    20,         55,        55,     52,     },
	{ 14400,    30,         75,        75,     72,     },
	{ 9600,     50,         114,       114,    112,    },
	{ 4800,     110,        233,       233,    230,    },
	{ 2400,     229,        472,       472,    469,    },
	{ 1200,     467,        948,       948,    945,    },
	{ 600,      948,        1895,      1895,   1890,   },
	{ 300,      1895,       3805,      3805,   3802,   },
};

const int XMIT_START_ADJUSTMENT = 4;

#elif F_CPU == 20000000

// 20MHz support courtesy of the good people at macegr.com.
// Thanks, Garrett!

static const DELAY_TABLE PROGMEM table[] =
{
	//  baud    rxcenter    rxintra    rxstop  tx
	{ 115200,   3,          21,        21,     18,     },
	{ 57600,    20,         43,        43,     41,     },
	{ 38400,    37,         73,        73,     70,     },
	{ 31250,    45,         89,        89,     88,     },
	{ 28800,    46,         98,        98,     95,     },
	{ 19200,    71,         148,       148,    145,    },
	{ 14400,    96,         197,       197,    194,    },
	{ 9600,     146,        297,       297,    294,    },
	{ 4800,     296,        595,       595,    592,    },
	{ 2400,     592,        1189,      1189,   1186,   },
	{ 1200,     1187,       2379,      2379,   2376,   },
	{ 600,      2379,       4759,      4759,   4755,   },
	{ 300,      4759,       9523,      9523,   9520,   },
};

const int XMIT_START_ADJUSTMENT = 6;

#else

#error This version of SoftwareSerial supports only 20, 16 and 8MHz processors

#endif




inline void tunedDelay(uint16_t delay)
{
	uint8_t tmp=0;

	asm volatile("sbiw    %0, 0x01 \n\t"
	"ldi %1, 0xFF \n\t"
	"cpi %A0, 0xFF \n\t"
	"cpc %B0, %1 \n\t"
	"brne .-10 \n\t"
	: "+r" (delay), "+a" (tmp)
	: "0" (delay)
	);
}


/*
#define TDELAY(delay) \
{ \
	uint8_t tmp=0;    \
	asm volatile("sbiw    %0, 0x01 \n\t" \
	"ldi %1, 0xFF \n\t" \
	"cpi %A0, 0xFF \n\t" \
	"cpc %B0, %1 \n\t" \
	"brne .-10 \n\t" \
  : "+w" (delay), "+a" (tmp)\
	: "0" (delay) \
	); \
}
*/

#define TDELAY(delay) tunedDelay(delay)



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

	uint16_t _rx_delay_centering;
	uint16_t _rx_delay_intrabit;
	uint16_t _rx_delay_stopbit;
	uint16_t _tx_delay;

	bool _buffer_overflow;
	bool _inverse_logic;

	// private methods
	
	//
	// The receive routine called by the interrupt handler
	//
	void recv()
	{

		#if GCC_VERSION < 40302
		// Work-around for avr-gcc 4.3.0 OSX version bug
		// Preserve the registers that the compiler misses
		// (courtesy of Arduino forum user *etracer*)
		asm volatile(
		"push r18 \n\t"
		"push r19 \n\t"
		"push r20 \n\t"
		"push r21 \n\t"
		"push r22 \n\t"
		"push r23 \n\t"
		"push r26 \n\t"
		"push r27 \n\t"
		::);
		#endif

		uint8_t d = 0;

		ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
		{

			// If RX line is high, then we don't see any start bit
			// so interrupt is probably not for us
			if (_inverse_logic ? _receivePin->read() : !_receivePin->read())
			{
				// Wait approximately 1/2 of a bit width to "center" the sample
				TDELAY(_rx_delay_centering);

				// Read each of the 8 bits
				for (uint8_t i=0x1; i; i <<= 1)
				{
					TDELAY(_rx_delay_intrabit);
					uint8_t noti = ~i;
					if (_receivePin->read())
						d |= i;
					else // else clause added to ensure function timing is ~balanced
						d &= noti;
				}

				// skip the stop bit
				TDELAY(_rx_delay_stopbit);

				if (_inverse_logic)
					d = ~d;

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
			}
		}

		#if GCC_VERSION < 40302
		// Work-around for avr-gcc 4.3.0 OSX version bug
		// Restore the registers that the compiler misses
		asm volatile(
		"pop r27 \n\t"
		"pop r26 \n\t"
		"pop r23 \n\t"
		"pop r22 \n\t"
		"pop r21 \n\t"
		"pop r20 \n\t"
		"pop r19 \n\t"
		"pop r18 \n\t"
		::);
		#endif
	}

	
	/*uint8_t rx_pin_read()
	{
		return _receivePin->read();
	}*/
	
	/*
	void tx_pin_write(uint8_t pin_state)
	{
		_transmitPin->write(pin_state);
	}
*/

	
	void setTX(Pin const* transmitPin)
	{
		_transmitPin = transmitPin;
		_transmitPin->modeOutput();
		_transmitPin->writeHigh();
	}

	
	void setRX(Pin const* receivePin)
	{
		_receivePin = receivePin;
		_receivePin->modeInput();
		if (!_inverse_logic)
			_receivePin->writeHigh(); // pullup for normal logic!
	}
	


public:
	
	// public methods
	SoftwareSerial(Pin const* receivePin, Pin const* transmitPin, bool inverse_logic = false) :
    _receive_buffer_tail(0),
    _receive_buffer_head(0),
		_rx_delay_centering(0),
		_rx_delay_intrabit(0),
		_rx_delay_stopbit(0),
		_tx_delay(0),
		_buffer_overflow(false),
		_inverse_logic(inverse_logic)
	{
		setTX(transmitPin);
		setRX(receivePin);
	}
	
	
	~SoftwareSerial()
	{
		end();
	}
	
	
	void begin(long speed)
	{
		_rx_delay_centering = _rx_delay_intrabit = _rx_delay_stopbit = _tx_delay = 0;

		for (unsigned i=0; i<sizeof(table)/sizeof(table[0]); ++i)
		{
			long baud = pgm_read_dword(&table[i].baud);
			if (baud == speed)
			{
				_rx_delay_centering = pgm_read_word(&table[i].rx_delay_centering);
				_rx_delay_intrabit = pgm_read_word(&table[i].rx_delay_intrabit);
				_rx_delay_stopbit = pgm_read_word(&table[i].rx_delay_stopbit);
				_tx_delay = pgm_read_word(&table[i].tx_delay);
				break;
			}
		}

		TDELAY(_tx_delay); // if we were low this establishes the end

		listen();
	}
	
	
	// This function sets the current object as the "listening"
	// one
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
			if (_inverse_logic)
				_transmitPin->writeHigh();
			else
				_transmitPin->writeLow();
			uint16_t d = _tx_delay + XMIT_START_ADJUSTMENT;
			TDELAY(d);

			// Write each of the 8 bits
			if (_inverse_logic)
			{
				for (uint8_t mask = 0x01; mask; mask <<= 1)
				{
					if (b & mask) // choose bit
						_transmitPin->writeLow(); // send 1
					else
						_transmitPin->writeHigh(); // send 0
			
					TDELAY(_tx_delay);
				}

				_transmitPin->writeLow(); // restore pin to natural state
			}
			else
			{
				for (uint8_t mask = 0x01; mask; mask <<= 1)
				{
					if (b & mask) // choose bit
						_transmitPin->writeHigh(); // send 1
					else
						_transmitPin->writeLow(); // send 0
			
					TDELAY(_tx_delay);
				}

				_transmitPin->writeHigh(); // restore pin to natural state
			}

			TDELAY(_tx_delay);
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
