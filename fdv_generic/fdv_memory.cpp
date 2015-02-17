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




#include <avr/io.h>
#include <stdlib.h>
#include <alloca.h>
#include <string.h>

#include "fdv_memory.h"
#include "fdv_debug.h"


////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////


int __cxa_guard_acquire(__guard* g) 
{
  return !*(char *)(g);
}; 


void __cxa_guard_release(__guard* g) 
{
  *(char *)g = 1;
}; 


void __cxa_guard_abort (__guard* ) 
{
};


void __cxa_pure_virtual(void) 
{
}; 


///   MCU would never "exit", so atexit can be dummy.
extern "C" void atexit( void )
{
}





////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////


namespace fdv
{

  // TODO: count free blocks also
  uint16_t getFreeMem()
  {
    uint16_t brkval = (uint16_t)__brkval;
    return (uint16_t)(AVR_STACK_POINTER_REG) - (uint16_t)__malloc_margin - (brkval == 0? (uint16_t)__malloc_heap_start : brkval);
  }



  //////////////////////////////////////////////////////////////////////////
  // EEPROMAllocator static storage

  uint8_t* EEPROMAllocator::s_pos = 0;

}
