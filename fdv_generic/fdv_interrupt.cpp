/*
# Created by Fabrizio Di Vittorio (fdivitto2013@gmail.com)
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
#include <avr/pgmspace.h>

#include "fdv_interrupt.h"
#include "fdv_platform.h"
#include "fdv_timers.h"



namespace fdv
{

  ExtInterrupt::IExtInterruptCallable* ExtInterrupt::s_callableObjects[EXTERNAL_NUM_INTERRUPTS];


#if defined(FDV_ATMEGA1280_2560)

  ISR(INT0_vect)
  {
    if (ExtInterrupt::s_callableObjects[ExtInterrupt::EXTINT_INT0])
			ExtInterrupt::s_callableObjects[ExtInterrupt::EXTINT_INT0]->extInterrupt();
  }

  ISR(INT1_vect)
  {
    if (ExtInterrupt::s_callableObjects[ExtInterrupt::EXTINT_INT1])
      ExtInterrupt::s_callableObjects[ExtInterrupt::EXTINT_INT1]->extInterrupt();
  }

  ISR(INT2_vect)
  {
    if (ExtInterrupt::s_callableObjects[ExtInterrupt::EXTINT_INT2])
      ExtInterrupt::s_callableObjects[ExtInterrupt::EXTINT_INT2]->extInterrupt();
  }

  ISR(INT3_vect)
  {
    if (ExtInterrupt::s_callableObjects[ExtInterrupt::EXTINT_INT3])
      ExtInterrupt::s_callableObjects[ExtInterrupt::EXTINT_INT3]->extInterrupt();
  }

  ISR(INT4_vect)
  {
    if (ExtInterrupt::s_callableObjects[ExtInterrupt::EXTINT_INT4])
      ExtInterrupt::s_callableObjects[ExtInterrupt::EXTINT_INT4]->extInterrupt();
  }

  ISR(INT5_vect)
  {
    if (ExtInterrupt::s_callableObjects[ExtInterrupt::EXTINT_INT5])
      ExtInterrupt::s_callableObjects[ExtInterrupt::EXTINT_INT5]->extInterrupt();
  }

  ISR(INT6_vect)
  {
    if (ExtInterrupt::s_callableObjects[ExtInterrupt::EXTINT_INT6])
      ExtInterrupt::s_callableObjects[ExtInterrupt::EXTINT_INT6]->extInterrupt();
  }

  ISR(INT7_vect)
  {
    if (ExtInterrupt::s_callableObjects[ExtInterrupt::EXTINT_INT7])
      ExtInterrupt::s_callableObjects[ExtInterrupt::EXTINT_INT7]->extInterrupt();
  }

#elif defined(FDV_ATMEGA88_328)

  ISR(INT0_vect)
  {
    if (ExtInterrupt::s_callableObjects[ExtInterrupt::EXTINT_INT0])
      ExtInterrupt::s_callableObjects[ExtInterrupt::EXTINT_INT0]->extInterrupt();
  }

  ISR(INT1_vect)
  {
    if (ExtInterrupt::s_callableObjects[ExtInterrupt::EXTINT_INT1])
      ExtInterrupt::s_callableObjects[ExtInterrupt::EXTINT_INT1]->extInterrupt();
  }

#elif defined(FDV_ATTINY84)

  ISR(INT0_vect)
  {
    if(ExtInterrupt::s_callableObjects[ExtInterrupt::EXTINT_INT0])
      ExtInterrupt::s_callableObjects[ExtInterrupt::EXTINT_INT0]->extInterrupt();
  }

#elif defined(FDV_ATTINY85)

  ISR(INT0_vect)
  {
    if(ExtInterrupt::s_callableObjects[ExtInterrupt::EXTINT_INT0])
      ExtInterrupt::s_callableObjects[ExtInterrupt::EXTINT_INT0]->extInterrupt();
  }

#else
#error Unknown MCU
#endif



  PCExtInterrupt::IExtInterruptCallable* PCExtInterrupt::s_callableObjects[PCEXTERNAL_NUM_GROUPS];


#if defined(FDV_ATMEGA88_328)

  ISR(PCINT0_vect)
  {
    if (PCExtInterrupt::s_callableObjects[PCExtInterrupt::PCEXTINT_0_7])
      PCExtInterrupt::s_callableObjects[PCExtInterrupt::PCEXTINT_0_7]->extInterrupt();
  }

  ISR(PCINT1_vect)
  {
    if (PCExtInterrupt::s_callableObjects[PCExtInterrupt::PCEXTINT_8_14])
      PCExtInterrupt::s_callableObjects[PCExtInterrupt::PCEXTINT_8_14]->extInterrupt();
  }

  ISR(PCINT2_vect)
  {
    if (PCExtInterrupt::s_callableObjects[PCExtInterrupt::PCEXTINT_16_23])
      PCExtInterrupt::s_callableObjects[PCExtInterrupt::PCEXTINT_16_23]->extInterrupt();
  }

#else
#error Unknown MCU
#endif

} // end of fdv namespace
