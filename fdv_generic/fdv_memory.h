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



#ifndef FDV_MEMORY_H
#define FDV_MEMORY_H

#include <stdlib.h>
#include <inttypes.h>

#include "fdv_algorithm.h"
#include "fdv_debug.h"


////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////

namespace fdv
{
  uint16_t getFreeMem();
  
  inline uint16_t testMem()
  {
    uint16_t m = 1;
    while (true)
    {
      void* ptr = malloc(m);
      if (ptr == NULL)
        return m - 1;
      free(ptr);
      ++m;
    }    
  }

}

struct __freelist
{
  size_t sz;
  struct __freelist *nx;
};


extern struct __freelist *__flp;
extern uint8_t* __brkval;




inline void* operator new(size_t size)
{
  return malloc(size);
}

inline void* operator new(size_t size, void* ptr)
{  
  return ptr;
}

inline void operator delete(void *ptr)
{
  free(ptr);
}

inline void* operator new[](size_t size) 
{ 
  return malloc(size); 
} 

inline void operator delete[](void* ptr) 
{ 
  free(ptr);
}



template <typename T>
inline T* allocItems(size_t size)
{  
  T* ret = static_cast<T*>(malloc(size * sizeof(T)));
  return ret;
}

template <typename T>
inline T* reallocItems(T* ptr, size_t newSize)
{
  return static_cast<T*>(realloc(ptr, newSize*sizeof(T)));
}



////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////

// support for static local variables and virtual pure methods
__extension__ typedef int __guard __attribute__((mode (__DI__))); 
extern "C" int __cxa_guard_acquire(__guard *); 
extern "C" void __cxa_guard_release (__guard *); 
extern "C" void __cxa_guard_abort (__guard *); 
extern "C" void __cxa_pure_virtual(void); 



////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////


namespace fdv
{

  ////////////////////////////////////////////////////////////////////////////////////////
  // SimpleBuffer
  
  template <typename ItemT>
  class SimpleBuffer
  {
    ItemT* m_data;
    
  public:
  
    SimpleBuffer()
      : m_data(NULL)
    {        
    }
    
    explicit SimpleBuffer(uint16_t size)
      : m_data((ItemT*)malloc(size * sizeof(ItemT)))
    {
    }
    
    ~SimpleBuffer()
    {
      free(m_data);
    }
    
    void reset(uint16_t size)
    {
      free(m_data);
      m_data = (ItemT*)malloc(size * sizeof(ItemT));
    }
    
    ItemT* get()
    {
      return m_data;
    }
    
  };


  ////////////////////////////////////////////////////////////////////////////////////////
  // Buffer

  template <typename ItemT>
  class Buffer
  {
  private:
    Buffer(Buffer const& c);            // copy not allowed
    Buffer& operator=(Buffer const& c); // assignment not allowed

  public:

    // creates empty buffer (anyway resizeable by resize())
    Buffer()
      : m_data(NULL), m_dataSize(0)
    {
    }

    // allocates the buffer (size specified), using malloc
    explicit Buffer(uint16_t size)
      : m_data(NULL), m_dataSize(0)
    {
      if (size)
      {
        m_data     = allocItems<ItemT>(size);
        m_dataSize = size;
      }
    }

    // attaches specified buffer
    explicit Buffer(ItemT* buffer, uint16_t itemsCount)
      : m_data(buffer), m_dataSize(itemsCount)
    {
    }

    ~Buffer()
    {
      free(m_data);
    }

    void reset(uint16_t size)
    {
      Buffer<ItemT> t(size);
      swap(t);
    }

    void reset(ItemT* buffer, uint16_t itemsCount)
    {
      Buffer<ItemT> t(buffer, itemsCount);
      swap(t);
    }

    // detach buffer (does not free memory!)
    ItemT* release()
    {
      ItemT* temp = m_data;
      m_data      = NULL;
      m_dataSize  = 0;
      return temp;
    }

    ItemT* operator*()
    {
      return m_data;
    }

    ItemT const* operator*() const
    {
      return m_data;
    }

    ItemT* operator->()
    {
      return m_data;
    }

    ItemT const* operator->() const
    {
      return m_data;
    }

    ItemT& operator[](uint16_t index)
    {
      return m_data[index];
    }

    ItemT const& operator[](uint16_t index) const
    {
      return m_data[index];
    }


    uint16_t max_size() const
    {
      return getFreeMem() / sizeof(ItemT);
    }


    uint16_t size() const
    {
      return m_dataSize;
    }


    bool resize(uint16_t newSize)
    {
      if (newSize != size())
      {
        ItemT* newPtr = reallocItems<ItemT>(m_data, newSize);
        if (newPtr)
        {
          m_data = newPtr;
          m_dataSize = newSize;
        }
        return newPtr!=NULL;
      }
      return true;
    }

    bool isEmpty() const
    {
      return m_dataSize == 0;
    }

    void swap(Buffer& b)
    {
      fdv::swap(m_data, b.m_data);
      fdv::swap(m_dataSize, b.m_dataSize);
    }

    ItemT* begin()
    {
      return m_dataSize? &m_data[0] : NULL;
    }

    ItemT const* begin() const
    {
      return m_dataSize? &m_data[0] : NULL;
    }

    ItemT* end()
    {
      return m_dataSize? &m_data[size()] : NULL;
    }

    ItemT const* end() const
    {
      return m_dataSize? &m_data[size()] : NULL;
    }


  private:

    ItemT*   m_data;
    uint16_t m_dataSize;

  };



  ////////////////////////////////////////////////////////////////////////////////////////
  // EEPROMAllocator

  struct EEPROMAllocator
  {

    template <typename T>
    static T* allocate(uint16_t itemsCount)
    {
      T* ret = reinterpret_cast<T*>(s_pos);
      s_pos += sizeof(T) * itemsCount;
      return ret;
    }

  private:

    static uint8_t* s_pos;

  };


  ////////////////////////////////////////////////////////////////////////////////////////
  // EEPROMValue

  template <typename T>
  struct EEPROMValue
  {
    EEPROMValue()
    {
      m_data = EEPROMAllocator::allocate<T>(1);
    }

    explicit EEPROMValue(T const& value)
    {
      m_data = EEPROMAllocator::allocate<T>(1);
      set(value);
    }

    operator const T() const
    {
      return get();
    }

    void operator = (T const& value)
    {
      set(value);
    }

  private:

    void set(T const& value)
    {
      eeprom_update_block(&value, m_data, sizeof(T));
    }

    T const get() const
    {
      T value;
      eeprom_read_block(&value, m_data, sizeof(T));
      return value;
    }


  private:

    T* m_data;
  };
  
  
        
}  // end of namespace fdv



#endif  // FDV_MEMORY_H
