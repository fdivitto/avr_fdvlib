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




#ifndef FDV_LOG_H_
#define FDV_LOG_H_


#include <string.h>
#include <stdlib.h>
#include <alloca.h>
#include <inttypes.h>
#include <avr/pgmspace.h>

#include "../fdv_sdlib/fdv_sdcard.h"


namespace fdv
{

  struct Log
  {

    static void init(SDCard* sdcard_)
    {
      sdcard = sdcard_;
    }


    static void add(char const* msg)
    {
      //debug << toString(DateTime::now()) << " : " << msg << ENDL;
      if (sdcard)
      {
        FileSystem fileSystem(sdcard);
        if (fileSystem.available())
        {
          string const filename = DateTime::now().format("ymd.LOG");
          File logFile(fileSystem, filename.c_str(), File::MD_CREATE | File::MD_APPEND | File::MD_WRITE);
          logFile.write(&toString(DateTime::now())[0]);
          logFile.write_P(PSTR(" : "));
          logFile.write(msg);
          logFile.write_P(PSTR("\n"));
        }
      }
    }


    static void add_P(prog_char const* msg)
    {
      uint16_t l = strlen_P(msg);
      char* xmsg = (char*)alloca(l+1);
      strcpy_P(xmsg, msg);
      add(xmsg);
    }


    static void add(string const& str)
    {
      add(str.c_str());
    }


  private:

    static SDCard* sdcard;

  };


  // static storage
  SDCard* Log::sdcard = NULL;


}


#endif /* FDV_LOG_H_ */
