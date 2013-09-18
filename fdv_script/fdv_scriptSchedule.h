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



#ifndef FDV_SCRIPTSCHEDULE_H_
#define FDV_SCRIPTSCHEDULE_H_


#include "../fdv_sdlib/fdv_sdcard.h"
#include "../fdv_sdlib/fdv_ini.h"
#include "../fdv_script/fdv_script.h"

#include <util/atomic.h>



namespace fdv
{

  // sizeof(ScriptTask) = 59
  struct ScriptTask
  {
    char eventKey[18];       // the same of configuration file
    char eventValue[33];     // the same of configuration file
    uint32_t last_datetime;  // last execution (unix timestamp)
    uint32_t next_datetime;  // programmed next datetime (unix timestamp)
  };


  template <typename OutputT, typename LibraryT>
  struct ScriptScheduler
  {

    static uint8_t const MAXTASKS = 10;   // 590 bytes of RAM!
    static ScriptTask    tasks[MAXTASKS];
    static uint8_t       taskCount;
    static SDCard*       sdcard;


    static void init(SDCard* sdcard_)
    {
      taskCount = 0;
      sdcard = sdcard_;
      TaskManager::add(1000, exec, NULL, false);
    }


    /*
     * Reloads programmed events from configuration file into memory
     *
     * Every seconds:
     *   KEY   = script_(name)
     *   VALUE = (scriptfilename);(seconds)
     *   Example:   KEY="script_pippo"  VALUE="pippo;2"        -> Executes file "pippo" every 2 seconds
     *
     *
     * Every date/time match:
     *   KEY   = script_(name)
     *   VALUE = (scriptfilename);DD/MM/YYYY HH:MM:SS
     *   Notes: every field can be ".." or "...." for "don't care". ".." must be placed from left to right.
     *   Valid Example:   KEY="script_pippo"  VALUE="pippo;../../.... 14:00:00"   -> Execute file "pippo" whenever time is "14:00:00"
     *   Invalid example: KEY="script_pippo"  VALUE="pippo;../../.... 14:..:.."   -> WRONG!!
     *
     * Notes:
     *   (name) = max 10 characters
     *   (scriptfilename) = max 13 characters
     *   (seconds) = max 32 bit unsigned
     *   No need to synchronize taskCount because tasks are executed out of interrupts
     */
    static void refresh(Ini& ini)
    {
      taskCount = 0;
      uint32_t pos = 0;
      while (ini.findKey(&pos, "script_", false) && taskCount<MAXTASKS)
      {
        ScriptTask& task = tasks[taskCount];
        strcpy(task.eventKey, ini.readKey(pos).c_str());
        strcpy(task.eventValue, ini.readString(&pos).c_str());
        *strchr(task.eventValue, ';') = 0;  // replaces ";" with \0
        task.last_datetime = DateTime::now().getUnixDateTime();  // TODO: whenever events are refreshed this is resetted, even the event has not fired now!
        task.next_datetime = decodeTimeToExecute(taskCount, DateTime::now());
        ++taskCount;
      }
    }

    static void refresh(char const* filename)
    {
      Ini ini(sdcard, filename);
      refresh(ini);
    }


  private:

    // the scripts executor
    static void exec(uint8_t)
    {
      bool doloop = true;
      while (doloop)
      {
        doloop = false;
        for (uint8_t i=0; i<taskCount; ++i)
        {
          //debug << "i=" << uint16_t(i) << ENDL;
          DateTime const now_dt = DateTime::now();
          uint32_t const now = now_dt.getUnixDateTime();
          if (now >= tasks[i].next_datetime)
          {
            // execute script
            //debug << "i=" << uint16_t(i) << " now=" << DateTime::now() << "  timetoexec=" << DateTime(tasks[i].next_datetime) << ENDL;
            tasks[i].last_datetime = now; // set before because tasks[i] could be invalid if script calls reloadevents()
            tasks[i].next_datetime = decodeTimeToExecute(i, now_dt);
            execScript(i);
            doloop = true;  // restart loop
          }
        }
      }
    }


    // returns unix timestamp
    static uint32_t decodeTimeToExecute(uint8_t taskIndex, DateTime const& now)
    {
      char const* t = strchr(tasks[taskIndex].eventValue, 0) +1;
      // DD/MM/YYYY HH:MM:SS
      // 0123456789012345678
      if (strlen(t)==19 && t[2]=='/' && t[5]=='/' && t[10]==' ' && t[13]==':' && t[16]==':')
      {
        // datetime specified
        uint8_t  dd = (t[0]  != '.') ? strtoul(&t[0],  NULL, 10) : now.day;
        uint8_t  mm = (t[3]  != '.') ? strtoul(&t[3],  NULL, 10) : now.month;
        uint16_t yy = (t[6]  != '.') ? strtoul(&t[6],  NULL, 10) : now.year;
        uint8_t  hh = (t[11] != '.') ? strtoul(&t[11], NULL, 10) : now.hours;
        uint8_t  mi = (t[14] != '.') ? strtoul(&t[14], NULL, 10) : now.minutes;
        uint8_t  ss = (t[17] != '.') ? strtoul(&t[17], NULL, 10) : now.seconds;
        return DateTime(dd, mm, yy, hh, mi, ss).getUnixDateTime();
      }
      else
      {
        // seconds specified
        return tasks[taskIndex].last_datetime + strtoul(&t[0], NULL, 10);
      }
    }


    static void execScript(uint8_t taskIndex)
    {
      //debug << "ScriptScheduler::execScript " << tasks[taskIndex].eventKey << ENDL;
      FileSystem fileSystem(sdcard);
      File file(fileSystem, tasks[taskIndex].eventValue, File::MD_READ);
      if (file.isOpen())
      {
        BufferedFileInput input(&file);
        LibraryT library(&fileSystem);
        RunTime<OutputT, LibraryT> globalRunTime(NULL, &library, NULL); // TODO: setting Socket=NULL will cause crash when scripts generates output
        globalRunTime.addVariable( Variable("ScriptName", Variant(tasks[taskIndex].eventKey)) );
        globalRunTime.addVariable( Variable("ScriptFileName", Variant(tasks[taskIndex].eventValue)) );
        if (!parseScript(&globalRunTime, &input))
          Log::add(string("Script error: ") + tasks[taskIndex].eventValue);
      }
      debug << "  end" << ENDL;
    }


  };



  // ScriptScheduler class storage
  template <typename OutputT, typename LibraryT> ScriptTask ScriptScheduler<OutputT, LibraryT>::tasks[MAXTASKS];
  template <typename OutputT, typename LibraryT> uint8_t    ScriptScheduler<OutputT, LibraryT>::taskCount;
  template <typename OutputT, typename LibraryT> SDCard*    ScriptScheduler<OutputT, LibraryT>::sdcard;


} // end of fdv namespace


#endif /* FDV_SCRIPTSCHEDULE_H_ */
