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



#ifndef FDV_MQ7_H_
#define FDV_MQ7_H_



#include <stdlib.h>
#include <inttypes.h>

#include <util/atomic.h>

#include "../fdv_generic/fdv_analog.h"
#include "../fdv_generic/fdv_debug.h"


namespace fdv
{

  class MQ7 : ITaskCallable
  {

  public:

    MQ7(Pin const* heatPin, Pin const* readADCPin)
      : m_heatPin(heatPin), m_readADCPin(readADCPin), m_heatState(false), m_lowEndValue(0), m_highEndValue(0)
    {
      heatPin->modeOutput();
      heatPin->writeLow();
      TaskManager::add(90000l, NULL, this, true);
    }


    bool heatState()
    {
      ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
      {
        return m_heatState;
      }
      return false; // avoid compiler warning
    }


    uint16_t lowEndValue()
    {
      ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
      {
        return m_lowEndValue;
      }
      return 0;
    }


    uint16_t highEndValue()
    {
      ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
      {
        return m_highEndValue;
      }
      return 0;
    }


  private:

    void task(uint8_t taskIndex)
    {
      if (m_heatState == false)
      {
        // current state LOW, switch to HIGH
        m_lowEndValue = Analog::read(m_readADCPin);
        m_heatPin->writeHigh();
        TaskManager::set(taskIndex, 60000l, NULL, this, true);
        m_heatState = true;
      }
      else
      {
        // current state HIGH, switch to LOW
        m_highEndValue = Analog::read(m_readADCPin);
        m_heatPin->writeLow();
        TaskManager::set(taskIndex, 90000l, NULL, this, true);
        m_heatState = false;
      }
    }


  private:

    // pins
    Pin const* m_heatPin;
    Pin const* m_readADCPin;

    // MQ7 sensor HEAT state
    bool volatile m_heatState;

    // MQ7 reads at the end of each state
    uint16_t volatile m_lowEndValue;
    uint16_t volatile m_highEndValue;


  };



} // end of fdv namespace


#endif /* FDV_MQ7_H_ */
