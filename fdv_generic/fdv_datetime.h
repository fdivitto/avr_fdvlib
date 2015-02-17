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



#ifndef FDV_DATETIME_H
#define FDV_DATETIME_H

#include <inttypes.h>
#include <avr/pgmspace.h>

#include "fdv_timesched.h"
#include "fdv_string.h"



namespace fdv
{

  // Contains datetimes >= 01/01/2000
  // parts from JeeLabs and Rob Tillaart
  struct DateTime
  {

    uint8_t  seconds;
    uint8_t  minutes;
    uint8_t  hours;
    uint16_t year;
    uint8_t  month;
    uint8_t  day;


    DateTime()
      : seconds(0), minutes(0), hours(0), year(2000), month(1), day(1)
    {      
    }


    DateTime(uint8_t day_, uint8_t month_, uint16_t year_, uint8_t hours_, uint8_t minutes_, uint8_t seconds_)
      : seconds(seconds_), minutes(minutes_), hours(hours_), year(year_), month(month_), day(day_)
    {  
    }


    explicit DateTime(uint32_t unixTimeStamp)
    {
      setUnixDateTime(unixTimeStamp);
    }


    // returns month as Jan, Feb, Mar, Apr, May, Jun, Jul, Aug, Sep, Oct, Nov, Dec
    string monthStr()
    {
      char buf[4];
      memcpy_P(buf, PSTR("JanFebMarAprMayJunJulAugSepOctNovDec")+month*3, 3);
      buf[3] = 0;
      return string(buf);
    }

    // 0=sunday...6=saturday
    uint8_t dayOfWeek() const
    {
      return (date2days(year, month, day) + 6) % 7;
    }    


    DateTime& setUnixDateTime(uint32_t unixTime)
    {
      unixTime      -= SECONDS_FROM_1970_TO_2000;
      seconds        = unixTime % 60;
      unixTime      /= 60;
      minutes        = unixTime % 60;
      unixTime      /= 60;
      hours         = unixTime % 24;
      uint16_t days = unixTime / 24;
      uint8_t leap;
      for (year = 2000; ; ++year)
      {
        leap = year % 4 == 0;
        if (days < 365u + leap)
          break;
        days -= 365 + leap;
      }
      for (month = 1; ; ++month)
      {
        uint8_t daysPerMonth = daysInMonth(month - 1);
        if (leap && month == 2)
          ++daysPerMonth;
        if (days < daysPerMonth)
          break;
        days -= daysPerMonth;
      }
      day = days + 1;    
      return *this;
    }


    uint32_t getUnixDateTime() const
    {
      uint16_t days = date2days(year, month, day);
      return time2long(days, hours, minutes, seconds) + SECONDS_FROM_1970_TO_2000;
    }


    DateTime& setNTPDateTime(uint8_t const* datetimeField, uint8_t timeZone)
    {
      uint32_t t = 0;
      for (uint8_t i = 0; i < 4; ++i)
        t = t << 8 | datetimeField[i];
      float f = ((long)datetimeField[4] * 256 + datetimeField[5]) / 65535.0; 
      t -= 2208988800UL; 
      t += (timeZone * 3600L);
      if (f > 0.4) ++t;
      return setUnixDateTime(t);
    }


    // must be updated before 50 days using adjustNow()
    static DateTime now()
    {
      uint32_t currentMillis = millis();
      uint32_t locLastMillis = lastMillis();
      uint32_t diff = (currentMillis < locLastMillis) ? (0xFFFFFFFF - locLastMillis + currentMillis) : (currentMillis - locLastMillis);
      return DateTime().setUnixDateTime( lastDateTime().getUnixDateTime() + (diff / 1000) );
    }


    static void adjustNow(DateTime const& currentDateTime)
    {
      lastMillis()   = millis();
      lastDateTime() = currentDateTime;
    }


    // format:
    //    'd' : Day of the month, 2 digits with leading zeros (01..31)
    //    'j' : Day of the month without leading zeros (1..31)
    //    'w' : Numeric representation of the day of the week (0=sunday, 6=saturday)
    //    'm' : Numeric representation of a month, with leading zeros (01..12)
    //    'n' : Numeric representation of a month, without leading zeros (1..12)
    //    'Y' : A full numeric representation of a year, 4 digits (1999, 2000...)
    //    'y' : Two digits year (99, 00...)
    //    'H' : 24-hour format of an hour with leading zeros (00..23)
    //    'i' : Minutes with leading zeros (00..59)
    //    's' : Seconds, with leading zeros (00..59)
    // Example:
    //   str = toString("d/m/Y H:i:s");
    string const format(string const& format)
    {
      string outstr;
      for (uint8_t i=0; i!=format.size(); ++i)
      {
        switch (format[i])
        {
        case 'd':
          outstr.append( padLeft(toString(day), '0', 2) );
          break;
        case 'j':
          outstr.append( toString(day) );
          break;
        case 'w':
          outstr.append( toString(dayOfWeek()) );
          break;
        case 'm':
          outstr.append( padLeft(toString(month), '0', 2) );
          break;
        case 'n':
          outstr.append( toString(month) );
          break;
        case 'Y':
          outstr.append( toString(year) );
          break;
        case 'y':
          outstr.append( toString(year).begin()+2 );
          break;
        case 'H':
          outstr.append( padLeft(toString(hours), '0', 2) );
          break;
        case 'i':
          outstr.append( padLeft(toString(minutes), '0', 2) );
          break;
        case 's':
          outstr.append( padLeft(toString(seconds), '0', 2) );
          break;
        default:
          outstr.push_back(format[i]);
          break;
        }
      }
      return outstr;
    }


  private:

    static uint32_t const SECONDS_FROM_1970_TO_2000 = 946684800;


    static uint8_t daysInMonth(uint8_t month)
    {
      static uint8_t const DIMO[] PROGMEM = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
      return pgm_read_byte(&DIMO[month]);
    }


    static long time2long(uint16_t days, uint8_t h, uint8_t m, uint8_t s)
    {
      return ((days * 24L + h) * 60 + m) * 60 + s;
    }  


    static uint16_t date2days(uint16_t y, uint8_t m, uint8_t d)
    {
      if (y >= 2000)
        y -= 2000;
      uint16_t days = d;
      for (uint8_t i = 1; i < m; ++i)
        days += daysInMonth(i - 1);
      if (m > 2 && y % 4 == 0)
        ++days;
      return days + 365 * y + (y + 3) / 4 - 1;
    }


    static DateTime& lastDateTime()
    {
      static DateTime s_lastDateTime;
      return s_lastDateTime;
    }


    static uint32_t& lastMillis()
    {
      static uint32_t s_lastMillis = millis();
      return s_lastMillis;
    }


  };


  inline bool operator > (DateTime const& lhs, DateTime const& rhs)
  {
    return lhs.getUnixDateTime() > rhs.getUnixDateTime();
  }


  inline string const toString(fdv::DateTime const& dt, bool date = true, bool time = true)
  {
    // dd/mm/yyyy hh:mm:ss
    // 0123456789012345678

    char buf[20];
    uint8_t off = 0;
    if (date)
    {
      Utility::fmtUInt32(dt.day, &buf[0], 3, 2);
      Utility::fmtUInt32(dt.month, &buf[3], 3, 2);
      Utility::fmtUInt32(dt.year, &buf[6], 5, 4);  // this add final \0
      buf[2] = buf[5] = '/';
      off = 11;
    }
    if (time)
    {
      Utility::fmtUInt32(dt.hours, &buf[off+0], 3, 2);
      Utility::fmtUInt32(dt.minutes, &buf[off+3], 3, 2);
      Utility::fmtUInt32(dt.seconds, &buf[off+6], 3, 2);  // this add final \0
      buf[off+2] = buf[off+5] = ':';
    }
    if (date && time)
      buf[10] = ' ';
    return string(&buf[0]);    
  }


  template <typename StreamT>
  inline StreamT& operator<< (StreamT& stream, fdv::DateTime const& dt)
  {
    return stream << toString(dt).c_str();
  }   




} // end of "fdv" namespace

#endif // DATETIME_H
