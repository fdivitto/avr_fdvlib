// 2010 by Fabrizio Di Vittorio (fdivitto@tiscali.it)
// filever: 1001

#ifndef FDV_VECTOR_H
#define FDV_VECTOR_H

#include <inttypes.h>
#include <avr/eeprom.h>


#include "fdv_algorithm.h"
#include "fdv_memory.h"
#include "fdv_functional.h"
#include "fdv_ctrandom.h"

namespace fdv
{
 

  ///////////////////////////////////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////////////////////////////////

  // a circular buffer of fixed size (max 255 items)
  template <typename ItemT, uint8_t MaxSizeV>
  class CircularBuffer
  {
    
  public:
    
    explicit CircularBuffer()
      : m_dataSize(0), m_dataPos(0)
    {
    }
    
    void clear()
    {
      m_dataSize = m_dataPos = 0;
    }
    
    void add(ItemT const& value)
    {
      if (m_dataSize == MaxSizeV)
        del_front(1);
      uint8_t p = getPos(m_dataSize++);
      m_data[p] = value;
    }
    
    void del(uint8_t index)
    {
      for (uint8_t i = index; i != m_dataSize - 1; ++i)
        m_data[ getPos(i) ] = m_data[ getPos(i + 1) ];
          --m_dataSize;
    }
    
    void del_front(uint8_t n)
    {
      m_dataPos = (m_dataPos + n) % MaxSizeV;
      m_dataSize -= n;
    }
    
    void del_back(uint8_t n)
    {
      m_dataSize -= n;
    }
    
    ItemT& operator[] (uint8_t i)
    {
      return m_data[ getPos(i) ];
    }
    
    ItemT const& operator[] (uint8_t i) const
    {
      return m_data[ getPos(i) ];
    }
    
    uint8_t size() const
    {
      return m_dataSize;
    }
    
    uint8_t maxSize() const
    {
      return MaxSizeV;
    }
    
    
  private:
    
    uint8_t getPos(uint8_t i) const
    {
      return static_cast<uint8_t>( (static_cast<uint16_t>(m_dataPos) + i) % MaxSizeV );
    }
    
    uint8_t m_dataSize; // 0..MaxSizeV
    uint8_t m_dataPos;  // 0..MaxSizeV-1
    ItemT   m_data[MaxSizeV];
  };



  ///////////////////////////////////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////////////////////////////////

  // packed bool array (8 bools per byte)
  template <uint16_t MaxSizeV>
  class BoolArray
  {

  private:

    static uint16_t const MaxSizeInBytes = MaxSizeV / 8 + MaxSizeV % 8 == 0? 0 : 1;

  public:

    BoolArray()
    {
      clear();
    }

    void clear()
    {
      m_size = 0;
      for (uint16_t i = 0; i != MaxSizeInBytes; ++i)
        m_data[i] = 0;
    }

    void push_back(bool value)
    {
      m_data[m_size / 8] = (value? 1 : 0) << (m_size % 8);
      ++m_size;
    }

    uint16_t size() const
    {
      return m_size;
    }

    uint16_t maxSize() const
    {
      return MaxSizeV;
    }

    bool operator[] (uint16_t i) const
    {
      return (m_data[i / 8] >> (i % 8)) & 1;
    }

  private:

    uint16_t m_size;
    uint8_t  m_data[MaxSizeInBytes];
  };



  ///////////////////////////////////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////////////////////////////////

  // a simple array
  template <typename ItemT, uint16_t MaxSizeV>
  class Array
  {

  public:

    Array()
      : m_size(0)
    {
    }

    void clear()
    {
      m_size = 0;
    }

    void push_back(ItemT const& value)
    {
      m_data[m_size++] = value;
    }

    ItemT pop_back()
    {
      return m_data[--m_size];
    }

    uint16_t size() const
    {
      return m_size;
    }

    uint16_t maxSize() const
    {
      return MaxSizeV;
    }

    ItemT& operator[] (uint16_t i)
    {
      return m_data[i];
    }
    
    
    ItemT const& operator[] (uint16_t i) const
    {
      return m_data[i];
    }


  private:
    
    ItemT    m_data[MaxSizeV];
    uint16_t m_size;

  };


  ///////////////////////////////////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////////////////////////////////

  // a simple EEPROM array
  template <typename ItemT, uint16_t MaxSizeV>
  class EEPROMArray
  {

    static uint16_t const MAGIC = CompileTimeRandom::value;

  public:

    EEPROMArray()
    {
      m_magic = EEPROMAllocator::allocate<uint16_t>(1);
      m_size  = EEPROMAllocator::allocate<uint16_t>(1);
      m_data  = EEPROMAllocator::allocate<ItemT>(MaxSizeV);
      if (MAGIC != eeprom_read_word(m_magic))
      {
        eeprom_update_word(m_magic, MAGIC);
        eeprom_update_word(m_size, 0);
      }
    }

    void clear()
    {
      eeprom_update_word(m_size, 0);
    }

    void push_back(ItemT const& value, bool allowDuplicates = true)
    {
      if (!allowDuplicates && contains(value))
        return;
      uint16_t sz = size();
      eeprom_update_block(&value, m_data + sz, sizeof(ItemT));
      eeprom_update_word(m_size, sz + 1);
    }

    ItemT pop_back()
    {
      uint16_t sz = size() - 1;
      eeprom_update_word(m_size, sz);
      return get(sz);
    }

    uint16_t size() const
    {
      return eeprom_read_word(m_size);
    }

    uint16_t maxSize() const
    {
      return MaxSizeV;
    }

    bool contains(ItemT const& item) const
    {
      uint16_t sz = size();
      for (uint16_t i = 0; i != sz; ++i)
        if (item == get(i))
          return true;
      return false;
    }

    ItemT const get(uint16_t i) const
    {
      ItemT ret;
      eeprom_read_block(&ret, m_data + i, sizeof(ItemT));
      return ret;
    }

    void set(uint16_t i, ItemT const& value)
    {
      eeprom_update_block(&value, m_data + i, sizeof(ItemT));
    }
    
    

  private:
    
    uint16_t* m_magic;
    ItemT*    m_data;
    uint16_t* m_size;

  };


  ///////////////////////////////////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////////////////////////////////

  
  // dynamic array
  template <typename ItemT> 
  class vector
  {

  public:    
    
    //// types
    
    typedef ItemT*       iterator;
    typedef ItemT const* const_iterator;
    
    
    ///// construct/copy/destroy    
    
    
    explicit vector()
      : m_dataSize(0)
    {
    }
    
    
    explicit vector(size_t n, ItemT const& value = ItemT())
      : m_dataSize(0)
    {
      assign(n, value);
    }
    
    
    vector(vector<ItemT> const& x)
      : m_dataSize(0)
    {
      assign(x.begin(), x.end());
    }
      
    
    vector(const_iterator first, const_iterator last, size_t capacity = 0)
      : m_dataSize(0)
    {
      assign(first, last, capacity);
    }
    
    
    ~vector()
    {
      for (iterator i=begin(); i!=end(); ++i)
        i->~ItemT();
    }


    vector<ItemT>& operator=(vector<ItemT> const& rhs)
    {
      return assign(rhs.begin(), rhs.end());
    }
    
    
    vector<ItemT>& assign(const_iterator first, const_iterator last, size_t capacity = 0)
    {
      clear();
      size_t n = last - first;
      if (reserve(max(capacity, n)))
      {
        m_dataSize = n;
        for (iterator i=begin(); i!=end(); ++i, first++)
          new (static_cast<void*>(&*i)) ItemT(*first);  // placement "new" to call copy constructor
      }
      return *this;
    }
    
    
    vector<ItemT>& assign(size_t n, ItemT const& value)
    {
      clear();
      if (reserve(n))
      {
        m_dataSize = n;
        for (iterator i=begin(); i!=end(); ++i)
          new (static_cast<void*>(&*i)) ItemT(value); // placement "new" to call copy constructor
      }
      return *this;
    }
    
                        
    
    //// iterators
    
    
    iterator begin()
    {
      return m_dataSize? &m_data[0] : NULL;
    }
    
    
    const_iterator begin() const
    {
      return m_dataSize? &m_data[0] : NULL;
    }
    
    
    iterator end()
    {
      return m_dataSize? &m_data[m_dataSize] : NULL;
    }
    
    
    const_iterator end() const
    {
      return m_dataSize? &m_data[m_dataSize] : NULL;
    }
    
        
    
    //// capacity
    
    
    size_t size() const
    {
      return m_dataSize;
    }
    
    
    size_t max_size() const
    {
      return m_data.max_size();
    }
                        
    
    void resize(size_t sz, ItemT const& value = ItemT())
    {
      if (sz < m_dataSize)
      {
        // just reduce m_dataSize and destroy cut off objects
        for (iterator i=begin()+sz; i!=end(); ++i)
          i->~ItemT();
        m_dataSize = sz;
      }
      else if (sz > m_dataSize)
      {
        if (reserve(sz))
        {
          iterator newEnd = &m_data[m_dataSize] + (sz - m_dataSize);
          for (iterator i=&m_data[m_dataSize]; i!=newEnd; ++i)
            new (static_cast<void*>(&*i)) ItemT(value); // placement "new" to call copy constructor
          m_dataSize = sz;
        }
      }
    }
    
    
    size_t capacity() const
    {
      return m_data.size();
    }
    
    
    bool empty() const
    {
      return m_dataSize == 0;
    }
    
    
    // cannot actually shrink the buffer
    bool reserve(size_t n)
    {
      return m_data.resize( max(n, m_data.size()) );
    }
    
    
    void shrink_to_fit()
    {
      m_data.resize(m_dataSize);
    }    
    
    
    //// element access
    
    
    ItemT& operator[] (size_t n)
    {
      return m_data[n];
    }
    
    
    ItemT const& operator[] (size_t n) const
    {
      return m_data[n];
    }
    
    
    ItemT& front()
    {
      return m_data[0];
    };
    
    
    ItemT const& front() const
    {
      return m_data[0];
    }
    
    
    ItemT& back()
    {
      return m_data[m_dataSize-1];
    }
    
    
    ItemT const& back() const
    {
      return m_data[m_dataSize-1];
    }
    
    
    //// modifiers
    
    
    void push_back(ItemT const& value)
    {
      if (reserve(m_dataSize+1))
        new (&m_data[m_dataSize++]) ItemT(value);
    }
    
    
    void pop_back()
    {
      if (m_dataSize > 0)
        erase(end()-1);
    }
    

    iterator insert(iterator position, ItemT const& value)
    {
      return insert(position, 1u, value);
    }
    
    
    iterator insert(iterator position, size_t n, ItemT const& value)
    {
      size_t index = position - begin();  // calculate here, because "position" will be invalidated after "reserve"
      
      if (reserve(m_dataSize + n))
      {
        if (n)
        {
          // move existing elements to the right
          memmove(begin()+index+n, begin()+index, (m_dataSize-index)*sizeof(ItemT));
        
          // creates new objects
          for (size_t i=0; i!=n; ++i)
            new (&m_data[index+i]) ItemT(value);   // placement new (copy constructor)
          m_dataSize += n;
        }        
        return &m_data[index];  // return first inserted element
      }
      else
        return end();  // error
    }
    
    
    void insert(iterator position, iterator first, iterator last)
    {  
      size_t index = position - begin(); // calculate here, becasue "position" will be invalidated after "reserve" 
      size_t n = last - first;
      
      if (n && reserve(m_dataSize + n))
      {      
        // move existing elements to the right
        memmove(begin()+index+n, begin()+index, (m_dataSize-index)*sizeof(ItemT));
        
        // creates new objects
        for (size_t i=0; i!=n; ++i)
          new (&m_data[index+i]) ItemT(*first++);  // placement new (copy constructor)
        m_dataSize += n;
      }
    }
    
                    
    iterator erase(iterator position)
    {
      return erase(position, position+1);
    }
    
                    
    iterator erase(iterator first, iterator last)
    {
      // call destructor for erased elements
      for (iterator f=first; f!=last; ++f)
        f->~ItemT();
      
      // move other elements
      if (end()-last > 0)
        memmove(first, last, (end()-last)*sizeof(ItemT));
      
      m_dataSize -= last - first;
      
      return first;
    }
    
    
    void clear()
    {
      erase(begin(), end());
    }
    
    
    void swap(vector<ItemT> &x)
    {
      x.m_data.swap(m_data);
      fdv::swap(x.m_dataSize, m_dataSize);
    }
    
     
    
  private:
    
    Buffer<ItemT> m_data;
    size_t        m_dataSize;

    
  };  // end of class vector<>
  
  
  template <typename ItemT>
  bool operator== (vector<ItemT> const& x, vector<ItemT> const& y)
  {
    return x.size() == y.size() && equal(x.begin(), x.end(), y.begin());
  }
  
  
  template <typename ItemT>
  bool operator< (vector<ItemT> const& x, vector<ItemT> const& y)
  {  
    return lexicographical_compare(x.begin(), x.end(), y.begin(), y.end());
  }
  
  
  template <typename ItemT>
  bool operator!= (vector<ItemT> const& x, vector<ItemT> const& y)
  {
    return !(x == y);
  }
  
  
  template <typename ItemT> 
  bool operator> (vector<ItemT> const& x, vector<ItemT> const& y)
  {
    return y < x;
  }
  
  
  template <typename ItemT>
  bool operator>= (vector<ItemT> const& x, vector<ItemT> const& y)
  {
    return !(x < y);
  }
  
  
  template <typename ItemT>
  bool operator<= (vector<ItemT> const& x, vector<ItemT> const& y)
  {
    return !(x > y);
  }
  
  
  //// specialized algorithms
  
  
  template <typename ItemT>
  inline void swap(vector<ItemT>& x, vector<ItemT>& y)
  {
    x.swap(y);    
  }
  
  
  
  
  
}  // end of namespace fdv



#endif  // FDV_VECTOR_H
