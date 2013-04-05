// 2010 by Fabrizio Di Vittorio (fdivitto@tiscali.it)



#include "fdv_timesched.h"
#include "fdv_platform.h"

#include <avr/io.h>



namespace fdv
{
  
  // realtime support
  uint32_t volatile s_seconds       = 0;
  uint32_t s_LastSeconds            = 0;
  uint32_t volatile s_overflowCount = 0;
  uint32_t volatile s_millis        = 0;
  uint8_t volatile  s_fract         = 0;
  
  // measurePulse support
  uint8_t volatile  s_specialMeasure      = 0;  // 0 = nop,  1 = measure pulse,   2 = delay millis (delay() support)
  uint32_t volatile s_specialMeasureValue = 0; 
  
  // nested timeout support
  

  // interrupt handler
  #if defined(FDV_ATMEGA88_328) || defined(FDV_ATMEGA1280_2560)
  ISR(TIMER0_OVF_vect)
  #elif defined(FDV_ATTINY84) || defined(FDV_ATTINY85)
  ISR(TIM0_OVF_vect)
  #endif
  {
	  // inside measurePulse function?
	  if (s_specialMeasure == 1)
	  {
		  ++s_specialMeasureValue;
		  return; // do not perform other tasks
	  }
	  
    s_millis += ((16384L / (F_CPU / 1000000L)) / 1000);
    s_fract += (((16384L / (F_CPU / 1000000L)) % 1000) >> 3);
    if (s_fract >= 125)
    {
      s_fract -= 125;
      ++s_millis;
    }
    ++s_overflowCount;

    if (s_specialMeasure == 2 && s_millis >= s_specialMeasureValue)
      s_specialMeasure = 0;

    // support for seconds()
    if (s_millis % 1000 == 0) // todo: s_millis could bypass %1000 because s_fract>=125!!
      ++s_seconds;

    // execute tasks
    TaskManager::schedule(s_millis, true);
  }

  
  // timeOut support function
  void TimeOut::timeOutFunc(uint8_t taskIndex)
  {
    TaskManager::get(taskIndex).m_everyMillisecs = 0xFFFFFFFF;
  }
  
  
  // TaskManager class static storage
  Task volatile TaskManager::s_info[MAXTASKS];
  
  
}  






