// 2010-2012 by Fabrizio Di Vittorio (fdivitto@tiscali.it)

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
extern "C"
void atexit( void )
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
