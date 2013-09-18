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




/*
TODO: client html (query html per ricevere dati da un sito web)
*/


#ifndef FDV_HTTPSCRIPTSERV_H_
#define FDV_HTTPSCRIPTSERV_H_



#include <string.h>
#include <stdlib.h>
#include <alloca.h>
#include <inttypes.h>
#include <avr/pgmspace.h>

#include "fdv_httpserver.h"
#include "../fdv_network/fdv_socket.h"
#include "../fdv_network/fdv_ntpclient.h"
#include "../fdv_sdlib/fdv_sdcard.h"
#include "../fdv_script/fdv_script.h"
#include "../fdv_script/fdv_scriptLibrary.h"
#include "../fdv_generic/fdv_debug.h"
#include "../fdv_generic/fdv_datetime.h"




namespace fdv
{

  struct HttpScriptLibrary : public ScriptLibrary
  {
    HttpScriptLibrary(FileSystem* fileSystem, string const& cookie = string(), char const* pageparams = NULL) :
      ScriptLibrary(fileSystem), m_cookie(cookie), m_pageParams(pageparams)
    {
    }

    template <typename RunTimeT>
    bool execFunc(char const* funcName, RunTimeT& runtime, Variant* result)
    {

      // STRING = getcookie(STRING cookiename)
      // Returns the specified cookie, if exists
      if (strcmp_P(funcName, PSTR("getcookie"))==0)
      {
        result->stringVal() = string();
        char const* p1 = strstr(m_cookie.c_str(), runtime.vars[0].value.stringVal().c_str());
        if (p1)
        {
          char const* p2 = strchr(p1, '=');
          if (p2)
          {
            char const* p3 = strpbrk_P(p2, PSTR(";\0"));
            result->stringVal() = trim(string(p2+1, p3-p2));
          }
        }
        return true;
      }

      // setcookie(STRING name, STRING value [, STRING expires = "session"])
      // Sets the specified cookie
      // expires:
      //   "session" = session
      //   "never"   = never expires
      //   date      = date ("DAY, DD-MMM-YYYY HH:MM:SS GMT", ex: "Fri, 31-Dec-2010 23:59:59 GMT"
      if (strcmp_P(funcName, PSTR("setcookie"))==0)
      {
        runtime.output->write_P(PSTR("<SCRIPT language=\"JavaScript\">document.cookie=\""));
        runtime.output->write( runtime.vars[0].value.stringVal() );
        runtime.output->write('=');
        runtime.output->write( runtime.vars[1].value.stringVal() );
        if (runtime.vars.size()==3)
        {
          if (strcmp_P(runtime.vars[2].value.stringVal().c_str(), PSTR("never"))==0)
            runtime.output->write_P(PSTR(";expires=Fri, 31-Dec-2999 23:59:59 GMT"));
          else if (strcmp_P(runtime.vars[2].value.stringVal().c_str(), PSTR("session"))!=0) // if not "session"
          {
            runtime.output->write_P(PSTR(";expires="));
            runtime.output->write( runtime.vars[2].value.stringVal() );
          }
        }
        runtime.output->write_P(PSTR("\"</SCRIPT>"));
        return true;
      }

      // STRING = getpageparam(STRING name)
      // Gets the specified page parameter (page?param1=...&param2=...etc...)
      if (strcmp_P(funcName, PSTR("getpageparam"))==0)
      {
        result->stringVal() = string();
        if (m_pageParams)
        {
          string& param = runtime.vars[0].value.stringVal();
          uint8_t len = param.size();
          char const* pos = strstr(m_pageParams, &param[0]);
          if (pos && pos[len]=='=' && pos[len+1]>31)
            result->stringVal() = string(&pos[len+1], strchrnul(&pos[len+1], '&')); // strchrnul returns pointer to \0 if char not found
        }
        return true;
      }

      // UINT8 = sntp_time(UINT8 timezone, STRING sntpServer, UINT32* outTimeStamp)
      // Returns current date/time using SNTP server and specified time zone.
      // Returns 1 on success, 0 on failure.
      // Example:
      //   now = 0;
      //   if (sntp_time(1, "169.229.70.64", &now))
      //     settime(now);
      if (strcmp_P(funcName, PSTR("sntp_time"))==0)
      {
        uint8_t timezone = runtime.vars[0].value.uint8Val();
        string& server = runtime.vars[1].value.stringVal();
        DateTime now = DateTime::now();
        Ethernet* ethernet = runtime.output->ethernet();
        result->uint8Val() = SNTPClient(ethernet, server.c_str()).query(timezone, now);
        runtime.vars[2].value.refVal()->uint32Val() = now.getUnixDateTime();
        return true;
      }

      // STRING = remoteip()
      // Returns remote IP address
      if (strcmp_P(funcName, PSTR("remoteip"))==0)
      {
        result->stringVal() = runtime.output->getRemoteIP();
        return true;
      }

      // STRING = localip()
      // Returns local IP address
      if (strcmp_P(funcName, PSTR("localip"))==0)
      {
        result->stringVal() = runtime.output->getLocalIP();
        return true;
      }



      return ScriptLibrary::execFunc(funcName, runtime, result);
    }

  private:
    string const& m_cookie;
    char const*   m_pageParams;
  };




  // note1: HTTPScriptServInfoT must define "getSDCard" static method: "static SDCard* getSDCard()
  template <typename HTTPScriptServInfoT>
  class HTTPScriptServ : public HTTPServer, public HTTPScriptServInfoT
  {

  public:

    // main constructor
    explicit HTTPScriptServ(void* userData)
      : HTTPServer(userData, false), m_fileSystem(HTTPScriptServInfoT::getSDCard())
    {
    }


    // extensions, example: "HTM;JPG;ICO;" plus final \0 automatically added
    // Compare case "insensitive"
    static bool hasExt_P(string const& page, prog_char const* extensions)
    {
      string const ext = Path::extractFilenameExtension(page.c_str());
      if (ext.size() > 0)
      {
        uint8_t count = strlen_P(extensions);
        for (uint8_t i=0; i<count; i+=4)
          if (strncasecmp_P(ext.c_str(), extensions+i, 3)==0)
            return true;
      }
      return false;
    }


    void replaceEncodings(string& page)
    {
      for (char* p = page.c_str(); *p; ++p)
      {
        if (*p == '%')
        {
          char hex[3];
          hex[0] = p[1]; hex[1] = p[2]; hex[2] = 0;
          *p = strtoul(hex, NULL, 16);  // replace '%' with the actual char
          page.erase(p+1, 2); // remove hex digits
        }
        else if (*p == '+')
          *p = ' ';
      }
    }


    // returns true if the page requires a userid/password
    bool requirePassword(string& page, string const& cookie)
    {
      return fileExists(m_fileSystem, "/HTPASSWD");
    }


    // returns true if the specified user can get the specified page
    // Checks for "htpasswd" file (no spaces allowed):
    //    user1:password1
    //    user2:password2
    //    etc...
    bool checkUser(string& page, string const& user, string const& password)
    {
      File htpasswd(m_fileSystem, "/HTPASSWD", File::MD_READ);
      while (!htpasswd.isEOF())
        if (htpasswd.readLine() == user+':'+password)
          return true;
      return false;
    }


    void commandGET(string& page, string const& cookie)
    {
      debug << "requested page: " << page << "   (size=" << (uint16_t)page.size() << ")" << ENDL;
      //cout << "mem: " << (int)availableHeap() << endl;

      Log::add_P(PSTR("HTTP request..."));
      Log::add(sck->getRemoteIP());
      Log::add(page);

      replaceEncodings(page);

      // search for page parameters. Separate page from parameters.
      char* params = strchr(page.c_str(), '?');
      if (params)
        *params++ = 0;

      if (page=="/")
        page = "/INDEX.HTM";

      File file(m_fileSystem, page.c_str(), File::MD_READ);
      if (file.isOpen())
      {
        if (hasExt_P(page, PSTR("htm;")))
        {
          // send header
          sendDefaultReplyHeader();
          sck->write_P(PSTR("\x0d\x0a"));
          // process/send page
          BufferedFileInput input(&file);
          HttpScriptLibrary library(&m_fileSystem, cookie, params);
          RunTime<Socket, HttpScriptLibrary> globalRunTime(sck, &library, NULL);
          if (!parseScript(&globalRunTime, &input))
            sck->write_P(PSTR("\n\nScript Error!"));
        }
        else if (hasExt_P(page, PSTR("css;jpg;bmp;gif;ico;mp3;wav;")))
        {
          // directly send file
          int16_t const BUFLEN = getFreeMem() / 2;
          char buffer[BUFLEN];
          int16_t len;
          while ( (len = file.read(&buffer[0], BUFLEN)) > 0 )
            sck->write(&buffer[0], len);
        }
        else
        {
          // error, unknown format
          sendDefaultReplyHeader();
          sck->write_P(PSTR("\x0d\x0aWrong request"));
        }
      }
      else
      {
        // error, file not found
        sendDefaultReplyHeader();
        sck->write_P(PSTR("\x0d\x0aNot Found"));
      }
    }


  private:

    FileSystem m_fileSystem;


  };






} // end of fdv namespace


#endif /* FDV_HTTPSCRIPTSERV_H_ */
