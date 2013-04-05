// 2010 by Fabrizio Di Vittorio (fdivitto@tiscali.it)




#ifndef FDV_AXE033_H
#define FDV_AXE033_H

#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <alloca.h>

#include <avr/interrupt.h>
#include <avr/pgmspace.h>


#include "fdv_twowire.h"
#include "fdv_memory.h"
#include "fdv_timesched.h"


namespace fdv
{



  ////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////
  // Class to control AXE033 LCD
  // 2010 by Fabrizio Di Vittorio (fdivitto@tiscali.it)
  // You may use fdv_DS1307.h to control AXE033 real time clock.
  //
  // Uses i2c connection (analog-in 4->SDA 5->SCL)
  // Delay 500ms before send commands

  class AXE033LCD
  {
  public:
    
    explicit AXE033LCD(uint8_t address = 0x63)
      : m_address(address), m_col(0), m_row(0)
    {
      TaskManager::init();
      delay(200); // wait for LCD initialization (actually needed only for powerup)
    }
    

    // send text
    void textOut(char const* str)
    {
      uint8_t slen = strlen(str);
      while (slen>0)
      {
        uint8_t len = slen<(16-m_col)? slen : (16-m_col);
        uint8_t buffer[18];
        buffer[0] = 0;
        memcpy(&buffer[1], str, len);
        buffer[len+1] = 255;
        m_I2C.writeTo(m_address, &buffer[0], len+2);
        delay(10);
        slen -= len;
        str += len;
        m_col += len;
        if (m_col==16)
        {
          m_col = 0;
          ++m_row;
          moveTo(m_row, m_col);
        }
      }
    }
    

    void textOut_P(PGM_P str)
    {
      char* buf = (char*)alloca(strlen_P(str)+1);
      strcpy_P(buf, str);
      textOut(buf);
    }


    // send text at specified position
    void textOut(uint8_t row, uint8_t col, char const* str)
    {
      moveTo(row, col);
      textOut(str);
    }


    void textOut_P(uint8_t row, uint8_t col, PGM_P str)
    {
      moveTo(row, col);
      textOut_P(str);
    }


    // clear display and home the cursor
    void clearDisplay()
    {
      sendCMD(1);
      delay(20);  // 10+20 ms delay
      moveTo(0, 0);
    }
    
    // move to position (row=0..15, col=0..1)
    void moveTo(uint8_t row, uint8_t col)
    {
      m_row = row;
      m_col = col;
      sendCMD( (row == 0 ? 128 : 192) + col );
      delay(10);
    }
    
    // turn on/off cursor and display and cursor blinking
    void setDisplay(bool displayVisible, bool cursorVisible, bool cursorBlink)
    {
      sendCMD(8 | (displayVisible? 4 : 0) | (cursorVisible? 2 : 0) | (cursorBlink? 1 : 0));
    }

    
  private:

    uint8_t m_address;
    TwoWire m_I2C;
    uint8_t m_col;
    uint8_t m_row;
    
    void sendCMD(uint8_t cmd)
    {
      uint8_t buffer[4];
      buffer[0] = 0;
      buffer[1] = 254;
      buffer[2] = cmd;
      buffer[3] = 255;
      m_I2C.writeTo(m_address, &buffer[0], 4);
      delay(10);
    }
    
  };


} // end of "fdv" namespace

#endif // FDV_AXE033_H


