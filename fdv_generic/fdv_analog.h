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

#ifndef FDV_ANALOG_H_
#define FDV_ANALOG_H_

#include <inttypes.h>

#include <avr/io.h>

#include "fdv_pin.h"
#include "fdv_platform.h"


namespace fdv
{

  struct Analog
  {

    enum Reference
    {
#if defined(FDV_ATMEGA1280_2560)
      REF_EXTERNAL = 0,
      REF_VCC = 1,
      REF_INTERNAL1V1 = 2,
      REF_INTERNAL2V56 = 3,
#elif defined (FDV_ATMEGA88_328)
      REF_EXTERNAL = 0,
      REF_VCC = 1,
      REF_INTERNAL1V1 = 3,
#elif defined(FDV_ATTINY84)
      REF_VCC = 0,
      REF_EXTERNAL = 1,
      REF_INTERNAL1V1 = 2
#elif defined(FDV_ATTINY85)
      REF_VCC          = 0b000,
      REF_EXTERNAL     = 0b001,
      REF_INTERNAL1V1  = 0b010,
      REF_INTERNAL2V56 = 0b110
#else
#error please define values for other MCU
#endif
    };


    static void init()
    {
      // set A->D prescale factor
#if F_CPU == 16000000L
      // 16Mhz/128 = 125KHz
      ADCSRA |= _BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0);
#elif F_CPU == 8000000L
      // 8Mhz/64 = 125KHz
      ADCSRA |= _BV(ADPS2) | _BV(ADPS1);
      ADCSRA &= ~_BV(ADPS0);
#else
#error Please implements other F_CPU values
#endif

      // enable A->D conversions
      ADCSRA |= _BV(ADEN);
    }


    static int16_t read(Pin const* pin, Reference reference = REF_VCC)
    {
      return read(pin->ADC_PIN, reference);
    }


    static int16_t read(uint8_t pin, Reference reference = REF_VCC)
    {
      // mux
#if defined(FDV_ATMEGA1280_2560)
      if (pin >= 8)
        ADCSRB |= _BV(MUX5);
      pin -= 8;
#endif

      // reference and multiplexer (must be set at the same time)
#if defined(FDV_ATMEGA1280_2560)
      ADMUX = (reference << 6) | (pin & 0b111);
#elif defined (FDV_ATMEGA88_328)
      ADMUX = ((uint8_t)reference << 6) | (pin & 0b111);
#elif defined(FDV_ATTINY84)
      ADMUX = (reference << 6) | (pin & 0b11);
#elif defined(FDV_ATTINY85)
      ADMUX = ((reference & 0b11) << 6) | (((reference & 0b100) >> 2) << 4);  // REFS0-2, ADLAR
      ADMUX |= (pin & 0b11);  // MUX0-1
#else
#error unsupported MCU
#endif

      // start the conversion
      ADCSRA |= _BV(ADSC);

      // wait for conversion
      while (ADCSRA & _BV(ADSC));

      // read value
      uint16_t ret = ADCL;  // must read ADCL first
      ret |= ADCH << 8;
      return (int16_t)ret;
    }


  };

} // end of fdv namespace

#endif /* FDV_ANALOG_H_ */
