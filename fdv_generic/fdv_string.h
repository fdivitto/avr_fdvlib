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



#ifndef FDV_STRING_H
#define FDV_STRING_H


#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <math.h>
#include <ctype.h>

#include "fdv_algorithm.h"
#include "fdv_vector.h"
#include "fdv_memory.h"
#include "fdv_utility.h"




namespace fdv
{ 
  
  class string
  {

    static uint16_t const PREALLOCSIZE = 15;

  public:
    
    //// construct/copy/destroy
    
    string()
      : m_data(allocChars(1))
    {
      m_data[0] = 0;
    }

    
    // copy constructor
    string(string const& str, uint16_t len = 0xFFFF)
      : m_data(NULL)
    {    
      assign(str, len);
    }


    string(char const* str, uint16_t len = 0xFFFF)
      : m_data(NULL)
    {
      assign(str, len);
    }


    string(uint16_t n, char c)
      : m_data(NULL)
    {
      assign(n, c);
    }


    string(char const* first, char const* last)
      : m_data(NULL)
    {
      assign(first, last-first);
    }
    

    ~string()
    {
      freeChars(m_data);
    }
    
    

    //////////// assignment
    
    string& assign(string const& str, uint16_t len = 0xFFFF)
    {
      return assign(&str[0], len);
    }


    string& assign(char const* str, uint16_t len = 0xFFFF)
    {
      len = (len == 0xFFFF? strlen(str) : len);
      m_data = reallocChars(m_data, len+1);
      memcpy(m_data, str, len);
      m_data[len] = 0;
      return *this;
    }


    string& assign(char const* first, char const* last)
    {
      return assign(first, last-first);
    }


    string& assign(char c)
    {
      return assign(1, c);
    }


    string& assign(uint16_t n, char c)
    {
      m_data = reallocChars(m_data, n+1);
      memset(m_data, c, n);
      m_data[n] = 0;
      return *this;
    }


    string& operator= (string const& str)
    {
      return assign(str);
    }
    
    
    string& operator= (char const* str)
    {
      return assign(str);
    }
    

    string& operator= (char c)
    {
      return assign(c);
    }


    void swap(string& s)
    {
      fdv::swap(m_data, s.m_data);
    }
    

    //// capacity
    

    // warning: linear cost
    uint16_t size() const
    {
      return strlen(m_data);
    }


    void clear()
    {
      m_data = reallocChars(m_data, 1);
      m_data[0] = 0;
    }


    bool empty() const
    {
      return m_data[0] == 0;
    }


    void resize(uint16_t n, char c = 0)
    {
      uint16_t oldlen = strlen(m_data);
      m_data = reallocChars(m_data, n+1);
      if (n > oldlen)
        memset(&m_data[oldlen], c, n-oldlen);
      m_data[n] = 0;
    }    
    
    
    //// element access
    
    char* begin()
    {
      return m_data;
    }

    char const* begin() const
    {
      return m_data;
    }

    // warning, linear cost!
    char* end()
    {
      return &m_data[size()];
    }

    // warning, linear cost!
    char const* end() const
    {
      return &m_data[size()];
    }

    
    char const& operator[] (uint16_t pos) const
    {
      return m_data[pos];
    }


    char& operator[] (uint16_t pos)
    {
      return m_data[pos];
    }    
    

    char const* c_str() const
    {
      return m_data;
    }


    char* c_str()
    {
      return m_data;
    }


    string& operator+= (string const& str)
    {
      return append(str);
    }
    
    
    string& operator+= (char const* s)
    {
      return append(s);
    }
    
    
    string& operator+= (char c)
    {
      return append(c);
    }
    
    
    string& append(string const& str)
    {
      return append(&str[0]);
    }
    
    
    string& append(char const* str)
    {
      m_data = reallocChars(m_data, strlen(m_data)+strlen(str)+1);
      strcat(m_data, str);
      return *this;
    }


    string& append(char c)
    {
      push_back(c);
      return *this;
    }


    void push_back(char c)
    {
      uint16_t oldlen = strlen(m_data);
      m_data = reallocChars(m_data, oldlen+2);
      m_data[oldlen]   = c;
      m_data[oldlen+1] = 0;
    }    

    
    int compare(string const& str) const
    {
      return compare(&str[0]);
    }
    
    
    int compare(char const* s) const
    {
      return strcmp(m_data, s);
    }
    
    
    void erase(char* first, uint16_t count)
    {
      if (count > 0)
      {
        uint16_t oldlen = strlen(m_data)+1; // +1 = final zero
        memmove(first, first+count, oldlen-(first-m_data)-count);
        m_data = reallocChars(m_data, oldlen-count);
      }
    }

    // returns 0xFFFF if not found
    uint16_t find(char c, uint16_t pos = 0) const
    {
      char* p = strchr(m_data+pos, c);
      return p? (p-m_data) : 0xFFFF;
    }

  private:
      
  
    char m_buffer[PREALLOCSIZE];
    
    char* allocChars(uint16_t size)
    {
      if (size <= PREALLOCSIZE)
        return &m_buffer[0];
      else
        return (char*)malloc(size);
    }
    
    char* reallocChars(char* ptr, uint16_t size)
    {
      char* newbuf = NULL;
      if (size <= PREALLOCSIZE)
        newbuf = &m_buffer[0];        
      else
      {
        if (ptr == &m_buffer[0])
          newbuf = (char*)malloc(size);  
        else
          newbuf = (char*)realloc(ptr, size);  
      }        
      if (ptr == &m_buffer[0]) 
        memcpy(newbuf, ptr, min(size, PREALLOCSIZE));
      return newbuf;
    }
    
    void freeChars(char* ptr)
    {
      if (ptr != &m_buffer[0])
        free(ptr);
    }


  private:
    
    char*  m_data;
  };  // end of string
  
  
  
  
  inline void swap(string& lhs, string& rhs)
  {
    lhs.swap(rhs);
  }
  
  
  
  inline bool operator==(string const& lhs, string const& rhs)
  {
    return lhs.compare(rhs) == 0;
  }
  
  inline bool operator==(string const& lhs, char const* rhs)
  {
    return lhs.compare(rhs) == 0;
  }
  
  inline bool operator==(char const* lhs, string const& rhs)
  {
    return rhs.compare(lhs) == 0;
  }
  
  inline bool operator<(string const& lhs, string const& rhs)
  {
    return rhs.compare(lhs) < 0;
  }

  inline bool operator>(string const& lhs, string const& rhs)
  {
    return rhs.compare(lhs) > 0;
  }

  inline bool operator!=(string const& lhs, string const& rhs)
  {
    return rhs.compare(lhs) != 0;
  }

  inline string operator+(string const& lhs, char const* rhs)
  {
    return string(lhs).append(rhs);
  }  
  
  inline string operator+(string const& lhs, char rhs)
  {
    return string(lhs).append(rhs);
  }  
  
  inline string operator+(string const& lhs, string const& rhs)
  {
    return string(lhs).append(rhs);
  }  
  
  
  
  ////////////////////////////////////////////////////////////////////////////////////////
  // toString()
  
  inline string const toString(int16_t v)
  {
    // -65535.
    // 0123456
    char str[8];
    itoa(v, &str[0], 10);
    return string(&str[0]);
  }

  inline string const toString(uint16_t v)
  {
    char str[9];
    Utility::fmtUInt32(v, &str[0], 8);
    return string(&str[0]);
  }

  
  inline string const toString(uint32_t v)
  {
    // 4294967296
    // 1234567890
    char str[10+2];
    Utility::fmtUInt32(v, &str[0], 11);
    return string(&str[0]);
  }

  inline string const toString(uint64_t v)
  {
    // 9223372036854775807
    // 1234567890123456789
    char str[20+2];
    Utility::fmtUInt64(v, &str[0], 21);
    return string(&str[0]);
  }
  
  
  inline string const toString(double v, uint8_t precision = 3)
  {
    char str[16];
    Utility::fmtDouble(v, precision, &str[0], 15);
    return string(&str[0]);
  }
  
  
  template <typename T>
  inline string const toString(vector<T> const& v)
  {
    string ret;
    ret.push_back('(');
    for (typename vector<T>::const_iterator i = v.begin(); i != v.end(); ++i)
    {
      ret.append( toString(*i) );
      if (i != v.end()-1 )
        ret.push_back(',');
    }
    ret.push_back(')');
    return ret;
  }


  ////////////////////////////////////////////////////////////////////////////////////////

  inline string const padLeft(string const& s, char c, uint8_t count)
  {
    if (s.size() < count)
    {
      count = max(static_cast<size_t>(count), s.size());
      string result(count, c);
      char* dst = &result[count-s.size()];
      char const* src = &s[0];
      while (*src)
        *dst++ = *src++;
      return result;
    }
    else
      return s;
  }
  

  inline string const trim(string const& str)
  {
    if (str.size()==0)
      return string();
    char const* sleft = &str[0];
    while (sleft && isspace(*sleft)) ++sleft;
    char const* sright = &str[str.size()-1];
    while (isspace(*sright)) --sright;
    return string(sleft, sright);
  }


  inline uint16_t toUInt16(char const* str)
  {
    return strtoul(str, 0, 10);
  }


  // takes 2 digits of str (ie "FE")
  inline uint8_t hexToUInt8(char const* str)
  {
    char s[3];
    s[0] = str[0];
    s[1] = str[1];
    s[2] = 0;
    return strtoul(&s[0], NULL, 16);
  }
  
  
  

  
}  // end of namespace fdv




#endif  // FDV_STRING_H


