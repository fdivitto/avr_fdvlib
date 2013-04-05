// 2010 by Fabrizio Di Vittorio (fdivitto@tiscali.it)


#ifndef FDV_INI_H_
#define FDV_INI_H_

// stdlib & avr
#include "string.h"
#include "stdlib.h"
#include "alloca.h"
#include <ctype.h>
#include <inttypes.h>
#include <avr/pgmspace.h>

#include "fdv_sdcard.h"




namespace fdv
{


  /*
   * Handle ini files
   *
   * KEY <SP> = <SP> VALUE <LF>
   * where LF='\n' (0x0A)
   *
   * Example:
   *
   * ip = 192.168.1.178
   *
   * KEY cannot contain spaces.
   *
   */
  class Ini
  {

  public:

    static uint8_t const MAX_KEY_LENGTH = 16;

    Ini(SDCard* sdcard, char const* filename) :
      m_sdcard(sdcard), m_filename(filename)
    {
    }


    // position: as input it is the first position where to look, as output it is the location of found key
    // exactMatch: if false "key" must match the first part of found key
    bool findKey(uint32_t* position, char const* key, bool exactMatch)
    {
      FileSystem fileSystem(m_sdcard);
      checkIniFile(fileSystem);
      File iniFile(fileSystem, m_filename.c_str(), File::MD_READ);
      uint8_t klen = strlen(key);
      iniFile.position(*position);
      while (!iniFile.isEOF())
      {
        *position = iniFile.position(); // store begin of line, in case it is usefull
        string const line = iniFile.readLine();
        char* sp = strchr(line.c_str(), '='); // look for '='
        uint8_t len = sp-line.c_str()-1;
        if ( (exactMatch && sp!=NULL && len==klen && strncmp(line.c_str(), key, len)==0)
             || (!exactMatch && sp!=NULL && len>=klen && strncmp(line.c_str(), key, klen)==0) )
        {
          return true; // key found
        }
      }
      return false; // not found (*position becomes undefined)
    }

    // Read key at position
    // Use with findKey
    string const readKey(uint32_t position)
    {
      FileSystem fileSystem(m_sdcard);
      File iniFile(fileSystem, m_filename.c_str(), File::MD_READ);
      iniFile.position(position);
      string const line = iniFile.readLine();
      return string(line.c_str(), strchr(line.c_str(), '=')-line.c_str()-1);
    }

    // updates position to the next item
    // Use with findKey
    string const readString(uint32_t* position)
    {
      FileSystem fileSystem(m_sdcard);
      File iniFile(fileSystem, m_filename.c_str(), File::MD_READ);
      iniFile.position(*position);
      string const line = iniFile.readLine();
      *position = iniFile.position(); // move to the next item
      return string(strchr(line.c_str(), '=')+2);
    }


    string const readString(char const* key, char const* defaultValue)
    {
      FileSystem fileSystem(m_sdcard);
      checkIniFile(fileSystem);
      File iniFile(fileSystem, m_filename.c_str(), File::MD_READ);
      uint8_t klen = strlen(key);
      while (!iniFile.isEOF())
      {
        string const line = iniFile.readLine();
        char* sp = strchr(line.c_str(), '='); // look for '='
        if (sp)
        {
          uint8_t len = sp - &line[0] - 1;
          if (len==klen && strncmp(&line[0], key, len)==0)
            return string(sp+2); // key found, return value
        }
      }
      return string(defaultValue);  // not found, return default value
    }


    string const readString_PP(PGM_P key, PGM_P defaultValue)
    {
      uint8_t lkey = strlen_P(key)+1;
      char* mkey = (char*)alloca(lkey);
      memcpy_P(mkey, key, lkey);

      uint8_t lvalue = strlen_P(defaultValue)+1;
      char* mvalue = (char*)alloca(lvalue);
      memcpy_P(mvalue, defaultValue, lvalue);

      return readString(mkey, mvalue);
    }


    uint32_t readUInt32(char const* key, uint32_t defaultValue)
    {
      return strtoul( readString(key, toString(defaultValue).c_str()).c_str(), NULL, 10 );
    }


    uint32_t readUInt32_P(PGM_P key, uint32_t defaultValue)
    {
      uint8_t lkey = strlen_P(key)+1;
      char* mkey = (char*)alloca(lkey);
      memcpy_P(mkey, key, lkey);
      return strtoul( readString(mkey, toString(defaultValue).c_str()).c_str(), NULL, 10 );
    }


    double readFloat(char const* key, float defaultValue)
    {
      return strtod( readString(key, toString(defaultValue).c_str()).c_str(), NULL );
    }


    void writeFloat(char const* key, float value)
    {
      string strval = toString(value);
      writeString(key, strval.c_str());
    }


    void writeUInt32(char const* key, uint32_t value)
    {
      string strval = toString(value);
      writeString(key, strval.c_str());
    }


    void writeString(char const* key, char const* value)
    {
      FileSystem fileSystem(m_sdcard);
      checkIniFile(fileSystem);
      File inputFile(fileSystem, m_filename.c_str(), File::MD_READ);
      string const tempFilename = getTempFilename(fileSystem);
      File outputFile(fileSystem, tempFilename.c_str(), File::MD_CREATE | File::MD_WRITE);

      uint8_t klen = strlen(key);

      bool found = false;
      while (!inputFile.isEOF())
      {
        string const line = inputFile.readLine();
        char* sp = strchr(line.c_str(), '='); // look for '='
        uint8_t len = sp-line.c_str()-1;
        if (sp!=NULL && len==klen && strncmp(line.c_str(), key, len)==0)
        {
          // found, replace key/value
          outputFile.write(key);
          outputFile.write_P(PSTR(" = "));
          outputFile.write(value);
          outputFile.write_P(PSTR("\n"));
          found = true;
        }
        else
        {
          outputFile.write(line.c_str());
          outputFile.write_P(PSTR("\n"));
        }
      }
      if (!found)
      {
        // not found, add key/value
        outputFile.write(key);
        outputFile.write_P(PSTR(" = "));
        outputFile.write(value);
        outputFile.write_P(PSTR("\n"));
      }

      inputFile.close();
      outputFile.close();
      fileSystem.removeFile(m_filename.c_str());
      fileMove(fileSystem, tempFilename.c_str(), m_filename.c_str());
    }


    void writeString_PP(PGM_P key, PGM_P value)
    {
      uint8_t lkey = strlen_P(key) + 1;
      char* mkey = (char*)alloca(lkey);
      memcpy_P(mkey, key, lkey);

      uint8_t lvalue = strlen_P(value) + 1;
      char* mvalue = (char*)alloca(lvalue);
      memcpy_P(mvalue, value, lvalue);

      writeString(mkey, mvalue);
    }


    void writeString_P(PGM_P key, char const* value)
    {
      uint8_t lkey = strlen_P(key) + 1;
      char* mkey = (char*)alloca(lkey);
      memcpy_P(mkey, key, lkey);

      writeString(mkey, value);
    }


    // removes ALL keys found
    // returns TRUE if key(s) found and removed
    bool removeKey(char const* key)
    {
      FileSystem fileSystem(m_sdcard);
      checkIniFile(fileSystem);
      File inputFile(fileSystem, m_filename.c_str(), File::MD_READ);
      string const tempFilename = getTempFilename(fileSystem);
      File outputFile(fileSystem, tempFilename.c_str(), File::MD_CREATE | File::MD_WRITE);

      bool found = false;
      while (!inputFile.isEOF())
      {
        string const line = inputFile.readLine();
        char* sp = strchr(line.c_str(), '='); // look for '='
        if (sp!=NULL && strncmp(line.c_str(), key, sp-line.c_str()-1)==0)
        {
          // found, bypass
          found = true;
        }
        else
        {
          outputFile.write(line.c_str());
          outputFile.write_P(PSTR("\n"));
        }
      }

      inputFile.close();
      outputFile.close();
      fileSystem.removeFile(m_filename.c_str());
      fileMove(fileSystem, tempFilename.c_str(), m_filename.c_str());

      return found;
    }

  private:


    void checkIniFile(FileSystem& fileSystem)
    {
      // Create ini file if not exists
      if (!fileExists(fileSystem, m_filename.c_str()))
        File iniFile(fileSystem, m_filename.c_str(), File::MD_CREATE | File::MD_WRITE);
    }

    SDCard* m_sdcard;
    string  m_filename;

  };


} // end of fdv namespace



#endif /* FDV_INI_H_ */
