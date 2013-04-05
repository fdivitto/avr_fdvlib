// 2010 by Fabrizio Di Vittorio (fdivitto@tiscali.it)


#ifndef FDV_SOUND_H
#define FDV_SOUND_H

#include <stdlib.h>
#include <inttypes.h>
#include <util/atomic.h>

#include "../fdv_generic/fdv_memory.h"
#include "../fdv_generic/fdv_pin.h"
#include "../fdv_generic/fdv_datetime.h"


namespace fdv
{

  struct Sound
  {

    static void beep(Pin const* pin, uint32_t duration_ms, uint32_t freq_hz)
    {
      ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
      {
        pin->modeOutput();
        uint32_t const T = 1000000 / freq_hz;  // period in us
        uint32_t const periodsCount = duration_ms * 1000 / T;
        for (uint32_t i=0; i!=periodsCount; ++i)
        {
          pin->writeLow();
          delayMicroseconds(T/2);
          pin->writeHigh();
          delayMicroseconds(T/2);
        }
      }
    }

  };



} // end of fdv namespace



#endif  // FDV_SOUND_H
