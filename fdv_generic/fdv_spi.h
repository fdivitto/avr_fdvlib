// 2010 by Fabrizio Di Vittorio (fdivitto@tiscali.it)


#ifndef FDV_SPI_H_
#define FDV_SPI_H_

#include <stdio.h>
#include <avr/pgmspace.h>

#include "fdv_pin.h"
#include "fdv_debug.h"
#include "fdv_timesched.h"

// TODO: software SPI



namespace fdv
{

  class HardwareSPIMaster
  {


  public:

    enum DataMode
    {
      MODE0 = 0x00,
      MODE1 = 0x04,
      MODE2 = 0x08,
      MODE3 = 0x0C
    };


    enum ClockDiv
    {
      CLOCK_DIV2   = 0b100,  // SPI2X = 1  SPR1,0 = 00
      CLOCK_DIV4   = 0b000,  // SPI2X = 0  SPR1,0 = 00
      CLOCK_DIV8   = 0b101,  // SPI2X = 1  SPR1,0 = 01
      CLOCK_DIV16  = 0b001,  // SPI2X = 0  SPR1,0 = 01
      CLOCK_DIV32  = 0b110,  // SPI2X = 1  SPR1,0 = 10
      CLOCK_DIV64  = 0b010,  // SPI2X = 0  SPR1,0 = 10
      CLOCK_DIV128 = 0b011,  // SPI2X = 0  SPR1,0 = 11
    };


    HardwareSPIMaster()
    {
    }


    // constructor with default hardware SCK, MOSI, MISO, SS
    HardwareSPIMaster(Pin const* CS_, bool LSBFirst, DataMode dataMode, ClockDiv clockDiv)
    {
      init(CS_, LSBFirst, dataMode, clockDiv);
    }



    void init(Pin const* CS_, bool LSBFirst, DataMode dataMode, ClockDiv clockDiv)
    {
      m_CS = CS_;
      m_LSBFirst = LSBFirst;
      m_dataMode = dataMode;
      m_clockDiv = clockDiv;

      m_CS->modeOutput();
      m_CS->writeLow(); // intermediate low state
      m_CS->writeHigh();

      if (s_HardwareInstances == 0)
      {
        // to setup hardware SPI
        PinSPI_SCK.modeOutput();
        PinSPI_MOSI.modeOutput();
        PinSPI_MISO.modeInput();
        PinSPI_SS.modeOutput();

        PinSPI_SCK.writeLow();
        PinSPI_MOSI.writeLow();

        PinSPI_SS.writeLow();  // intermediate low state
        PinSPI_SS.writeHigh();
      }
      calc_SPCR_SPSR();
      ++s_HardwareInstances;
    }

    /*
    ~HardwareSPIMaster()
    {
      --s_HardwareInstances;
      if (s_HardwareInstances == 0)
        SPCR &= ~_BV(SPE);
    }
    */


    ClockDiv getClockDiv()
    {
      return m_clockDiv;
    }


    void setClockDiv(ClockDiv clockDiv)
    {
      m_clockDiv = clockDiv;
      calc_SPCR_SPSR();
    }


    void write(uint8_t data)
    {
      SPDR = data;
      while (!(SPSR & _BV(SPIF)));
    }


    uint8_t read()
    {
      write(0xFF);
      return SPDR;
    }


    void select()
    {
      setup();
      m_CS->writeLow();
      //delayMicroseconds(1);
    }


    void deselect()
    {
      //delayMicroseconds(1);
      m_CS->writeHigh();
    }


  private:

    void setup()
    {
      if (SPCR != m_SPCR)
        SPCR = m_SPCR;
      if ((SPSR & 0b1) != (m_SPSR & 0b1)) // interested only to the first bit change
        SPSR = m_SPSR;
    }


    void calc_SPCR_SPSR()
    {
      m_SPCR = _BV(MSTR) | _BV(SPE);
      m_SPSR = 0;

      // speed
      m_SPCR &= 0b11111100;         // reset SPR1 and SPR0
      m_SPCR |= m_clockDiv & 0b11;  // set SPR1 and SPR0
      m_SPSR &= 0b11111110;         // reset SPI2X
      m_SPSR |= m_clockDiv >> 2;    // set SPI2X

      // bit order
      if (m_LSBFirst)
        m_SPCR |= _BV(DORD);
      else
        m_SPCR &= ~_BV(DORD);

      // data mode
      uint8_t const SPI_MODE_MASK = 0x0C;  // CPOL = bit 3, CPHA = bit 2 on SPCR
      m_SPCR = (m_SPCR & ~SPI_MODE_MASK) | m_dataMode;
    }


  private:

    Pin const*  m_CS;         // the device SS (could be SS=CS)

    bool        m_LSBFirst;   // true=LSBFirst false=MSBFirst
    DataMode    m_dataMode;
    ClockDiv    m_clockDiv;
    uint8_t     m_SPCR;
    uint8_t     m_SPSR;

    static uint8_t s_HardwareInstances;  // number of hardware SPI users (instances of SPIMaster which uses hardware SPI)

  };



} // end of fdv namespace


#endif /* FDV_SPI_H_ */
