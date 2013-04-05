


#include <avr/io.h>
#include <avr/interrupt.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>


#include "fdv_platform.h"
#include "fdv_serial.h"


namespace fdv
{


  // Interrupt handlers
  #if defined(FDV_ATMEGA1280_2560)

    ISR(USART0_RX_vect)
    {
      HardwareSerial<0>::getBuffer()->put(UDR0);
    }

    ISR(USART1_RX_vect)
    {
      HardwareSerial<1>::getBuffer()->put(UDR1);
    }

    ISR(USART2_RX_vect)
    {
      HardwareSerial<2>::getBuffer()->put(UDR2);
    }

    ISR(USART3_RX_vect)
    {
      HardwareSerial<3>::getBuffer()->put(UDR3);
    }

  #else

    ISR(USART_RX_vect)
    {
      HardwareSerial<0>::getBuffer()->put(UDR0);
    }

  #endif














}
