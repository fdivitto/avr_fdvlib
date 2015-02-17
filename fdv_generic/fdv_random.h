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

  private:
    static bool init(bool doInit = false)
    {
      static bool s_init = false;
      if (doInit)
        s_init = true;
      return s_init;
    }

  public:
    static void reseed(uint32_t seed)
    {
      init(true);
      srandom(seed + 1);
    }

    // 0...RANDOM_MAX
    static uint32_t nextUInt32()
    {      
      if (!init())
        reseed(millis());      
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
