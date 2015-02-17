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




#ifndef FDV_SDCARD_H
#define FDV_SDCARD_H


// stdlib & avr
#include "string.h"
#include "stdlib.h"
#include "alloca.h"
#include <inttypes.h>
#include <avr/pgmspace.h>


// fdv
#include "../fdv_generic/fdv_memory.h"
#include "../fdv_generic/fdv_string.h"
#include "../fdv_generic/fdv_algorithm.h"
#include "../fdv_generic/fdv_datetime.h"
#include "../fdv_generic/fdv_timesched.h"
#include "../fdv_generic/fdv_spi.h"
#include "../fdv_generic/fdv_random.h"


// SDFatLib
#include "SdFat.h"




namespace fdv
{


  /////////////////////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////////////////////
  // Path

  struct Path
  {
    static string const extractDirectory(char const* filename)
    {
      char const* p = strrchr(filename, '/');      
      if (p==NULL)
        p = strrchr(filename, '\\');
      if (p==NULL)  // no path, return current directory (.)
        return string(".");
      return string(filename, p+1);        
    }

    static string const extractFilename(char const* filename)
    {
      char const* p = strrchr(filename, '/');
      if (p==NULL)
        p = strrchr(filename, '\\');
      if (p==NULL)  // no path, return full filename
        return string(filename);
      return string(p+1);
    }

    // return filename extension without "."
    static string const extractFilenameExtension(char const* filename)
    {
      char const* p = strrchr(filename, '.');
      return p==NULL? string() : string(p+1);
    }

    static bool isSep(char c)
    {
      return c=='/' || c=='\\';
    }
  };



  /////////////////////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////////////////////

  class SDCard
  {

  public:

    SDCard(HardwareSPIMaster* spi) :
        m_card(spi)
        {
          TaskManager::init();  // just in case it is not already initialized
          m_init = m_card.init() && m_volume.init(&m_card);
          SdFile::dateTimeCallback(dtcallback);
        }


        SdVolume& volume() const
        {
          return m_volume;
        }


        bool available() const
        {
          return m_init;
        }


  private:

    static void dtcallback(uint16_t* date, uint16_t* time)
    {
      DateTime now = DateTime::now();
      *date = FAT_DATE(now.year, now.month, now.day);
      *time = FAT_TIME(now.hours, now.minutes, now.seconds);
    }

    mutable Sd2Card  m_card;
    mutable bool     m_init;
    mutable SdVolume m_volume;
  };


  /////////////////////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////////////////////
  // FileSystem



  class FileSystem
  {
  public:

    friend class File;


    // Changes and restores directory on destroy  
    class DirChanger
    {
    public:
      DirChanger(FileSystem& fileSystem, char const* newDir)
        : m_fileSystem(fileSystem), m_prevDir(fileSystem.getCurrentDirectory())
      {
        m_fileSystem.setCurrentDirectory(newDir);
      }

      ~DirChanger()
      {
        m_fileSystem.setCurrentDirectory(m_prevDir.c_str());
      }
    private:
      FileSystem& m_fileSystem;
      string      m_prevDir;
    };


    // used by beginDirectoryList() and nextDirectoryItem()
    struct DirectoryItem
    {
      char     name[13];    // zero terminated file/dir name
      bool     isDirectory;   // true=directory  false=file
      uint32_t size;
      DateTime lastWriteDateTime;
    };    


    FileSystem(SDCard* sdcard)
      : m_card(sdcard)
    {
      m_curDir.openRoot(&m_card->volume());
      m_curDirStr = '/';      
    }


    bool available()
    {
      return m_card->available();
    }


    // accepts relative or absolute paths
    // "/dir1/dir2" or "/dir1/dir2" or "dir1/dir2/" or "dir1/dir2" or "/" or ".." or "."
    void setCurrentDirectory(char const* dirname)
    {
      if (strncmp_P(dirname, PSTR(".."), 2)==0)
      {
        if (m_curDirStr == "/" || m_curDirStr == "\\")
          return;  // this is the root
        string newPath;
        if (Path::isSep(m_curDirStr[m_curDirStr.size()-1]))
          newPath.assign(m_curDirStr, m_curDirStr.size()-1);
        else
          newPath.assign(m_curDirStr);
        char* p = max(strrchr(newPath.c_str(), '/'), strrchr(newPath.c_str(), '\\'));
        *p = 0;
        cd(newPath.c_str());
      }
      else if (strncmp_P(dirname, PSTR("."), 1)==0)
        return; // nothing to do
      else if (Path::isSep(dirname[0]))
        cd(dirname); // absolute path
      else
      {
        // relative path
        string path = m_curDirStr
          + (Path::isSep(m_curDirStr[m_curDirStr.size()-1])?"":"/") 
          + dirname 
          + (Path::isSep(dirname[strlen(dirname)-1])?"":"/");
        cd(path.c_str());
      }
    }


  private:

    // SDCard must be already enabled
    // "/dir1/dir2/" or "/dir1/dir2"
    void cd(char const* fullpath)
    {
      char const* readPos = fullpath;

      // select root (initial '/')
      m_curDir.close();
      m_curDir.openRoot(&m_card->volume());
      ++readPos;

      while (*readPos)
      {
        char const* nextSlash = max(strchr(readPos, '/'), strchr(readPos, '\\'));      
        if (nextSlash==NULL)
          nextSlash = strchr(readPos, 0);
        string dirname(readPos, nextSlash);
        SdFile dir;
        if (dir.open(&m_curDir, &dirname[0], O_READ))
          m_curDir = dir;
        else
        {
          // error, go to root
          m_curDirStr = '/';
          m_curDir.close();
          m_curDir.openRoot(&m_card->volume());
          return;
        }
        if (*nextSlash==0)
          break;
        readPos = nextSlash+1;
      }
      m_curDirStr.assign(fullpath);
    }


  public:

    string const getCurrentDirectory()
    {
      return m_curDirStr;
    }


    // accepts relative or absolute paths
    void makeDirectory(char const* dirpath)
    {
      DirChanger dirChanger(*this, &Path::extractDirectory(dirpath)[0]);
      SdFile dir;
      dir.makeDir(&m_curDir, &Path::extractFilename(dirpath)[0]);
      dir.sync();
      m_curDir.sync();      
    }


    // accepts relative or absolute paths
    void removeFile(char const* filename)
    {
      DirChanger dirChanger(*this, &Path::extractDirectory(filename)[0]);
      SdFile::remove(&m_curDir, &Path::extractFilename(filename)[0]);
      m_curDir.sync();      
    }


    // accepts relative or absolute paths
    void removeDirectory(char const* dirpath)
    {
      DirChanger dirChanger(*this, &Path::extractDirectory(dirpath)[0]);
      SdFile dir;
      if (dir.open(&m_curDir, &Path::extractFilename(dirpath)[0], O_READ))
      {
        dir.rmDir();
        dir.sync();
        m_curDir.sync();
      }      
    }


    // applies to current directory
    void beginDirectoryList()
    {
      m_curDir.rewind();
    }


    // applies to current directory
    bool nextDirectoryItem(DirectoryItem& item)
    {
      dir_t p;
      while (m_curDir.readDir(&p) > 0 && p.name[0] != DIR_NAME_FREE) 
      {                
        // skip deleted entry and entries for . and  .. and not file/dir
        if (p.name[0] != DIR_NAME_DELETED && p.name[0] != '.' && DIR_IS_FILE_OR_SUBDIR(&p))
        {
          item.isDirectory = DIR_IS_SUBDIR(&p);
          item.size = p.fileSize;
          item.lastWriteDateTime = DateTime(FAT_DAY(p.lastWriteDate),
            FAT_MONTH(p.lastWriteDate),
            FAT_YEAR(p.lastWriteDate),
            FAT_HOUR(p.lastWriteTime),
            FAT_MINUTE(p.lastWriteTime),
            FAT_SECOND(p.lastWriteTime));

          // file/dir name
          char* w = &item.name[0];
          for (uint8_t i=0; i<11; ++i)
          {
            if (p.name[i]==' ') continue;
            if (i==8) *w++ = '.';
            *w++ = p.name[i];
          }
          *w = 0;

          return true;        
        }
      }
      return false; // no more items
    }


    SDCard* sdcard()
    {
      return m_card;
    }


  private:

    SDCard* m_card;

    // current directory
    SdFile m_curDir;
    string m_curDirStr; // as string
  };


  /////////////////////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////////////////////
  // DirChanger

  typedef FileSystem::DirChanger DirChanger;



  /////////////////////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////////////////////
  // File

  class File
  {

  public:

    static uint8_t const MD_READ   = O_READ;
    static uint8_t const MD_WRITE  = O_WRITE;
    static uint8_t const MD_APPEND = O_APPEND;
    static uint8_t const MD_CREATE = O_CREAT;
    static uint8_t const MD_EXCL   = O_EXCL;
    static uint8_t const MD_TRUNC  = O_TRUNC;


    enum Seek
    {
      SK_SET,
      SK_END,
      SK_CUR
    };    


    // filename accepts relative or absolute path
    File(FileSystem& fileSystem, char const* filename, uint8_t mode)
    {
      DirChanger dirChanger(fileSystem, Path::extractDirectory(filename).c_str());
      m_file.open(&fileSystem.m_curDir, Path::extractFilename(filename).c_str(), mode);
    }


    ~File()
    {
      close();
    }


    void close()
    {
      m_file.close();
    }


    uint32_t position()
    {
      return m_file.curPosition();
    }


    void position(uint32_t newPos)
    {
      m_file.seekSet(newPos); // don't use seek(), to allow unsigned 32 bit position
    }


    uint32_t seek(int32_t offset, Seek origin)
    {
      switch (origin)
      {
      case SK_SET:
        m_file.seekSet(offset);
        break;
      case SK_CUR:
        m_file.seekCur(offset);
        break;
      case SK_END:
        m_file.seekEnd();
        break;
      }
      return m_file.curPosition();
    }


    uint32_t size()
    {
      return m_file.fileSize();
    }


    void truncate(uint32_t newSize)
    {
      m_file.truncate(newSize);
    }


    bool isOpen()
    {
      return m_file.isOpen();
    }


    bool isEOF()
    {
      return position() >= size();
    }


    uint16_t read(void* buf, uint16_t nbyte)
    {
      return m_file.read(buf, nbyte);
    }


    string const readLine()
    {
      string out;
      while (!isEOF())
      {
        char c;
        read(&c, 1);
        if (c==0x0A)  // LF
          break;
        if (c==0x0D)  // CR+LF
        {
          read(&c, 1);  // bypass LF
          break;
        }
        out.push_back(c);
      }
      return out;
    }


    uint16_t write(const void* buf, uint16_t nbyte)
    {
      return m_file.write(buf, nbyte);
    }


    void write(const char* str)
    {
      m_file.write(str);
    }


    void write_P(PGM_P str)
    {
      m_file.write_P(str);
    }


  private:

    SdFile m_file;

  };


  /////////////////////////////////////////////////////////////////////////
  // fileCopy
  // accepts absolute and relative paths
  inline bool fileCopy(FileSystem& fileSystem, char const* sourcePath, char const* destPath)
  {
    uint16_t const BUFFERSIZE = getFreeMem() / 2;
    Buffer<uint8_t> buffer(BUFFERSIZE);

    File srcFile(fileSystem, sourcePath, File::MD_READ);
    File dstFile(fileSystem, destPath, File::MD_WRITE | File::MD_CREATE);

    if (srcFile.isOpen() && dstFile.isOpen())
    {
      uint16_t len;
      while ( (len=srcFile.read(&buffer[0], BUFFERSIZE)) > 0 )
        dstFile.write(&buffer[0], len);
      return true;
    }
    return false;
  }


  /////////////////////////////////////////////////////////////////////////
  // fileMove
  // accepts absolute and relative paths
  // Warning: makes a copy of the original file then removes it (unfortunately SdLib hasn't rename/move methods)
  inline void fileMove(FileSystem& fileSystem, char const* sourcePath, char const* destPath)
  {
    if (fileCopy(fileSystem, sourcePath, destPath))
      fileSystem.removeFile(sourcePath);
  }


  /////////////////////////////////////////////////////////////////////////
  // fileExists
  // accepts absolute and relative paths
  // Works with files and directories
  inline bool fileExists(FileSystem& fileSystem, char const* path)
  {
    File file(fileSystem, path, File::MD_READ);
    return file.isOpen();
  }


  /////////////////////////////////////////////////////////////////////////
  // getTempFilename
  inline string const getTempFilename(FileSystem& fileSystem)
  {
    char buf[13];
    strcpy_P(&buf[0], PSTR("01234567.TMP"));
    do
    {
      for (uint8_t i=0; i!=8; ++i)
        buf[i] = Random::nextUInt16(65, 90);
    } while( fileExists(fileSystem, &buf[0]) );
    return string(&buf[0]);
  }


} // end of fdv namespace


#endif  // FDV_SDCARD_H

