// 2011 by Fabrizio Di Vittorio (fdivitto@tiscali.it)

#ifndef FDV_PLATFORM_H_
#define FDV_PLATFORM_H_


#if defined (__AVR_ATmega328P__) || defined (__AVR_ATmega328__) || defined (__AVR_ATmega168__) || defined (__AVR_ATmega168P__) || defined (__AVR_ATmega88__) || defined (__AVR_ATmega88P__)
  #define FDV_ATMEGA88_328
#endif

#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
  #define FDV_ATMEGA1280_2560
#endif

#if defined(__AVR_ATtiny84__) || defined(__AVR_ATtiny84A__)
  #define FDV_ATTINY84
#endif

#if defined(__AVR_ATtiny85__)
  #define FDV_ATTINY85
#endif


#endif /* FDV_PLATFORM_H_ */
