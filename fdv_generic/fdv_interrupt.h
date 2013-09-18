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




#ifndef FDV_INTERRUPT_H_
#define FDV_INTERRUPT_H_

#include <stddef.h>
#include <util/atomic.h>

#include "fdv_platform.h"


namespace fdv
{



  // handles PC external interrupts (ie PCINT0-23 on 328)
  struct PCExtInterrupt
  {
    #if defined(FDV_ATMEGA88_328)
      static uint8_t const PCEXTERNAL_NUM_INTERRUPTS = 24;
      static uint8_t const PCEXTERNAL_NUM_GROUPS     = 3;
    #else
      #error unknown MCU
    #endif

    struct IExtInterruptCallable
    {
      virtual void extInterrupt() = 0;
    };

    // for extint parameter of attach()
    enum ExtInt
    {
      #if defined(FDV_ATMEGA88_328)        
        PCEXTINT_INT0     = 0,
        PCEXTINT_INT1     = 1,
        PCEXTINT_INT2     = 2,
        PCEXTINT_INT3     = 3,
        PCEXTINT_INT4     = 4,
        PCEXTINT_INT5     = 5,
        PCEXTINT_INT6     = 6,
        PCEXTINT_INT7     = 7,
        PCEXTINT_INT8     = 8,
        PCEXTINT_INT9     = 9,
        PCEXTINT_INT10    = 10,
        PCEXTINT_INT11    = 11,
        PCEXTINT_INT12    = 12,
        PCEXTINT_INT13    = 13,
        PCEXTINT_INT14    = 14,
        PCEXTINT_INT16    = 16, // ...15 doesn't exist!
        PCEXTINT_INT17    = 17,
        PCEXTINT_INT18    = 18,
        PCEXTINT_INT19    = 19,
        PCEXTINT_INT20    = 20,
        PCEXTINT_INT21    = 21,
        PCEXTINT_INT22    = 22,
        PCEXTINT_INT23    = 23
      #else
        #error unknown MCU
      #endif
    };
    
    // grouped for s_callableObjects (actually atmel groups pcint in three groups and we cannot distinguish single pins ints)
    enum ExtIntGroups
    {
      #if defined(FDV_ATMEGA88_328)        
        PCEXTINT_0_7      = 0,
        PCEXTINT_8_14     = 1,
        PCEXTINT_16_23    = 2
      #else
        #error unknown MCU
      #endif
    };

    static IExtInterruptCallable* s_callableObjects[PCEXTERNAL_NUM_GROUPS]; // should be zero init

    static void attach(ExtInt extint, IExtInterruptCallable* extInterruptCallable)
    {
      ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
      {        

        #if defined(FDV_ATMEGA88_328)
          if (extint >= PCEXTINT_INT0 && extint <= PCEXTINT_INT7)
          {
            s_callableObjects[PCEXTINT_0_7] = extInterruptCallable;
            PCICR |= (1 << PCIE0);
            PCMSK0 |= (1 << extint);
          }
          else if (extint >= PCEXTINT_INT8 && extint <= PCEXTINT_INT14)
          {
            s_callableObjects[PCEXTINT_8_14] = extInterruptCallable;
            PCICR |= (1 << PCIE1);
            PCMSK1 |= (1 << (extint - PCEXTINT_INT8));
          }
          else if (extint >= PCEXTINT_INT16 && extint <= PCEXTINT_INT23)
          {
            s_callableObjects[PCEXTINT_16_23] = extInterruptCallable;
            PCICR |= (1 << PCIE2);
            PCMSK2 |= (1 << (extint - PCEXTINT_INT16));
          }
        #else
          #error Unknown MCU
        #endif
      }
    }


  };



  // handles external interrupts (ie INT0/INT1 on 328)
  struct ExtInterrupt
  {

    #if defined(FDV_ATMEGA1280_2560)
      static uint8_t const EXTERNAL_NUM_INTERRUPTS = 8;
    #elif defined(FDV_ATMEGA88_328)
      static uint8_t const EXTERNAL_NUM_INTERRUPTS = 2;
    #elif defined(FDV_ATTINY84)
      static uint8_t const EXTERNAL_NUM_INTERRUPTS = 1;
	  #elif defined(FDV_ATTINY85)
	    static uint8_t const EXTERNAL_NUM_INTERRUPTS = 1;
    #else
      #error unknown MCU
    #endif


    struct IExtInterruptCallable
    {
      virtual void extInterrupt() = 0;
    };


    enum Mode
    {
      EXTINT_CHANGE   = 1,
      EXTINT_FALLING  = 2,
      EXTINT_RISING   = 3
    };


    enum ExtInt
    {
      #if defined(FDV_ATMEGA1280_2560)
        EXTINT_INT0     = 0,
        EXTINT_INT1     = 1,
        EXTINT_INT2     = 2,
        EXTINT_INT3     = 3,
        EXTINT_INT4     = 4,
        EXTINT_INT5     = 5,
        EXTINT_INT6     = 6,
        EXTINT_INT7     = 7,
      #elif defined(FDV_ATMEGA88_328)
        EXTINT_INT0     = 0,
        EXTINT_INT1     = 1,
      #elif defined(FDV_ATTINY84)
        EXTINT_INT0     = 0,
		  #elif defined(FDV_ATTINY85)
		    EXTINT_INT0     = 0,
      #else
        #error unknown MCU
      #endif
      EXTINT_NONE = 0xFF
    };


    static IExtInterruptCallable* s_callableObjects[EXTERNAL_NUM_INTERRUPTS]; // should be zero init


    static void attach(ExtInt extint, IExtInterruptCallable* extInterruptCallable, Mode mode)
    {
      ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
      {
        s_callableObjects[extint] = extInterruptCallable;

        switch (extint)
        {
          #if defined(FDV_ATMEGA1280_2560)
            case EXTINT_INT0:
              EICRA = (EICRA & ~((1 << ISC00) | (1 << ISC01))) | (mode << ISC00);
              EIMSK |= (1 << INT0);
              break;
            case EXTINT_INT1:
              EICRA = (EICRA & ~((1 << ISC10) | (1 << ISC11))) | (mode << ISC10);
              EIMSK |= (1 << INT1);
              break;
            case EXTINT_INT2:
              EICRA = (EICRA & ~((1 << ISC20) | (1 << ISC21))) | (mode << ISC20);
              EIMSK |= (1 << INT2);
              break;
            case EXTINT_INT3:
              EICRA = (EICRA & ~((1 << ISC30) | (1 << ISC31))) | (mode << ISC30);
              EIMSK |= (1 << INT3);
              break;
            case EXTINT_INT4:
              EICRB = (EICRB & ~((1 << ISC40) | (1 << ISC41))) | (mode << ISC40);
              EIMSK |= (1 << INT4);
              break;
            case EXTINT_INT5:
              EICRB = (EICRB & ~((1 << ISC50) | (1 << ISC51))) | (mode << ISC50);
              EIMSK |= (1 << INT5);
              break;
            case EXTINT_INT6:
              EICRB = (EICRB & ~((1 << ISC60) | (1 << ISC61))) | (mode << ISC60);
              EIMSK |= (1 << INT6);
              break;
            case EXTINT_INT7:
              EICRB = (EICRB & ~((1 << ISC70) | (1 << ISC71))) | (mode << ISC70);
              EIMSK |= (1 << INT7);
              break;
          #elif defined(FDV_ATMEGA88_328)
            case EXTINT_INT0:
              EICRA = (EICRA & ~((1 << ISC00) | (1 << ISC01))) | (mode << ISC00);
              EIMSK |= (1 << INT0);
              break;
            case EXTINT_INT1:
              EICRA = (EICRA & ~((1 << ISC10) | (1 << ISC11))) | (mode << ISC10);
              EIMSK |= (1 << INT1);
              break;
          #elif defined(FDV_ATTINY84)
            case EXTINT_INT0:
              MCUCR = (MCUCR & ~((1 << ISC00) | (1 << ISC01))) | (mode << ISC00);
              GIMSK |= _BV(INT0);
              break;
          #elif defined(FDV_ATTINY85)
            case EXTINT_INT0:
              MCUCR = (MCUCR & ~((1 << ISC00) | (1 << ISC01))) | (mode << ISC00);
              GIMSK |= _BV(INT0);
              break;
          #else
            #error Unknown MCU
          #endif
           default:
            break;
        }
      }
    }

  };




  /////////////////////////////////////////////////////////////////////////////////////////////
  // Mutex class

  inline uint32_t MutexGet(uint8_t volatile* mutex)
  {
    uint32_t r = 1;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
      r = *mutex;
      if (r == 0)
        *mutex = 1;
    }
    return r;
  }


  class Mutex
  {
    public:

      Mutex()
       : m_mutex(0)
      {
      }

      void lock() volatile
      {
        while (!tryLock());
      }

      bool tryLock() volatile
      {
        return MutexGet(&m_mutex) == 0;
      }

      void unlock() volatile
      {
        m_mutex = 0;
      }

    private:

      uint8_t volatile m_mutex;
  };

  /////////////////////////////////////////////////////////////////////////////////////////////
  // Mutex automatic lock/unlock helper

  class MutexLock
  {
    public:
      MutexLock(Mutex& mutex, bool enabled = true)
        : m_mutex(enabled? &mutex : NULL)
      {
        if (m_mutex)       
          m_mutex->lock();
      }

      ~MutexLock()
      {
        if (m_mutex)
          m_mutex->unlock();
      }

    private:
      Mutex* m_mutex;
  };


  /////////////////////////////////////////////////////////////////////////////////////////////
  // Mutex automatic trylock/unlock helper

  class MutexTryLock
  {
    public:
      MutexTryLock(Mutex& mutex)
        : m_mutex(mutex), m_acquired(false)
      {
        m_acquired = m_mutex.tryLock();
      }

      ~MutexTryLock()
      {
        if (m_acquired)
          m_mutex.unlock();
      }

      bool acquired()
      {
        return m_acquired;
      }

    private:
      Mutex& m_mutex;
      bool   m_acquired;
  };





} // end of fdv namespace


#endif /* FDV_INTERRUPT_H_ */
