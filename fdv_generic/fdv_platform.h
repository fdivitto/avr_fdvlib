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



#ifndef FDV_PLATFORM_H_
#define FDV_PLATFORM_H_


#if defined (__AVR_ATmega328P__) || defined (__AVR_ATmega328__) || defined (__AVR_ATmega168__) || defined (__AVR_ATmega168P__) || defined (__AVR_ATmega88__) || defined (__AVR_ATmega88P__)
  #define FDV_ATMEGA88_328
#endif

#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
  #define FDV_ATMEGA1280_2560
#endif

#if defined(__AVR_ATtiny84__) || defined(__AVR_ATtiny84A__)
  #define FDV_ATTINY84
#endif

#if defined(__AVR_ATtiny85__)
  #define FDV_ATTINY85
#endif




#endif /* FDV_PLATFORM_H_ */
