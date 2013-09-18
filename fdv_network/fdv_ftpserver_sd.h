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




#ifndef FDV_FTPSERVER_SD_H
#define FDV_FTPSERVER_SD_H


#include <string.h>
#include <stdlib.h>
#include <alloca.h>
#include <inttypes.h>
#include <avr/pgmspace.h>


#include "../fdv_generic/fdv_string.h"
#include "../fdv_generic/fdv_algorithm.h"
#include "../fdv_generic/fdv_datetime.h"
#include "../fdv_generic/fdv_timesched.h"
#include "fdv_ethernet.h"
#include "fdv_socket.h"
#include "fdv_tcpserver.h"
#include "fdv_ftpserver.h"
#include "../fdv_sdlib/fdv_sdcard.h"


namespace fdv
{

  // note1: FTPServerInfoT must define "getSDCard" static method: "static SDCard* getSDCard()"
  // note2: FTPServerInfoT must define "authenticate" static method: "static bool authenticate(char const* user, char const* password)"

  template <typename FTPServerInfoT>
  class FTPServer_SD : public FTPServer, public FTPServerInfoT
  {
    
  public:
    
    // main constructor
    explicit FTPServer_SD(void* userData) :
      FTPServer(userData),
      m_fileSystem(FTPServerInfoT::getSDCard())
    {      
    }
    
    
    void initConnection(Socket* socket)
    {
      FTPServer::initConnection(socket);
      
      if (!m_fileSystem.available())
      {
        sck->close();
        return;
      }
      
      sck->write_P(PSTR("220---------- Welcome to FDV-FTP server ----------\r\n"
                        "220-This is a private system - No anonymous login\r\n"
                        "220 Your IP is "));  // last line has a space after "220"
      *sck << sck->getRemoteIP() << "\r\n";
    }
    

    bool authenticate(char const* user, char const* password) 
    {
      return FTPServerInfoT::authenticate(user, password);
    }
    
    
    string const getCurrentDirectory()
    {
      return m_fileSystem.getCurrentDirectory();
    }
    
    
    void setCurrentDirectory(char const* dirname)
    {
      m_fileSystem.setCurrentDirectory(dirname);
    }
    
    
    void mlsd(Socket* dataSocket, char const* dirname)
    {
      DirChanger dirChanger(m_fileSystem, dirname);
      
      // current directory
      dataSocket->write_P(PSTR("Type=cdir; ")); *dataSocket << m_fileSystem.getCurrentDirectory(); dataSocket->write_P(PSTR("\r\n"));  
      
      // parent directory
      dataSocket->write_P(PSTR("Type=pdir; ..\r\n"));        

      // other files/dirs
      FileSystem::DirectoryItem dirItem;
      m_fileSystem.beginDirectoryList();
      while (m_fileSystem.nextDirectoryItem(dirItem)) 
      {        
        if (dirItem.isDirectory)
        {
          // directory
          dataSocket->write_P(PSTR("Type=dir")); 
        }
        else
        {
          // file
          dataSocket->write_P(PSTR("Type=file;Size="));
          *dataSocket << toString(dirItem.size);
        }
        dataSocket->write_P(PSTR(";Modify="));        
        *dataSocket << dirItem.lastWriteDateTime.year
                    << padLeft(toString(dirItem.lastWriteDateTime.month), '0', 2)
                    << padLeft(toString(dirItem.lastWriteDateTime.day), '0', 2)
                    << padLeft(toString(dirItem.lastWriteDateTime.hours), '0', 2)
                    << padLeft(toString(dirItem.lastWriteDateTime.minutes), '0', 2)
                    << padLeft(toString(dirItem.lastWriteDateTime.seconds), '0', 2);        
        dataSocket->write_P(PSTR("; "));
        *dataSocket << dirItem.name;
        dataSocket->write_P(PSTR("\r\n"));
      }
    }
    
    
    void list(Socket* dataSocket, char const* dirname)
    {
      DirChanger dirChanger(m_fileSystem, dirname);
      
      FileSystem::DirectoryItem dirItem;
      m_fileSystem.beginDirectoryList();
      while (m_fileSystem.nextDirectoryItem(dirItem)) 
      {        
        *dataSocket << (dirItem.isDirectory? 'd' : '-')
                    << "rwxrwxrwx 777 admin"
                    << padLeft(toString(dirItem.size), ' ', 11) 
                    << ' ' << dirItem.lastWriteDateTime.monthStr()
                    << ' ' << padLeft(toString(dirItem.lastWriteDateTime.day), ' ', 2)            
                    << ' ' << padLeft(toString(dirItem.lastWriteDateTime.hours), '0', 2)
                    << ':' << padLeft(toString(dirItem.lastWriteDateTime.minutes), '0', 2)
                    << ' ' << dirItem.name
                    << "\r\n";
      }
    }
    
        
    void fileRead(Socket* dataSocket, char const* fullPath)
    {
      int16_t const BUFLEN = 32;
      File file(m_fileSystem, fullPath, File::MD_READ);
      if (file.isOpen())
      {
        char buffer[BUFLEN];
        int16_t len;
        while (dataSocket->connected() && (len = file.read(&buffer[0], BUFLEN)) > 0)
          dataSocket->write(&buffer[0], len);
      };
    }
    
    
    void fileWrite(Socket* dataSocket, char const* fullPath)
    {
      m_fileSystem.removeFile(fullPath);
      File file(m_fileSystem, fullPath, File::MD_CREATE | File::MD_WRITE);
      if (file.isOpen())
      {
        uint16_t const MAXBUF = min<uint16_t>(getFreeMem() / 2, 2048);
        Buffer<uint8_t> buffer(MAXBUF);
        //debug << "fileWrite sck.status = " << (uint16_t)dataSocket->status() << ENDL;
        //debug << "fileWrite dataAvail = " << dataSocket->dataAvailable()<< ENDL;
        while (dataSocket->connected())
        {
          uint16_t len = min(MAXBUF, dataSocket->dataAvailable());
          len = dataSocket->read(&buffer[0], len);
          //debug << "fileWrite  len=" << len << " sockstat=" << (uint16_t)dataSocket->status() << " dataAvail=" << dataSocket->dataAvailable() << ENDL;
          file.write(&buffer[0], len);
        }
        //debug << "fileWrite end" << ENDL;
      }
    }    
    
    
    void fileMove(char const* srcPath, char const* dstPath)
    {
      fdv::fileMove(m_fileSystem, srcPath, dstPath);
    }
    
    
    bool fileExists(char const* path)
    {
      return fdv::fileExists(m_fileSystem, path);
    }
    
    
    void fileDelete(char const* path)
    {
      m_fileSystem.removeFile(path);
    }
    
    
    void dirCreate(char const* path)
    {
      m_fileSystem.makeDirectory(path);
    }
    
    
    void dirDelete(char const* path)
    {
      m_fileSystem.removeDirectory(path);
    }
    
    
    
    
  private:
    
    FileSystem m_fileSystem;
  };
  
  
  
} // end of fdv namespace


#endif  // FDV_FTPSERVER_SD_H

