// 2010 by Fabrizio Di Vittorio (fdivitto@tiscali.it)

#ifndef FDV_UTILITY_H
#define FDV_UTILITY_H


#include <math.h>
#include <util/delay_basic.h>
#include <util/delay.h>



namespace fdv
{

  template <typename T1, typename T2>
  struct pair
  {
    typedef T1 first_type;
    typedef T2 second_type;
    
    T1 first;
    T2 second;
    
    pair() 
    : first(T1()), second(T2()) 
    {
    }
    
    pair(T1 const& x, T2 const& y) 
    : first(x), second(y) 
    {
    }
    
    template <typename U, typename V>
    pair(pair<U, V> const& p) 
    : first(p.first), second(p.second) 
    { 
    }

    bool operator == (pair const & rhs) const
    {
      return first == rhs.first && second == rhs.second;
    }
  };
  
  
  template <typename T1, typename T2>
  inline pair<T1, T2> make_pair(T1 x, T2 y)
  {
    return pair<T1, T2>(x, y);
  }
  

  struct Utility
  {
  
    // Produce a formatted string in a buffer corresponding to the value provided.
    // If the 'width' parameter is non-zero, the value will be padded with leading
    // zeroes to achieve the specified width.  The number of characters added to
    // the buffer (not including the null termination) is returned.
    static uint8_t fmtUInt32(uint32_t val, char* buf, uint8_t bufLen, uint8_t width = 0)
    {
      if (!buf || !bufLen)
        return(0);

      // produce the digit string (backwards in the digit buffer)
      char dbuf[10];
      uint8_t idx = 0;
      while (idx < sizeof(dbuf))
      {
        dbuf[idx++] = (val % 10) + '0';
        if ((val /= 10) == 0)
          break;
      }

      // copy the optional leading zeroes and digits to the target buffer
      uint8_t len = 0;
      uint8_t padding = (width > idx) ? width - idx : 0;
      char c = '0';
      while ((--bufLen > 0) && (idx || padding))
      {
        if (padding)
          padding--;
        else
          c = dbuf[--idx];
        *buf++ = c;
        ++len;
      }

      // add the null termination
      *buf = 0;
      return len;
    }


    static uint8_t fmtUInt64(uint64_t val, char* buf, uint8_t bufLen, uint8_t width = 0)
    {
      if (!buf || !bufLen)
        return(0);

      // produce the digit string (backwards in the digit buffer)
      char dbuf[21];
      uint8_t idx = 0;
      while (idx < sizeof(dbuf))
      {
        dbuf[idx++] = (val % 10) + '0';
        if ((val /= 10) == 0)
          break;
      }

      // copy the optional leading zeroes and digits to the target buffer
      uint8_t len = 0;
      uint8_t padding = (width > idx) ? width - idx : 0;
      char c = '0';
      while ((--bufLen > 0) && (idx || padding))
      {
        if (padding)
          padding--;
        else
          c = dbuf[--idx];
        *buf++ = c;
        ++len;
      }

      // add the null termination
      *buf = 0;
      return len;
    }


    // Format a floating point value with number of decimal places.
    // The 'precision' parameter is a number from 0 to 6 indicating the desired decimal places.
    // The 'buf' parameter points to a buffer to receive the formatted string.  This must be
    // sufficiently large to contain the resulting string.  The buffer's length may be
    // optionally specified.  If it is given, the maximum length of the generated string
    // will be one less than the specified value.
    //
    // example: fmtDouble(3.1415, 2, buf); // produces 3.14 (two decimal places)
    //
    static void fmtDouble(double val, uint8_t precision, char* buf, uint8_t bufLen)
    {
      if (!buf || !bufLen)
        return;

      // limit the precision to the maximum allowed value
      uint8_t const maxPrecision = 6;
      if (precision > maxPrecision)
        precision = maxPrecision;

      if (--bufLen > 0)
      {
        // check for a negative value
        if (val < 0.0)
        {
          val = -val;
          *buf = '-';
          --bufLen;
        }

        // compute the rounding factor and fractional multiplier
        double roundingFactor = 0.5;
        uint32_t mult = 1;
        for (uint8_t i = 0; i < precision; ++i)
        {
          roundingFactor /= 10.0;
          mult *= 10;
        }

        if (bufLen > 0)
        {
          // apply the rounding factor
          val += roundingFactor;

          // add the integral portion to the buffer
          uint8_t len = fmtUInt32((uint32_t)val, buf, bufLen);
          buf += len;
          bufLen -= len;
        }

        // handle the fractional portion
        if ((precision > 0) && (bufLen > 0))
        {
          *buf++ = '.';
          if (--bufLen > 0)
            buf += fmtUInt32((uint32_t)((val - (uint32_t)val) * mult), buf, bufLen, precision);
        }
      }

      // null-terminate the string
      *buf = 0;
    }


    // convert word to big-endian
    static uint16_t htons(uint16_t host_word)
    {
      return (host_word >> 8) | ((host_word & 0xFF) << 8);
    }


    // convert word to little-endian
    static uint16_t ntohs(uint16_t network_word)
    {
      return htons(network_word);
    }
    
  };  // end of Utility struct    



}


#endif  // FDV_UTILITY_H
