// 2011 by Fabrizio Di Vittorio (fdivitto@tiscali.it)


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
