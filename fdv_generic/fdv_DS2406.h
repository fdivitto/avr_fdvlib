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




#ifndef FDV_DS2406_H_
#define FDV_DS2406_H_


#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include <avr/pgmspace.h>

#include "fdv_pin.h"
#include "fdv_timesched.h"
#include "fdv_onewire.h"



namespace fdv
{


  // uses only channel A. Don't use EPROM memory.
  // note: place a pull-up (4.7K) resistor in DATA pin
  //
  // Example 1: (list all attached devices on PC0)
  //  DS2406 m_DS2406(&PinC0);
  //  while (m_DS2406.oneWire().search())
  //    cout << (uint64_t)(m_DS2406.oneWire().currentROMCode64()) << endl;
  //
  class DS2406
  {

  public:

    explicit DS2406(Pin const* pin) :
      m_onewire(pin)
    {
    }


    // write channel A
    // deviceAddress==null can be used when only one slave is present
    bool write(bool value, uint8_t const* deviceAddress = NULL)
    {
      if (!m_onewire.reset())
        return false;
      m_onewire.select(deviceAddress);
      m_onewire.write(0xF5);        // 0xF5: Channel Access command
      m_onewire.write(0b00000100);  // Channel Control Byte 0: ALR=0, IM=0, TOG=0, IC=0, CHS1=0, CHS0=1, CRC1=0, CRC0=0
      m_onewire.write(0b11111111);  // Channel Control Byte 1: 0xFF
      /*uint8_t channelInfoByte = */m_onewire.read(); // bypass channelInfoByte
      m_onewire.write( value? 0xFF : 0x00 );
      return true;
    }

    bool write(bool value, uint64_t const& deviceAddress)
    {
      return write(value, (uint8_t const*)&deviceAddress);
    }


    // read channel A
    // deviceAddress==null can be used when only one slave is present
    bool read(bool* result, uint8_t const* deviceAddress = NULL)
    {
      if (!m_onewire.reset())
        return false;
      m_onewire.select(deviceAddress);
      m_onewire.write(0xF5);        // 0xF5: Channel Access command
      m_onewire.write(0b00000100);  // Channel Control Byte 0: ALR=0, IM=1, TOG=0, IC=0, CHS1=0, CHS0=1, CRC1=0, CRC0=0
      m_onewire.write(0b11111111);  // Channel Control Byte 1: 0xFF
      uint8_t channelInfoByte = m_onewire.read(); // read channelInfoByte
      *result = channelInfoByte & (1 << 2); // read "PIO A Sensed Level"
      return true;
    }

    bool read(bool* result, uint64_t const& deviceAddress)
    {
      return read(result, (uint8_t const*)&deviceAddress);
    }


    // One wire direct access
    OneWire& oneWire()
    {
      return m_onewire;
    }

  private:

    OneWire m_onewire;
  };




};





#endif /* FDV_DS2406_H_ */
