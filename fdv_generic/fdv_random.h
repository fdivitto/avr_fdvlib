// 2010 by Fabrizio Di Vittorio (fdivitto@tiscali.it)


#ifndef FDV_RANDOM_H
#define FDV_RANDOM_H


#include <stdlib.h>

#include "fdv_timesched.h"


namespace fdv
{
  
  /////////////////////////////////////////////////////////////////////////////////
  // Random

  struct Random
  {

    static void reseed(uint32_t seed)
    {
      srandom(seed + 1);
    }
    
    // 0...RANDOM_MAX
    static uint32_t nextUInt32()
    {
      static bool s_init = false;
      if (s_init == false)
      {
        s_init = true;
        reseed(millis());
      }
      
      return random();
    }
    
    // in 16 bit range
    static uint16_t nextUInt16(uint16_t minVal, uint16_t maxVal)
    {
      return nextUInt32() % (maxVal - minVal + 1) + minVal;
    }
    
    static uint16_t nextUInt16()
    {
      return nextUInt32() % 65536;      
    }


  };
  
  
  
} // end of fdv namespace




#endif  // FDV_RANDOM_H
