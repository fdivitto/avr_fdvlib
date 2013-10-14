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
