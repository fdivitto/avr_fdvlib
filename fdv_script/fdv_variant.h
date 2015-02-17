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



#ifndef FDV_VARIANT_H_
#define FDV_VARIANT_H_


#if defined(__AVR__)
#include "../fdv_generic/fdv_string.h"
#endif


namespace fdv
{

  ///////////////////////////////////////////////////////////////////////////////////////////////
  // Variant

  struct Variant
  {
    enum Type
    {
      INVALID   = 0b00000001, // 1
      REFERENCE = 0b00000010, // 2
      ARRAY     = 0b00000100, // 4
      STRING    = 0b00001000, // 8
      FLOAT     = 0b00010000, // 16
      UINT8     = 0b00100000, // 32
      UINT16    = 0b01000000, // 64
      UINT32    = 0b10000000  // 128
    };

    Variant() :
      m_type(INVALID)
      {
      }

      explicit Variant(Variant* value) :
      m_type(REFERENCE), m_refVal(value)
      {
      }

      explicit Variant(char const* value) :
      m_type(STRING), m_stringVal(new string(value))
      {
      }

      explicit Variant(string const& value) :
      m_type(STRING), m_stringVal(new string(value))
      {
      }

      explicit Variant(vector<Variant> const& value) :
      m_type(ARRAY), m_arrayVal(new vector<Variant>(value))
      {
      }

      explicit Variant(float value) :
      m_type(FLOAT), m_floatVal(value)
      {
      }

      explicit Variant(double value) :
      m_type(FLOAT), m_floatVal(value)
      {
      }

      explicit Variant(uint8_t value) :
      m_type(UINT8), m_uint8Val(value)
      {
      }

      explicit Variant(uint16_t value) :
      m_type(UINT16), m_uint16Val(value)
      {
      }

      explicit Variant(uint32_t value) :
      m_type(UINT32), m_uint32Val(value)
      {
      }

      Variant(Variant const& c) :
      m_type(c.m_type)
      {
        switch (m_type)
        {
        case STRING:
          m_stringVal = new string(*c.m_stringVal);
          break;
        case ARRAY:
          m_arrayVal = new vector<Variant>(*c.m_arrayVal);
          break;
        default:
          m_uint32Val = c.m_uint32Val; // copies largest value
          break;
        }
      }

      ~Variant()
      {
        clear();
      }

      uint16_t size()
      {
        switch (m_type)
        {
        case REFERENCE:
          return sizeof(Variant*);
        case ARRAY:
          return m_arrayVal->size();
        case STRING:
          return m_stringVal->size();
        case FLOAT:
          return sizeof(m_floatVal);
        case UINT8:
          return sizeof(uint8_t);
        case UINT16:
          return sizeof(uint16_t);
        case UINT32:
          return sizeof(uint32_t);
        default:
          return 0;
        }
      }

      void operator=(Variant const& c)
      {
        clear();
        m_type = c.m_type;
        switch (m_type)
        {
        case STRING:
          m_stringVal = new string(*c.m_stringVal);
          break;
        case ARRAY:
          m_arrayVal = new vector<Variant>(*c.m_arrayVal);
          break;
        default:
          m_uint32Val = c.m_uint32Val; // copies largest value
          break;
        }
      }

      void operator=(bool v)
      {
        uint8Val() = v;
      }

      void clear()
      {
        if (m_type==STRING)
          delete m_stringVal;
        else if (m_type==ARRAY)
          delete m_arrayVal;
        m_type = INVALID;
      }

      Type type() const
      {
        return m_type;
      }

      // true = new type assigned (false, no change necessary)
      bool type(Type type_)
      {
        if (type_ != m_type)
        {
          clear();
          m_type = type_;
          switch (m_type)
          {
          case STRING:
            m_stringVal = new string;
            break;
          case ARRAY:
            m_arrayVal = new vector<Variant>;
            break;
          default:
            m_uint32Val = 0;  // init largest value
            break;
          }
          return true;
        }
        return false;
      }


  private:

#if defined(__AVR__)

    template <typename T>
    static string const toString(T f)
    {
      return fdv::toString(f);
    }

#else

    template <typename T>
    static string const toString(T f)
    {
      ostringstream ss;
      ss << (double)f;
      return ss.str();
    }

#endif

    // note: convert to a dynamic array (array initialization)
    static string const toString(vector<Variant>& a)
    {
      string ret;
      ret.push_back('{');
      if (a.size() > 0)
      {
        for (vector<Variant>::iterator i=a.begin(); i!=a.end(); ++i)
        {
          if (i->type() == STRING)
          {
            ret.push_back('\"');
            for (char const* j=&i->stringVal()[0]; *j; ++j)
            {
              if (*j=='\"')
              {
                ret.push_back('\\');
                ret.push_back('\"');
              }
              else
                ret.push_back(*j);
            }
            ret.push_back('\"');
          }
          else
            ret.append(i->toString());
          if (i!=a.end()-1)
            ret.push_back(',');
        }
      }
      ret.push_back('}');
      return ret;
    }


  public:


    string const toString() const
    {
      switch (m_type)
      {
      case REFERENCE:
        return toString(uint16_t(m_refVal));  // prints pointer
      case STRING:
        return *m_stringVal;
      case ARRAY:
        return toString(*m_arrayVal);
      case FLOAT:
        return toString(m_floatVal);
      case UINT8:
        return toString(m_uint8Val);
      case UINT16:
        return toString(m_uint16Val);
      case UINT32:
        return toString(m_uint32Val);
      default:
        return string();
      }
    }

    float toFloat() const
    {
      switch (m_type)
      {
      case STRING:
        return strtod(m_stringVal->c_str(), NULL);
      case FLOAT:
        return m_floatVal;
      case UINT8:
        return m_uint8Val;
      case UINT16:
        return m_uint16Val;
      case UINT32:
        return m_uint32Val;
      default:
        return 0.0;
      }
    }

    uint32_t toUInt32() const
    {
      switch (m_type)
      {
      case STRING:
        return strtoul(m_stringVal->c_str(), NULL, 10);
      case FLOAT:
        return (uint32_t)m_floatVal;
      case UINT8:
        return m_uint8Val;
      case UINT16:
        return m_uint16Val;
      case UINT32:
        return m_uint32Val;
      default:
        return 0;
      }
    }

    bool toBool() const
    {
      switch (m_type)
      {
      case REFERENCE:
        return m_refVal != NULL;
      case STRING:
        return !m_stringVal->empty(); // TODO: should convert from "true/false/0/1" to int?
      case ARRAY:
        return !m_arrayVal->empty();
      case FLOAT:
        return m_floatVal != 0.0;
      case UINT8:
        return m_uint8Val != 0;
      case UINT16:
        return m_uint16Val != 0;
      case UINT32:
        return m_uint32Val != 0;
      default:
        return false;
      }
    }

    void shrink(uint32_t value)
    {
      if (value < 256)
        uint8Val() = static_cast<uint8_t>(value);   // UINT8
      else if (value < 65536)
        uint16Val() = static_cast<uint16_t>(value); // UINT16
      else
        uint32Val() = value;                        // UINT32
    }

    Variant*& refVal()
    {
      type(REFERENCE);
      return m_refVal;
    }

    string& stringVal()
    {
      type(STRING);
      return *m_stringVal;
    }

    vector<Variant>& arrayVal()
    {
      type(ARRAY);
      return *m_arrayVal;
    }

    float& floatVal()
    {
      type(FLOAT);
      return m_floatVal;
    }

    uint8_t& uint8Val()
    {
      type(UINT8);
      return m_uint8Val;
    }

    uint16_t& uint16Val()
    {
      type(UINT16);
      return m_uint16Val;
    }

    uint32_t& uint32Val()
    {
      type(UINT32);
      return m_uint32Val;
    }

    void operator--()
    {
      switch (m_type)
      {
      case FLOAT:
        --m_floatVal;
        break;
      case UINT8:
        --m_uint8Val;
        break;
      case UINT16:
        --m_uint16Val;
        break;
      case UINT32:
        --m_uint32Val;
        break;
      default:
        break;
      }
    }

    void operator++()
    {
      switch (m_type)
      {
      case FLOAT:
        ++m_floatVal;
        break;
      case UINT8:
        ++m_uint8Val;
        break;
      case UINT16:
        ++m_uint16Val;
        break;
      case UINT32:
        ++m_uint32Val;
        break;
      default:
        break;
      }
    }

    Variant const operator-() const
    {
      switch (m_type)
      {
      case STRING:  // no change
        return Variant(*m_stringVal);
      case ARRAY:   // no change
        return Variant(*m_arrayVal);
      case FLOAT:
        return Variant(-m_floatVal);
        /*
        case UINT8:
        return Variant((uint8_t)-m_uint8Val);
        case UINT16:
        return Variant((uint16_t)-m_uint16Val);
        case UINT32:
        return Variant(-m_uint32Val);
        */
      default:
        return Variant(-toFloat()); // until signed integers will be supported
      }
    }

    Variant const operator!() const
    {
      switch (m_type)
      {
      case STRING:  // no change
        return Variant(*m_stringVal);
      case ARRAY:   // no change
        return Variant(*m_arrayVal);
      case FLOAT:
        return Variant(m_floatVal?1.0:0.0);
      case UINT8:
        return Variant((uint8_t)!m_uint8Val);
      case UINT16:
        return Variant((uint16_t)!m_uint16Val);
      case UINT32:
        return Variant((uint32_t)!m_uint32Val);
      default:
        return Variant();
      }
    }

    Variant const operator~() const
    {
      switch (m_type)
      {
      case STRING:  // no change
        return Variant(*m_stringVal);
      case FLOAT:   // no change
        return Variant(m_floatVal);
      case ARRAY:   // no change
        return Variant(*m_arrayVal);
      case UINT8:
        return Variant((uint8_t)~m_uint8Val);
      case UINT16:
        return Variant((uint16_t)~m_uint16Val);
      case UINT32:
        return Variant((uint32_t)~m_uint32Val);
      default:
        return Variant();
      }
    }

    Variant const operator*(Variant const& rhs) const
    {
      if (m_type==FLOAT || rhs.m_type==FLOAT)
        return Variant( toFloat() * rhs.toFloat() );
      else
      {
        Variant r( toUInt32() * rhs.toUInt32() );
        r.shrink(r.toUInt32());
        return r;
      }
    }

    Variant const operator/(Variant const& rhs) const
    {
      if (m_type==FLOAT || rhs.m_type==FLOAT)
        return Variant( toFloat() / rhs.toFloat() );
      else
      {
        Variant r( toUInt32() / rhs.toUInt32() );
        r.shrink(r.toUInt32());
        return r;
      }
    }

    Variant const operator%(Variant const& rhs) const
    {
      Variant r( toUInt32() % rhs.toUInt32() );
      r.shrink(r.toUInt32());
      return r;
    }

    Variant const operator+(Variant const& rhs) const
    {
      if (m_type==STRING || rhs.m_type==STRING) // TODO: optimize
        return Variant( toString() + rhs.toString() );
      else if (m_type==FLOAT || rhs.m_type==FLOAT)
        return Variant( toFloat() + rhs.toFloat() );
      else
      {
        Variant r( toUInt32() + rhs.toUInt32() );
        r.shrink(r.toUInt32());
        return r;
      }
    }

    Variant const operator-(Variant const& rhs) const
    {
      if (m_type==FLOAT || rhs.m_type==FLOAT)
        return Variant( toFloat() - rhs.toFloat() );
      else
      {
        Variant r( toUInt32() - rhs.toUInt32() );
        r.shrink(r.toUInt32());
        return r;
      }
    }

    bool operator<(Variant const& rhs) const
    {
      if (m_type==STRING || rhs.m_type==STRING)
        return toString() < rhs.toString();
      else if (m_type==FLOAT || rhs.m_type==FLOAT)
        return toFloat() < rhs.toFloat();
      else
        return toUInt32() < rhs.toUInt32();
    }

    bool operator<=(Variant const& rhs) const
    {
      return *this < rhs || *this == rhs;
    }

    bool operator>(Variant const& rhs) const
    {
      if (m_type==STRING || rhs.m_type==STRING)
        return toString() > rhs.toString();
      else if (m_type==FLOAT || rhs.m_type==FLOAT)
        return toFloat() > rhs.toFloat();
      else
        return toUInt32() > rhs.toUInt32();
    }

    bool operator>=(Variant const& rhs) const
    {
      return *this > rhs || *this == rhs;
    }

    bool operator==(Variant const& rhs) const
    {
      if (m_type==STRING && rhs.m_type==STRING)
        return *m_stringVal == *rhs.m_stringVal;
      else if (m_type==STRING || rhs.m_type==STRING) // TODO: optimize comparing native values
        return toString() == rhs.toString();
      else if (m_type==FLOAT || rhs.m_type==FLOAT)
        return toFloat() == rhs.toFloat();
      else if (m_type==ARRAY && rhs.m_type==ARRAY)
        return *m_arrayVal == *rhs.m_arrayVal;
      else if (m_type==REFERENCE && rhs.m_type==REFERENCE)
        return m_refVal == rhs.m_refVal;
      else
        return toUInt32() == rhs.toUInt32();
    }

    bool operator!=(Variant const& rhs) const
    {
      return !operator==(rhs);
    }

    Variant const operator|(Variant const& rhs) const
    {
      if (m_type==UINT32 || rhs.m_type==UINT32)
        return Variant(toUInt32() | rhs.toUInt32());
      else if (m_type==UINT16 || rhs.m_type==UINT16)
        return Variant(static_cast<uint16_t>(toUInt32() | rhs.toUInt32()));
      else if (m_type==UINT8 || rhs.m_type==UINT8)
        return Variant(static_cast<uint8_t>(toUInt32() | rhs.toUInt32()));
      else
        return Variant();
    }

    Variant const operator^(Variant const& rhs) const
    {
      if (m_type==UINT32 || rhs.m_type==UINT32)
        return Variant(toUInt32() ^ rhs.toUInt32());
      else if (m_type==UINT16 || rhs.m_type==UINT16)
        return Variant(static_cast<uint16_t>(toUInt32() ^ rhs.toUInt32()));
      else if (m_type==UINT8 || rhs.m_type==UINT8)
        return Variant(static_cast<uint8_t>(toUInt32() ^ rhs.toUInt32()));
      else
        return Variant();
    }

    Variant const operator&(Variant const& rhs) const
    {
      if (m_type==UINT32 || rhs.m_type==UINT32)
        return Variant(toUInt32() & rhs.toUInt32());
      else if (m_type==UINT16 || rhs.m_type==UINT16)
        return Variant(static_cast<uint16_t>(toUInt32() & rhs.toUInt32()));
      else if (m_type==UINT8 || rhs.m_type==UINT8)
        return Variant(static_cast<uint8_t>(toUInt32() & rhs.toUInt32()));
      else
        return Variant();
    }

    Variant const operator<<(Variant const& rhs) const
    {
      if (m_type==UINT8)
        return Variant(static_cast<uint8_t>(m_uint8Val << rhs.toUInt32()));
      else if (m_type==UINT16)
        return Variant(static_cast<uint16_t>(m_uint16Val << rhs.toUInt32()));
      else if (m_type==UINT32)
        return Variant(static_cast<uint32_t>(m_uint32Val << rhs.toUInt32()));
      else
        return Variant();
    }

    Variant const operator>>(Variant const& rhs) const
    {
      if (m_type==UINT8)
        return Variant(static_cast<uint8_t>(m_uint8Val >> rhs.toUInt32()));
      else if (m_type==UINT16)
        return Variant(static_cast<uint16_t>(m_uint16Val >> rhs.toUInt32()));
      else if (m_type==UINT32)
        return Variant(static_cast<uint32_t>(m_uint32Val >> rhs.toUInt32()));
      else
        return Variant();
    }


  private:

    Type m_type;

    union
    {
      string*          m_stringVal;
      vector<Variant>* m_arrayVal;
      Variant*         m_refVal;
      float            m_floatVal;
      uint8_t          m_uint8Val;
      uint16_t         m_uint16Val;
      uint32_t         m_uint32Val;
    };
  };


  inline string const toString(Variant const& v)
  {
    return v.toString();
  }


} // end of fdv namespace

#endif /* FDV_VARIANT_H_ */
