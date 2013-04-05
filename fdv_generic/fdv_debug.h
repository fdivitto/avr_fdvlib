#ifndef FDV_DEBUG_H_
#define FDV_DEBUG_H_


#include "fdv_utility.h"


#if defined(FDVDEBUG)
  extern void debugStr(char const* msg);


  namespace fdv
  {

    struct Debug
    {
      Debug const& operator << (char const* msg) const
      {
        debugStr(msg);
        return *this;
      }

      Debug const& operator << (char* msg) const
      {
        debugStr(msg);
        return *this;
      }

      Debug const& operator << (char value) const
      {
        char buf[2];
        buf[0] = value; buf[1] = 0;
        debugStr(&buf[0]);
        return *this;
      }

      Debug const& operator << (uint16_t value) const
      {
        return operator<<((uint32_t)value);
      }

      Debug const& operator << (uint32_t value) const
      {
        char str[11];
        fmtUInt32(value, &str[0], 10);
        debugStr(&str[0]);
        return *this;
      }

      template <typename StringT>
      Debug const& operator << (StringT const& value) const
      {
        debugStr(value.c_str());
        return *this;
      }

    };

    #define ENDL "\x0d\x0a"
    //#define ENDL '\r'
    #define debug fdv::Debug()

  } // end of fdv namespace


#else // not defined(FDVDEBUG)


  namespace fdv
  {

    struct Debug
    {
      template <typename T>
      Debug const& operator << (T) const
      {
        return *this;
      }
    };

    #define ENDL 0
    #define debug fdv::Debug()

  } // end of fdv namespace



#endif  // end of defined(FDVDEBUG)

#endif /* FDV_DEBUG_H_ */
