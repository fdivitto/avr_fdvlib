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




#ifndef FDV_DS1307_H_
#define FDV_DS1307_H_


#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include <avr/interrupt.h>


#include "fdv_twowire.h"
#include "fdv_string.h"
#include "fdv_datetime.h"



namespace fdv
{



  ////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////
  // Class to control DS1307 (i2c)
  //
  // Uses i2c connection (analog-in 4->SDA 5->SCL)
  // Delay 500ms before send the first command to allow LCD to startup

  class DS1307
  {

  public:


    DS1307(uint8_t address = 0x68)
      : m_address(address)
    {
    }


    // 1) Sets the date and time on the ds1307
    // 2) Starts the clock
    // 3) Sets hour mode to 24 hour clock
    void setDateTime(DateTime const& dt)
    {
      uint8_t buffer[8];
      buffer[0] = 0;
      buffer[1] = decToBcd(dt.seconds);         // 0-59
      buffer[2] = decToBcd(dt.minutes);         // 0-59
      buffer[3] = decToBcd(dt.hours);           // 0-23
      buffer[4] = decToBcd(dt.dayOfWeek()+1);   // 1-7 (1=sunday)
      buffer[5] = decToBcd(dt.day);             // 1-28/29/30/31
      buffer[6] = decToBcd(dt.month);           // 1-12
      buffer[7] = decToBcd(dt.year-2000);       // 0-99
      m_I2C.writeTo(m_address, &buffer[0], 8);
    }


    // Gets the date and time from the ds1307
    DateTime getDateTime()
    {
      uint8_t buffer[7];
      // Reset the register pointer
      buffer[0] = 0;
      m_I2C.writeTo(m_address, &buffer[0], 1);
      // request 7 bytes
      m_I2C.readFrom(m_address, &buffer[0], 7);
      // build DateTime
      return DateTime(bcdToDec(buffer[4]), bcdToDec(buffer[5]), 2000+bcdToDec(buffer[6]), bcdToDec(buffer[2] & 0x3f), bcdToDec(buffer[1]), bcdToDec(buffer[0] & 0x7f));
    }


  private:

    uint8_t m_address;
    char m_timeStr[9];
    char m_dateStr[11];
    TwoWire m_I2C;

    uint8_t decToBcd(uint8_t decValue)
    {
      return (decValue / 10 << 4) + decValue % 10;
    }

    uint8_t bcdToDec(uint8_t bcdValue)
    {
      return (bcdValue >> 4) * 10 +  bcdValue % 16;
    }
  };




}




#endif /* FDV_DS1307_H_ */
