// 2010 by Fabrizio Di Vittorio (fdivitto@tiscali.it)

#ifndef FDV_DS18B20_H
#define FDV_DS18B20_H

#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include <avr/pgmspace.h>

#include "fdv_pin.h"
#include "fdv_timesched.h"
#include "fdv_onewire.h"




namespace fdv
{

  static uint16_t const DS18B20_waitConv[4] PROGMEM = { 94, 188, 375, 750 }; // wait conversion in ms
  static uint8_t const  DS18B20_zeroBit[4] PROGMEM = { 0b1000, 0b1100, 0b1110, 0b1111 };
  static uint8_t const  DS18B20_conv[4] PROGMEM= { 0x1F, 0x3F, 0x5F, 0x7F}; // 9, 10, 11, 12 bits


  // class to control DS18B20 temperature sensor(s) (serial 1-wire controlled)
  class DS18B20
  {

  public:


    explicit DS18B20(Pin const* pin)
      : m_onewire(pin)
    {
    }


    // resolution can be: 9 (0.5°C), 10 (0.25°C), 11 (0.125°C), 12 bits (0.0625°C). For each one conversion time is: 93.75ms, 187.5ms, 375ms, 750ms
    bool readTemperature(int16_t* num, int16_t* den, uint8_t resolution = 12, uint8_t const* deviceAddress = NULL)
    {
      uint8_t const resIdx = resolution - 9;

      // set resolution
      if (!m_onewire.reset())
        return false;
      m_onewire.select(deviceAddress);
      m_onewire.write(0x4e); // "WRITE SCRATCHPAD"
      m_onewire.write(0x00); // TH
      m_onewire.write(0x00); // TL
      m_onewire.write(pgm_read_byte(&DS18B20_conv[resIdx])); // CONFIGURATION REGISTER

      // start temperature acquisition
      if (!m_onewire.reset())
        return false;
      m_onewire.select(deviceAddress);
      m_onewire.write(0x44); // "CONVERT T"
      delay_ms(pgm_read_word(&DS18B20_waitConv[resIdx])); // wait for conversion

      // read acquired temperature
      if (!m_onewire.reset())
        return false;
      m_onewire.select(deviceAddress);
      m_onewire.write(0xbe); // "READ SCRATCHPAD"
      uint8_t LSB = m_onewire.read(); // Scratchpad byte #0: Temperature LSB
      uint8_t MSB = m_onewire.read(); // Scratchpad byte #1: Temperature MSB
      *num = ((LSB >> 4) | ((MSB & 0x7) << 4)) * 16 + (LSB & pgm_read_byte(&DS18B20_zeroBit[resIdx]));
      *den = 16;
      if (MSB & 0x80) *num = -*num;
	  
      return true;
    }


    // resolution can be: 9 (0.5°C), 10 (0.25°C), 11 (0.125°C), 12 bits (0.0625°C). For each one conversion time is: 93.75ms, 187.5ms, 375ms, 750ms
    bool readTemperature(float* result, uint8_t resolution = 12, uint8_t const* deviceAddress = NULL)
    {
      int16_t num, den;
      bool ret = readTemperature(&num, &den, resolution, deviceAddress);
      if (ret)
        *result = static_cast<float>(num) / static_cast<float>(den);
      return ret;
    }

  private:

    OneWire m_onewire;
  };




} // end of "fdv" namespace

#endif // FDV_DS18B20_H
