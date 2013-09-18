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





#include <avr/io.h>
#include <avr/interrupt.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>


#include "fdv_platform.h"
#include "fdv_serial.h"


namespace fdv
{


  // Interrupt handlers
  #if defined(FDV_ATMEGA1280_2560)

    ISR(USART0_RX_vect)
    {
      HardwareSerial<0>::getBuffer()->put(UDR0);
    }

    ISR(USART1_RX_vect)
    {
      HardwareSerial<1>::getBuffer()->put(UDR1);
    }

    ISR(USART2_RX_vect)
    {
      HardwareSerial<2>::getBuffer()->put(UDR2);
    }

    ISR(USART3_RX_vect)
    {
      HardwareSerial<3>::getBuffer()->put(UDR3);
    }

  #else

    ISR(USART_RX_vect)
    {
      HardwareSerial<0>::getBuffer()->put(UDR0);
    }

  #endif














}
