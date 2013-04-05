/*
 Copyright (c) 2007, Jim Studt


 Version 2.0: Modifications by Paul Stoffregen, January 2010:
 http://www.pjrc.com/teensy/td_libs_OneWire.html
 Search fix from Robin James
 http://www.arduino.cc/cgi-bin/yabb2/YaBB.pl?num=1238032295/27#27
 Use direct optimized I/O in all cases
 Disable interrupts during timing critical sections
 (this solves many random communication errors)
 Disable interrupts during read-modify-write I/O
 Reduce RAM consumption by eliminating unnecessary
 variables and trimming many to 8 bits
 Optimize both crc8 - table version moved to flash

 Modified to work with larger numbers of devices - avoids loop.
 Tested in Arduino 11 alpha with 12 sensors.
 26 Sept 2008 -- Robin James
 http://www.arduino.cc/cgi-bin/yabb2/YaBB.pl?num=1238032295/27#27

 Updated to work with arduino-0008 and to include skip() as of
 2007/07/06. --RJL20

 Modified to calculate the 8-bit CRC directly, avoiding the need for
 the 256-byte lookup table to be loaded in RAM.  Tested in arduino-0010
 -- Tom Pollard, Jan 23, 2008

 Permission is hereby granted, free of charge, to any person obtaining
 a copy of this software and associated documentation files (the
 "Software"), to deal in the Software without restriction, including
 without limitation the rights to use, copy, modify, merge, publish,
 distribute, sublicense, and/or sell copies of the Software, and to
 permit persons to whom the Software is furnished to do so, subject to
 the following conditions:

 The above copyright notice and this permission notice shall be
 included in all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 Much of the code was inspired by Derek Yerger's code, though I don't
 think much of that remains.  In any event that was..
 (copyleft) 2006 by Derek Yerger - Free to distribute freely.

 The CRC code was excerpted and inspired by the Dallas Semiconductor
 sample code bearing this copyright.
 //---------------------------------------------------------------------------
 // Copyright (C) 2000 Dallas Semiconductor Corporation, All Rights Reserved.
 //
 // Permission is hereby granted, free of charge, to any person obtaining a
 // copy of this software and associated documentation files (the "Software"),
 // to deal in the Software without restriction, including without limitation
 // the rights to use, copy, modify, merge, publish, distribute, sublicense,
 // and/or sell copies of the Software, and to permit persons to whom the
 // Software is furnished to do so, subject to the following conditions:
 //
 // The above copyright notice and this permission notice shall be included
 // in all copies or substantial portions of the Software.
 //
 // THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 // OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 // MERCHANTABILITY,  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 // IN NO EVENT SHALL DALLAS SEMICONDUCTOR BE LIABLE FOR ANY CLAIM, DAMAGES
 // OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 // ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 // OTHER DEALINGS IN THE SOFTWARE.
 //
 // Except as contained in this notice, the name of Dallas Semiconductor
 // shall not be used except as stated in the Dallas Semiconductor
 // Branding Policy.
 //--------------------------------------------------------------------------
 //
 // modified by Fabrizio Di Vittorio (fdivitto@tiscali.it)
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/atomic.h>

#include "fdv_onewire.h"
#include "fdv_pin.h"
#include "fdv_timesched.h"


namespace fdv
{

  OneWire::OneWire(Pin const* pin) :
    m_pin(pin)
  {
    reset_search();
  }


  // Perform the onewire reset function.  We will wait up to 250uS for
  // the bus to come high, if it doesn't then it is broken or shorted
  // and we return a 0;
  //
  // Returns 1 if a device asserted a presence pulse, 0 otherwise.
  //
  bool OneWire::reset()
  {
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
      m_pin->modeInput();
      // wait until the wire is high... just in case
      uint8_t retries = 125;
      do
      {
        if (--retries == 0)
          return false;
        delayMicroseconds(2);
      } while (!m_pin->read());

      m_pin->writeLow();
      m_pin->modeOutput(); // drive output low
      delayMicroseconds(500);
      m_pin->modeInput(); // allow it to float
      delayMicroseconds(80);
      uint8_t r = m_pin->read();
      delayMicroseconds(420);
      return r == 0;
    }
    return false;
  }


  //
  // Write a bit. Port and bit is used to cut lookup time and provide
  // more certain timing.
  //
  void OneWire::write_bit(uint8_t v)
  {
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
      if (v & 1)
      {
        m_pin->writeLow();
        m_pin->modeOutput(); // drive output low
        delayMicroseconds(10);
        m_pin->writeHigh(); // drive output high
        delayMicroseconds(55);
      }
      else
      {
        m_pin->writeLow();
        m_pin->modeOutput(); // drive output low
        delayMicroseconds(65);
        m_pin->writeHigh(); // drive output high
        delayMicroseconds(5);
      }
    }
  }


  //
  // Read a bit. Port and bit is used to cut lookup time and provide
  // more certain timing.
  //
  uint8_t OneWire::read_bit()
  {
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
      m_pin->modeOutput();
      m_pin->writeLow();
      delayMicroseconds(3);
      m_pin->modeInput(); // let pin float, pull up will raise
      delayMicroseconds(9);
      uint8_t r = m_pin->read();
      delayMicroseconds(53);
      return r;
    }
    return 0;
  }


  //
  // Write a byte. The writing code uses the active drivers to raise the
  // pin high, if you need power after the write (e.g. DS18S20 in
  // parasite power mode) then set 'power' to 1, otherwise the pin will
  // go tri-state at the end of the write to avoid heating in a short or
  // other mishap.
  //
  void OneWire::write(uint8_t v, bool power)
  {
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
      for (uint8_t bitMask = 0x01; bitMask; bitMask <<= 1)
        write_bit((bitMask & v) ? 1 : 0);
      if (!power)
      {
        m_pin->modeInput();
        m_pin->writeLow();
      }
    }
  }


  //
  // Read a byte
  //
  uint8_t OneWire::read()
  {
    uint8_t r = 0;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
      for (uint8_t bitMask = 0x01; bitMask; bitMask <<= 1)
        if (OneWire::read_bit())
          r |= bitMask;
    }
    return r;
  }


  //
  // Do a ROM select (deviceAddress=8 bytes rom) or Skip (if deviceAddress==NULL)
  //
  void OneWire::select(uint8_t const* deviceAddress)
  {
    if (deviceAddress==NULL)
    {
      write(0xCC); // Skip ROM
    }
    else
    {
      write(0x55); // Choose ROM
      for (uint8_t i = 0; i < 8; ++i)
        write(deviceAddress[i]);
    }
  }


  void OneWire::depower()
  {
    m_pin->modeInput();
  }


  //
  // You need to use this function to start a search again from the beginning.
  // You do not need to do it for the first search, though you could.
  //
  void OneWire::reset_search()
  {
    // reset the search state
    m_lastDiscrepancy = 0;
    m_lastDeviceFlag = false;
    m_lastFamilyDiscrepancy = 0;
    m_ROMCode.value = 0;
  }


  //
  // Perform a search. If this function returns a '1' then it has
  // enumerated the next device and you may retrieve the ROM from the
  // OneWire::address variable. If there are no devices, no further
  // devices, or something horrible happens in the middle of the
  // enumeration then a 0 is returned.  If a new device is found then
  // its address is copied to newAddr.  Use OneWire::reset_search() to
  // start over.
  //
  // --- Replaced by the one from the Dallas Semiconductor web site ---
  //--------------------------------------------------------------------------
  // Perform the 1-Wire Search Algorithm on the 1-Wire bus using the existing
  // search state.
  // Return TRUE  : device found, ROM number in ROM_NO buffer
  //        FALSE : device not found, end of search
  //
  bool OneWire::search()
  {
    // initialize for search
    uint8_t id_bit_number = 1;
    uint8_t last_zero = 0;
    uint8_t rom_byte_number = 0;
    uint8_t rom_byte_mask = 1;
    bool search_result = false;

    // if the last call was not the last one
    if (!m_lastDeviceFlag)
    {
      // 1-Wire reset
      if (!reset())
      {
        // reset the search
        m_lastDiscrepancy = 0;
        m_lastDeviceFlag = false;
        m_lastFamilyDiscrepancy = 0;
        return false;
      }

      // issue the search command
      write(0xF0);

      // loop to do the search
      ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
      {

        do
        {
          // read a bit and its complement
          uint8_t id_bit = read_bit();
          uint8_t cmp_id_bit = read_bit();

          // check for no devices on 1-wire
          if ((id_bit == 1) && (cmp_id_bit == 1))
            break;
          else
          {
            uint8_t search_direction;
            // all devices coupled have 0 or 1
            if (id_bit != cmp_id_bit)
              search_direction = id_bit; // bit write value for search
            else
            {
              // if this discrepancy if before the Last Discrepancy
              // on a previous next then pick the same as last time
              if (id_bit_number < m_lastDiscrepancy)
                search_direction = ((m_ROMCode.bytes[rom_byte_number] & rom_byte_mask) > 0);
              else
                // if equal to last pick 1, if not then pick 0
                search_direction = (id_bit_number == m_lastDiscrepancy);

              // if 0 was picked then record its position in LastZero
              if (search_direction == 0)
              {
                last_zero = id_bit_number;

                // check for Last discrepancy in family
                if (last_zero < 9)
                  m_lastFamilyDiscrepancy = last_zero;
              }
            }

            // set or clear the bit in the ROM byte rom_byte_number
            // with mask rom_byte_mask
            if (search_direction == 1)
              m_ROMCode.bytes[rom_byte_number] |= rom_byte_mask;
            else
              m_ROMCode.bytes[rom_byte_number] &= ~rom_byte_mask;

            // serial number search direction write bit
            write_bit(search_direction);

            // increment the byte counter id_bit_number
            // and shift the mask rom_byte_mask
            id_bit_number++;
            rom_byte_mask <<= 1;

            // if the mask is 0 then go to new SerialNum byte rom_byte_number and reset mask
            if (rom_byte_mask == 0)
            {
              rom_byte_number++;
              rom_byte_mask = 1;
            }
          }
        } while (rom_byte_number < 8); // loop until through all ROM bytes 0-7

      } // end of ATOMIC_BLOCK(ATOMIC_RESTORESTATE)


      // if the search was successful then
      if (!(id_bit_number < 65))
      {
        // search successful so set LastDiscrepancy,LastDeviceFlag,search_result
        m_lastDiscrepancy = last_zero;

        // check for last device
        if (m_lastDiscrepancy == 0)
          m_lastDeviceFlag = true;

        search_result = true;
      }
    }

    // if no device found then reset counters so next 'search' will be like a first
    if (!search_result || !m_ROMCode.bytes[0])
    {
      m_lastDiscrepancy = 0;
      m_lastDeviceFlag = false;
      m_lastFamilyDiscrepancy = 0;
      search_result = false;
    }
    return search_result;
  }



  //
  // Compute a Dallas Semiconductor 8 bit CRC directly.
  //
  uint8_t OneWire::crc8(uint8_t const* addr, uint8_t len)
  {
    uint8_t crc = 0;

    while (len--)
    {
      uint8_t inbyte = *addr++;
      for (uint8_t i = 8; i; i--)
      {
        uint8_t mix = (crc ^ inbyte) & 0x01;
        crc >>= 1;
        if (mix)
          crc ^= 0x8C;
        inbyte >>= 1;
      }
    }
    return crc;
  }


  static uint8_t const OneWire_oddparity[16] PROGMEM =  { 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0 };

  //
  // Compute a Dallas Semiconductor 16 bit CRC. I have never seen one of
  // these, but here it is.
  //
  uint16_t OneWire::crc16(uint16_t const* data, uint16_t len)
  {

    uint16_t crc = 0;

    for (uint16_t i = 0; i < len; i++)
    {
      uint16_t cdata = data[len];

      cdata = (cdata ^ (crc & 0xff)) & 0xff;
      crc >>= 8;

      if (pgm_read_byte(&OneWire_oddparity[cdata & 0xf]) ^ pgm_read_byte(&OneWire_oddparity[cdata >> 4]))
        crc ^= 0xc001;

      cdata <<= 6;
      crc ^= cdata;
      cdata <<= 1;
      crc ^= cdata;
    }
    return crc;
  }



} // end of fdv namespace
