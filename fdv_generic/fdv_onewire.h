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





#ifndef FDV_ONEWIRE2_H_
#define FDV_ONEWIRE2_H_



#include <inttypes.h>

#include "fdv_pin.h"



namespace fdv
{


  /*

  Sample code to read ROM code for each attached device:

  OneWire ow = OneWire( &PinB0 );
  while (ow.search())
  {
    for (uint8_t i=0; i<8; ++i)
      serial << (uint16_t)ow.currentROMCode()[i] << ' ';
    serial << endl;
  }

 */


  class OneWire
  {

  public:

    // Dallas family codes (first byte of ROM code)
    static uint8_t const FAMILY_DS18B20 = 0x28;
    static uint8_t const FAMILY_DS2406  = 0x12;


    OneWire(Pin const* pin);

    // Perform a 1-Wire reset cycle. Returns 1 if a device responds
    // with a presence pulse.  Returns 0 if there is no device or the
    // bus is shorted or otherwise held low for more than 250uS
    bool reset();

    // Issue a 1-Wire rom select command, you do the reset first.
    // If "rom"=NULL  a skip command is sent.
    void select(uint8_t const* rom);

    // Write a byte. If 'power' is one then the wire is held high at
    // the end for parasitically powered devices. You are responsible
    // for eventually depowering it by calling depower() or doing
    // another read or write.
    void write(uint8_t v, bool power = 0);

    // Read a byte.
    uint8_t read();

    // Write a bit. The bus is always left powered at the end, see
    // note in write() about that.
    void write_bit(uint8_t v);

    // Read a bit.
    uint8_t read_bit();

    // Stop forcing power onto the bus. You only need to do this if
    // you used the 'power' flag to write() or used a write_bit() call
    // and aren't about to do another read or write. You would rather
    // not leave this powered if you don't have to, just in case
    // someone shorts your bus.
    void depower();

    // Clear the search state so that if will start from the beginning again.
    void reset_search();

    // Look for the next device. Returns 1 if a new address has been
    // returned. A zero might mean that the bus is shorted, there are
    // no devices, or you have already retrieved all of them.  It
    // might be a good idea to check the CRC to make sure you didn't
    // get garbage.  The order is deterministic. You will always get
    // the same devices in the same order.
    bool search();

    // result of search(). Can also be used as current device rom code.
    uint8_t const* currentROMCode() const
    {
      return &m_ROMCode.bytes[0];
    }

    // same of currentROMCode() but returns a single 64 bit value
    uint64_t const& currentROMCode64() const
    {
      return m_ROMCode.value;
    }

    // Compute a Dallas Semiconductor 8 bit CRC, these are used in the
    // ROM and scratch pad registers.
    static uint8_t crc8(uint8_t const* addr, uint8_t len);

    // Compute a Dallas Semiconductor 16 bit CRC. Maybe. I don't have
    // any devices that use this so this might be wrong. I just copied
    // it from their sample code.
    static uint16_t crc16(uint16_t const* data, uint16_t len);


  private:

    Pin const* m_pin;

    // search state
    union
    {
      uint8_t  bytes[8];
      uint64_t value;
    } m_ROMCode;    
    uint8_t m_lastDiscrepancy;
    uint8_t m_lastFamilyDiscrepancy;
    bool    m_lastDeviceFlag;

  };


} // end of fdv namespace

#endif /* FDV_ONEWIRE2_H_ */
