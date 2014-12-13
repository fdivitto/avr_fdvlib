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


#include <inttypes.h>

#include <avr/io.h>
#include <avr/interrupt.h>



#ifndef FDV_TIMERS_H_
#define FDV_TIMERS_H_


//inline void timer1Start()  __attribute__((always_inline));


// start timer 1 (no prescaler, no interrupt)
inline void timer1Start(uint16_t startPoint = 0)
{
	TIMSK1 = 0;
	TCCR1A = 0;
	TCNT1  = startPoint;
	OCR1A  = 0xFFFF;
	TIFR1 |= (1 << OCF1A);
	TCCR1B = 1 << CS10;
}


inline void timer1SetCheckPointA(uint16_t value)
{
	TIFR1 |= (1 << OCF1A);
	OCR1A = value;
}


inline void timer1WaitCheckPointA()
{
	if (TCNT1 < OCR1A)	// already expired?
		while ((TIFR1 & (1 << OCF1A)) == 0)
			;
}


inline void timer1WaitA(uint16_t value)
{
	timer1SetCheckPointA(value);
	timer1WaitCheckPointA();
}



#endif /* FDV_TIMERS_H_ */