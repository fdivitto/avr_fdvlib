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




#ifndef FDV_PORTS_H_
#define FDV_PORTS_H_


#include <inttypes.h>

#include <avr/io.h>

#include "fdv_platform.h"
#include "fdv_interrupt.h"


namespace fdv
{


  struct Pin
  {
    volatile uint8_t*    DDR_ADDR;
    volatile uint8_t*    IN_ADDR;
    volatile uint8_t*    OUT_ADDR;
    uint8_t              BIT_MSK;
    ExtInterrupt::ExtInt EXT_INT;
    uint8_t              ADC_PIN;

    void modeOutput() const
    {
      *DDR_ADDR |= BIT_MSK;
    }

    void modeInput() const
    {
      *DDR_ADDR &= ~BIT_MSK;
    }

    void writeLow() const
    {
      *OUT_ADDR &= ~BIT_MSK;
    }

    void writeHigh() const
    {
      *OUT_ADDR |= BIT_MSK;
    }

    void write(uint8_t level) const
    {
      if (level)
        *OUT_ADDR |= BIT_MSK;
      else
        *OUT_ADDR &= ~BIT_MSK;
    }

    uint8_t read() const
    {
      return (*IN_ADDR & BIT_MSK) ? 1 : 0;
    }
  };



  #if defined (FDV_ATMEGA88_328)

    // PORT-B

    Pin const PinB0 = {&DDRB, &PINB, &PORTB, _BV(PB0)};  // Arduino D-8
    Pin const PinB1 = {&DDRB, &PINB, &PORTB, _BV(PB1)};  // Arduino D-9
    Pin const PinB2 = {&DDRB, &PINB, &PORTB, _BV(PB2)};  // Arduino D-10 (SPI_SS)
    Pin const PinB3 = {&DDRB, &PINB, &PORTB, _BV(PB3)};  // Arduino D-11 (SPI_MOSI)
    Pin const PinB4 = {&DDRB, &PINB, &PORTB, _BV(PB4)};  // Arduino D-12 (SPI_MISO)
    Pin const PinB5 = {&DDRB, &PINB, &PORTB, _BV(PB5)};  // Arduino D-13 (led) (SPI_SCK)
    Pin const PinB6 = {&DDRB, &PINB, &PORTB, _BV(PB6)};  // Arduino (XTAL1)
    Pin const PinB7 = {&DDRB, &PINB, &PORTB, _BV(PB7)};  // Arduino (XTAL2)
      

    #define PinSPI_SS   PinB2
    #define PinSPI_MOSI PinB3
    #define PinSPI_MISO PinB4
    #define PinSPI_SCK  PinB5

    #define PinLED PinB5

    // PORT-C

    Pin const PinC0 = {&DDRC, &PINC, &PORTC, _BV(PC0), ExtInterrupt::EXTINT_NONE, 0};  // Arduino A-0 (ADC0)
    Pin const PinC1 = {&DDRC, &PINC, &PORTC, _BV(PC1), ExtInterrupt::EXTINT_NONE, 1};  // Arduino A-1 (ADC1)
    Pin const PinC2 = {&DDRC, &PINC, &PORTC, _BV(PC2), ExtInterrupt::EXTINT_NONE, 2};  // Arduino A-2 (ADC2)
    Pin const PinC3 = {&DDRC, &PINC, &PORTC, _BV(PC3), ExtInterrupt::EXTINT_NONE, 3};  // Arduino A-3 (ADC3)
    Pin const PinC4 = {&DDRC, &PINC, &PORTC, _BV(PC4), ExtInterrupt::EXTINT_NONE, 4};  // Arduino A-4 (ADC4)(I2C_SDL)
    Pin const PinC5 = {&DDRC, &PINC, &PORTC, _BV(PC5), ExtInterrupt::EXTINT_NONE, 5};  // Arduino A-5 (ADC5)(I2C_SCL)
    Pin const PinC6 = {&DDRC, &PINC, &PORTC, _BV(PC6)};                                // Arduino (RESET)

    // PORT-D

    Pin const PinD0 = {&DDRD, &PIND, &PORTD, _BV(PD0)};                                // Arduino D-0 (RXD)
    Pin const PinD1 = {&DDRD, &PIND, &PORTD, _BV(PD1)};                                // Arduino D-1 (TXD)
    Pin const PinD2 = {&DDRD, &PIND, &PORTD, _BV(PD2), ExtInterrupt::EXTINT_INT0};     // Arduino D-2 (INT0)
    Pin const PinD3 = {&DDRD, &PIND, &PORTD, _BV(PD3), ExtInterrupt::EXTINT_INT1};     // Arduino D-3 (INT1)
    Pin const PinD4 = {&DDRD, &PIND, &PORTD, _BV(PD4)};                                // Arduino D-4
    Pin const PinD5 = {&DDRD, &PIND, &PORTD, _BV(PD5)};                                // Arduino D-5
    Pin const PinD6 = {&DDRD, &PIND, &PORTD, _BV(PD6)};                                // Arduino D-6
    Pin const PinD7 = {&DDRD, &PIND, &PORTD, _BV(PD7)};                                // Arduino D-7

    // Arduino Ethernet/SD shield (v.05)

    #define PinArduinoETHERNET_CS               PinB2  /* CS of W5100  (D-10)  */
    #define PinArduinoSDCARD_CS                 PinD4  /* CS of SDCARD (D-4)   */
    #define PinArduinoSDCARD_WRITEPROTECTSENSOR PinC0  /* write protect sensor */
    #define PinArduinoSDCARD_PLUGGEDINSENSOR    PinC1  /* plugged in sensor    */




  #elif defined(FDV_ATMEGA1280_2560)


    // PORT-A

    Pin const PinA0 = {&DDRA, &PINA, &PORTA, _BV(PA0)}; // Arduino D-22
    Pin const PinA1 = {&DDRA, &PINA, &PORTA, _BV(PA1)}; // Arduino D-23
    Pin const PinA2 = {&DDRA, &PINA, &PORTA, _BV(PA2)}; // Arduino D-24
    Pin const PinA3 = {&DDRA, &PINA, &PORTA, _BV(PA3)}; // Arduino D-25
    Pin const PinA4 = {&DDRA, &PINA, &PORTA, _BV(PA4)}; // Arduino D-26
    Pin const PinA5 = {&DDRA, &PINA, &PORTA, _BV(PA5)}; // Arduino D-27
    Pin const PinA6 = {&DDRA, &PINA, &PORTA, _BV(PA6)}; // Arduino D-28
    Pin const PinA7 = {&DDRA, &PINA, &PORTA, _BV(PA7)}; // Arduino D-29


    // PORT-B

    Pin const PinB0 = {&DDRB, &PINB, &PORTB, _BV(PB0)};  // Arduino D-53 (SPI_SS)
    Pin const PinB1 = {&DDRB, &PINB, &PORTB, _BV(PB1)};  // Arduino D-52 (SPI_SCK)
    Pin const PinB2 = {&DDRB, &PINB, &PORTB, _BV(PB2)};  // Arduino D-51 (SPI_MOSI)
    Pin const PinB3 = {&DDRB, &PINB, &PORTB, _BV(PB3)};  // Arduino D-50 (SPI_MISO)
    Pin const PinB4 = {&DDRB, &PINB, &PORTB, _BV(PB4)};  // Arduino D-10
    Pin const PinB5 = {&DDRB, &PINB, &PORTB, _BV(PB5)};  // Arduino D-11
    Pin const PinB6 = {&DDRB, &PINB, &PORTB, _BV(PB6)};  // Arduino D-12
    Pin const PinB7 = {&DDRB, &PINB, &PORTB, _BV(PB7)};  // Arduino D-13 (led)

    #define PinSPI_SS   PinB0
    #define PinSPI_MOSI PinB2
    #define PinSPI_MISO PinB3
    #define PinSPI_SCK  PinB1

    #define PinLED PinB7


    // PORT-C

    Pin const PinC0 = {&DDRC, &PINC, &PORTC, _BV(PC0)};  // Arduino D-37
    Pin const PinC1 = {&DDRC, &PINC, &PORTC, _BV(PC1)};  // Arduino D-36
    Pin const PinC2 = {&DDRC, &PINC, &PORTC, _BV(PC2)};  // Arduino D-35
    Pin const PinC3 = {&DDRC, &PINC, &PORTC, _BV(PC3)};  // Arduino D-34
    Pin const PinC4 = {&DDRC, &PINC, &PORTC, _BV(PC4)};  // Arduino D-33
    Pin const PinC5 = {&DDRC, &PINC, &PORTC, _BV(PC5)};  // Arduino D-32
    Pin const PinC6 = {&DDRC, &PINC, &PORTC, _BV(PC6)};  // Arduino D-31
    Pin const PinC7 = {&DDRC, &PINC, &PORTC, _BV(PC7)};  // Arduino D-30

    // PORT-D

    Pin const PinD0 = {&DDRD, &PIND, &PORTD, _BV(PD0), ExtInterrupt::EXTINT_INT0};  // Arduino D-21 (I2C_SCL) (INT0)
    Pin const PinD1 = {&DDRD, &PIND, &PORTD, _BV(PD1), ExtInterrupt::EXTINT_INT1};  // Arduino D-20 (I2C_SDA) (INT1)
    Pin const PinD2 = {&DDRD, &PIND, &PORTD, _BV(PD2), ExtInterrupt::EXTINT_INT2};  // Arduino D-19 (USART1_RX) (INT2)
    Pin const PinD3 = {&DDRD, &PIND, &PORTD, _BV(PD3), ExtInterrupt::EXTINT_INT3};  // Arduino D-18 (USART1_TX) (INT3)
    Pin const PinD4 = {&DDRD, &PIND, &PORTD, _BV(PD4)};
    Pin const PinD5 = {&DDRD, &PIND, &PORTD, _BV(PD5)};
    Pin const PinD6 = {&DDRD, &PIND, &PORTD, _BV(PD6)};
    Pin const PinD7 = {&DDRD, &PIND, &PORTD, _BV(PD7)};     // Arduino D-38

    // PORT-E

    Pin const PinE0 = {&DDRE, &PINE, &PORTE, _BV(PE0)};     // Arduino D-0 (USART0_RX)
    Pin const PinE1 = {&DDRE, &PINE, &PORTE, _BV(PE1)};     // Arduino D-1 (USART0_TX)
    Pin const PinE2 = {&DDRE, &PINE, &PORTE, _BV(PE2)};
    Pin const PinE3 = {&DDRE, &PINE, &PORTE, _BV(PE3)};     // Arduino D-5
    Pin const PinE4 = {&DDRE, &PINE, &PORTE, _BV(PE4), ExtInterrupt::EXTINT_INT4};  // Arduino D-2 (INT4)
    Pin const PinE5 = {&DDRE, &PINE, &PORTE, _BV(PE5), ExtInterrupt::EXTINT_INT5};  // Arduino D-3 (INT5)
    Pin const PinE6 = {&DDRE, &PINE, &PORTE, _BV(PE6), ExtInterrupt::EXTINT_INT6};  // (INT6)
    Pin const PinE7 = {&DDRE, &PINE, &PORTE, _BV(PE7), ExtInterrupt::EXTINT_INT7};  // (INT7)

    // PORT-F

    Pin const PinF0 = {&DDRF, &PINF, &PORTF, _BV(PF0), ExtInterrupt::EXTINT_NONE, 0};  // Arduino A-0 (ADC0)
    Pin const PinF1 = {&DDRF, &PINF, &PORTF, _BV(PF1), ExtInterrupt::EXTINT_NONE, 1};  // Arduino A-1 (ADC1)
    Pin const PinF2 = {&DDRF, &PINF, &PORTF, _BV(PF2), ExtInterrupt::EXTINT_NONE, 2};  // Arduino A-2 (ADC2)
    Pin const PinF3 = {&DDRF, &PINF, &PORTF, _BV(PF3), ExtInterrupt::EXTINT_NONE, 3};  // Arduino A-3 (ADC3)
    Pin const PinF4 = {&DDRF, &PINF, &PORTF, _BV(PF4), ExtInterrupt::EXTINT_NONE, 4};  // Arduino A-4 (ADC4)
    Pin const PinF5 = {&DDRF, &PINF, &PORTF, _BV(PF5), ExtInterrupt::EXTINT_NONE, 5};  // Arduino A-5 (ADC5)
    Pin const PinF6 = {&DDRF, &PINF, &PORTF, _BV(PF6), ExtInterrupt::EXTINT_NONE, 6};  // Arduino A-6 (ADC6)
    Pin const PinF7 = {&DDRF, &PINF, &PORTF, _BV(PF7), ExtInterrupt::EXTINT_NONE, 7};  // Arduino A-7 (ADC7)

    // PORT-G

    Pin const PinG0 = {&DDRG, &PING, &PORTG, _BV(PG0)};  // Arduino D-41
    Pin const PinG1 = {&DDRG, &PING, &PORTG, _BV(PG1)};  // Arduino D-40
    Pin const PinG2 = {&DDRG, &PING, &PORTG, _BV(PG2)};  // Arduino D-39
    Pin const PinG3 = {&DDRG, &PING, &PORTG, _BV(PG3)};
    Pin const PinG4 = {&DDRG, &PING, &PORTG, _BV(PG4)};
    Pin const PinG5 = {&DDRG, &PING, &PORTG, _BV(PG5)};  // Arduino D-4

    // PORT-H

    Pin const PinH0 = {&DDRH, &PINH, &PORTH, _BV(PH0)};  // Arduino D-17 (USART2_RX)
    Pin const PinH1 = {&DDRH, &PINH, &PORTH, _BV(PH1)};  // Arduino D-16 (USART2_TX)
    Pin const PinH2 = {&DDRH, &PINH, &PORTH, _BV(PH2)};
    Pin const PinH3 = {&DDRH, &PINH, &PORTH, _BV(PH3)};  // Arduino D-6
    Pin const PinH4 = {&DDRH, &PINH, &PORTH, _BV(PH4)};  // Arduino D-7
    Pin const PinH5 = {&DDRH, &PINH, &PORTH, _BV(PH5)};  // Arduino D-8
    Pin const PinH6 = {&DDRH, &PINH, &PORTH, _BV(PH6)};  // Arduino D-9
    Pin const PinH7 = {&DDRH, &PINH, &PORTH, _BV(PH7)};

    // PORT-J

    Pin const PinJ0 = {&DDRJ, &PINJ, &PORTJ, _BV(PJ0)};  // Arduino D-15 (USART3_RX)
    Pin const PinJ1 = {&DDRJ, &PINJ, &PORTJ, _BV(PJ1)};  // Arduino D-14 (USART3_TX)
    Pin const PinJ2 = {&DDRJ, &PINJ, &PORTJ, _BV(PJ2)};
    Pin const PinJ3 = {&DDRJ, &PINJ, &PORTJ, _BV(PJ3)};
    Pin const PinJ4 = {&DDRJ, &PINJ, &PORTJ, _BV(PJ4)};
    Pin const PinJ5 = {&DDRJ, &PINJ, &PORTJ, _BV(PJ5)};
    Pin const PinJ6 = {&DDRJ, &PINJ, &PORTJ, _BV(PJ6)};
    Pin const PinJ7 = {&DDRJ, &PINJ, &PORTJ, _BV(PJ7)};

    // PORT-K

    Pin const PinK0 = {&DDRK, &PINK, &PORTK, _BV(PK0), ExtInterrupt::EXTINT_NONE, 8};   // Arduino A-8 (ADC8)
    Pin const PinK1 = {&DDRK, &PINK, &PORTK, _BV(PK1), ExtInterrupt::EXTINT_NONE, 9};   // Arduino A-9 (ADC9)
    Pin const PinK2 = {&DDRK, &PINK, &PORTK, _BV(PK2), ExtInterrupt::EXTINT_NONE, 10};  // Arduino A-10 (ADC10)
    Pin const PinK3 = {&DDRK, &PINK, &PORTK, _BV(PK3), ExtInterrupt::EXTINT_NONE, 11};  // Arduino A-11 (ADC11)
    Pin const PinK4 = {&DDRK, &PINK, &PORTK, _BV(PK4), ExtInterrupt::EXTINT_NONE, 12};  // Arduino A-12 (ADC12)
    Pin const PinK5 = {&DDRK, &PINK, &PORTK, _BV(PK5), ExtInterrupt::EXTINT_NONE, 13};  // Arduino A-13 (ADC13)
    Pin const PinK6 = {&DDRK, &PINK, &PORTK, _BV(PK6), ExtInterrupt::EXTINT_NONE, 14};  // Arduino A-14 (ADC14)
    Pin const PinK7 = {&DDRK, &PINK, &PORTK, _BV(PK7), ExtInterrupt::EXTINT_NONE, 15};  // Arduino A-15 (ADC15)

    // PORT-L

    Pin const PinL0 = {&DDRL, &PINL, &PORTL, _BV(PL0)};  // Arduino D-49
    Pin const PinL1 = {&DDRL, &PINL, &PORTL, _BV(PL1)};  // Arduino D-48
    Pin const PinL2 = {&DDRL, &PINL, &PORTL, _BV(PL2)};  // Arduino D-47
    Pin const PinL3 = {&DDRL, &PINL, &PORTL, _BV(PL3)};  // Arduino D-46
    Pin const PinL4 = {&DDRL, &PINL, &PORTL, _BV(PL4)};  // Arduino D-45
    Pin const PinL5 = {&DDRL, &PINL, &PORTL, _BV(PL5)};  // Arduino D-44
    Pin const PinL6 = {&DDRL, &PINL, &PORTL, _BV(PL6)};  // Arduino D-43
    Pin const PinL7 = {&DDRL, &PINL, &PORTL, _BV(PL7)};  // Arduino D-42

    // Arduino Ethernet/SD shield (v.05)

    #define PinArduinoETHERNET_CS               PinB4  /* CS of W5100  (D-10)  */
    #define PinArduinoSDCARD_CS                 PinG5  /* CS of SDCARD (D-4)   */
    #define PinArduinoSDCARD_WRITEPROTECTSENSOR PinF0  /* write protect sensor */
    #define PinArduinoSDCARD_PLUGGEDINSENSOR    PinF1  /* plugged in sensor    */


  #elif defined (FDV_ATTINY84)

    // PORT-A

    Pin const PinA0 = {&DDRA, &PINA, &PORTA, _BV(PA0), ExtInterrupt::EXTINT_NONE, 0};  // ADC0
    Pin const PinA1 = {&DDRA, &PINA, &PORTA, _BV(PA1), ExtInterrupt::EXTINT_NONE, 1};  // ADC1
    Pin const PinA2 = {&DDRA, &PINA, &PORTA, _BV(PA2), ExtInterrupt::EXTINT_NONE, 2};  // ADC2
    Pin const PinA3 = {&DDRA, &PINA, &PORTA, _BV(PA3), ExtInterrupt::EXTINT_NONE, 3};  // ADC3
    Pin const PinA4 = {&DDRA, &PINA, &PORTA, _BV(PA4), ExtInterrupt::EXTINT_NONE, 4};  // ADC4
    Pin const PinA5 = {&DDRA, &PINA, &PORTA, _BV(PA5), ExtInterrupt::EXTINT_NONE, 5};  // ADC5
    Pin const PinA6 = {&DDRA, &PINA, &PORTA, _BV(PA6), ExtInterrupt::EXTINT_NONE, 6};  // ADC6
    Pin const PinA7 = {&DDRA, &PINA, &PORTA, _BV(PA7), ExtInterrupt::EXTINT_NONE, 7};  // ADC7

    // PORT-B

    Pin const PinB0 = {&DDRB, &PINB, &PORTB, _BV(PB0)};
    Pin const PinB1 = {&DDRB, &PINB, &PORTB, _BV(PB1)};
    Pin const PinB2 = {&DDRB, &PINB, &PORTB, _BV(PB2), ExtInterrupt::EXTINT_INT0};    // INT0
    Pin const PinB3 = {&DDRB, &PINB, &PORTB, _BV(PB3)};


  #elif defined (FDV_ATTINY85)

    // PORT-B

    Pin const PinB0 = {&DDRB, &PINB, &PORTB, _BV(PB0)};
    Pin const PinB1 = {&DDRB, &PINB, &PORTB, _BV(PB1)};
    Pin const PinB2 = {&DDRB, &PINB, &PORTB, _BV(PB2), ExtInterrupt::EXTINT_INT0,    1};  // INT0, ADC1
    Pin const PinB3 = {&DDRB, &PINB, &PORTB, _BV(PB3), ExtInterrupt::EXTINT_NONE, 3};  // ADC3
    Pin const PinB4 = {&DDRB, &PINB, &PORTB, _BV(PB4), ExtInterrupt::EXTINT_NONE, 2};  // ADC2
    Pin const PinB5 = {&DDRB, &PINB, &PORTB, _BV(PB5), ExtInterrupt::EXTINT_NONE, 0};  // ADC0 (RESET)

  #else

     #error Undefined processor

  #endif


}



#endif /* FDV_PORTS_H_ */
