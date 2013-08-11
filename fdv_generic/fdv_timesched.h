// 2010 by Fabrizio Di Vittorio (fdivitto@tiscali.it)


#ifndef FDV_TIMESCHED_H
#define FDV_TIMESCHED_H


#include <stdlib.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#include <avr/eeprom.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <util/atomic.h>


#include "fdv_vector.h"
#include "fdv_platform.h"
#include "fdv_pin.h"




namespace fdv
{
  
  // forward
  inline uint32_t millis();
  
  
  // realtime support
  extern uint32_t volatile s_seconds;
  extern uint32_t          s_LastSeconds;
  extern uint32_t volatile s_overflowCount;
  extern uint32_t volatile s_millis;
  extern uint8_t volatile  s_fract;
  
  // measurePulse support
  extern uint8_t volatile  s_specialMeasure;      // 0 = nop,  1 = measure pulse,   2 = delay millis (delay() support)
  extern uint32_t volatile s_specialMeasureValue; 
  
  

  
  // ITaskCallable interface
  struct ITaskCallable
  {
    virtual void task(uint8_t taskIndex) = 0;
  };
  

  ////////////////////////////////////////////////////////////////
  // Task
  // Single operation handled by TaskManager
  struct Task
  {
    typedef void (*FuncT)(uint8_t); // params: taskIndex
  	
    static uint8_t const FLAG_INSIDEINTERRUPT = 0b00000001; // 1 = allow execution inside an ISR (interrupt service routine). 0 = do not execute inside the ISR. You have to call TaskManager::schedule() manually
    static uint8_t const FLAG_FUNCTION        = 0b00000010; // 1 = exec is FuncT pointer,  0 = exec is ITaskCallable pointer
    
    uint32_t volatile m_lastExecution;  // time of last execution in ms
    uint32_t volatile m_everyMillisecs; // time interval in ms
    void*    volatile m_exec;           // can be "FuncT" or "ITaskCallable*"
    uint8_t  volatile m_flags;
    

    bool getFlag(uint8_t flag) volatile const
    {
      return m_flags & flag;
    }
    
    void setFlag(uint8_t flag, bool value) volatile
    {
      m_flags = value? (m_flags | flag) : (m_flags & ~flag);
    }
    
    void exec(uint8_t taskIndex) volatile const
    {
      if (getFlag(FLAG_FUNCTION))
        ((FuncT)m_exec)(taskIndex);
      else
        ((ITaskCallable*)m_exec)->task(taskIndex);      
    }
  };
  
  
  
  
  ////////////////////////////////////////////////////////////////
  // TaskManager
  // Timed operations support
  class TaskManager
  {
    
  private:
    
    #if defined(FDV_ATMEGA88_328)
      static uint8_t const MAXTASKS = 10;
    #elif defined(FDV_ATMEGA1280_2560)
      static uint8_t const MAXTASKS = 10;
    #elif defined(FDV_ATTINY84) || defined(FDV_ATTINY85)
      static uint8_t const MAXTASKS = 3;
    #else
      #error Undefined MCU
    #endif

    static Task volatile s_info[MAXTASKS];
    
  public:
    
        
    
    static void init()
    {      
      static bool ls_initialized = false;

      if (!ls_initialized)
      {
        memset((void*)&s_info[0], 0, sizeof(Task) * MAXTASKS);
        
        // set timer 0 prescale factor to 64 (at 16Mhz timer0 will increase every 4uS [1(/16000000/64)])
        TCCR0B |= (1 << CS00) | (1 << CS01);

        // enable timer 0 overflow interrupt (at 16Mhz an overflow interrupt every 1024uS [4us*256])
        #if defined(FDV_ATTINY85)
          TIMSK |= (1 << TOIE0);
        #else
          TIMSK0 |= (1 << TOIE0);
        #endif

        // enable interrupts
        sei();

        ls_initialized = true;
      }
    }
    
    
    static void schedule(uint32_t currentMillis, bool isInterrupt = false)
    {
      for (uint8_t i = 0; i != MAXTASKS; ++i)
      {
        Task volatile& task = s_info[i];
        if (task.m_exec != NULL && isInterrupt == task.getFlag(Task::FLAG_INSIDEINTERRUPT))
        {
          uint32_t timeToExec = task.m_lastExecution + task.m_everyMillisecs;
          if (timeToExec > task.m_lastExecution && currentMillis >= timeToExec)  // first cond checks overflow
          {
            task.exec(i);
            task.m_lastExecution = currentMillis;
          }  		
        }
      }        
    }
    
    
    // adds a function task and/or callable object
    static uint8_t add(uint32_t everyMillisecs, Task::FuncT func, ITaskCallable* taskCallableObj, bool insideInterrupt)
    {
      for (uint8_t i = 0; i != MAXTASKS; ++i)
        if (s_info[i].m_exec == NULL)
        {
          set(i, everyMillisecs, func, taskCallableObj, insideInterrupt);
          return i;
        }
      return 0xFF;  // fail!
    } 
    

    // replaces a task (doesn't check index)
    static void set(uint8_t taskIndex, uint32_t everyMillisecs, Task::FuncT func, ITaskCallable* taskCallableObj, bool insideInterrupt)
    {
      if (taskIndex != 0xFF)
      {
        ATOMIC_BLOCK(ATOMIC_RESTORESTATE) // no interrupts
        {
          s_info[taskIndex].m_everyMillisecs  = everyMillisecs;
          s_info[taskIndex].m_exec            = (func? (void*)func : (void*)taskCallableObj);
          s_info[taskIndex].m_lastExecution   = millis();
          s_info[taskIndex].m_flags           = 0;
          s_info[taskIndex].setFlag(Task::FLAG_FUNCTION, func != NULL);
          s_info[taskIndex].setFlag(Task::FLAG_INSIDEINTERRUPT, insideInterrupt);
        }
      }        
    }
    
    static Task volatile& get(uint8_t taskIndex)
    {
      return s_info[taskIndex];
    }    
        
    // removes a task (does nothing if taskIndex==0xFF)
    static void remove(uint8_t taskIndex)
    {
      set(taskIndex, 0, NULL, NULL, false); // "set" supports the case taskIndex==0xFF
    }
    
  };
  
  
    
  ////////////////////////////////////////////////////////////////
  // seconds()
  // returns elapsed seconds. Reset after 4294967296 secs = 71582788 mins = 1193046 hours = 49710 days = 136 years
  inline uint32_t seconds()
  {
    if (s_LastSeconds != s_seconds) // to avoid to disable interrupts
    {
      ATOMIC_BLOCK(ATOMIC_RESTORESTATE) // no interrupts
      {
        s_LastSeconds = s_seconds;
      }
    }
    return s_LastSeconds;
  }
  
  
  ////////////////////////////////////////////////////////////////
  // delay()
  // delay milliseconds using timer 0 interrupt
  inline void delay(uint32_t millisecs)
  {
    uint32_t m = millis();
    s_specialMeasureValue = m + millisecs;
    
    if (s_specialMeasureValue < m)  // overflow?
    {
      s_specialMeasureValue = 0xFFFFFFFF;
      millisecs -= 0xFFFFFFFF - m;
    }
    else
      millisecs = 0;
    
    s_specialMeasure = 2;
    while (s_specialMeasure == 2); // delay wait cycle
		
    // handle overflow
    if (millisecs > 0)
    {
      while (millis() == 0xFFFFFFFF);
      delay(millisecs); // delay wait cicle
    }
  }
  
  
  ////////////////////////////////////////////////////////////////
  // millisDiff()
  // calculates time difference in milliseconds, taking into consideration the time overflow
  // note: time1 must be less than time2 (time1 < time2)
  inline uint32_t millisDiff(uint32_t time1, uint32_t time2)
  {
    if (time1 > time2)
      // overflow
      return 0xFFFFFFFF - time1 + time2;
    else
      return time2 - time1;
  }
  

  ////////////////////////////////////////////////////////////////
  // millis()
  inline uint32_t millis()
  {
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) // no interrupts
    {
      return s_millis;
    }
    return 0;   // avoid compiler warning
  }


  ////////////////////////////////////////////////////////////////
  // micros()
  inline uint32_t micros()
  {
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) // no interrupts
    {
      uint32_t m = s_overflowCount;
      uint8_t t = TCNT0;
      #if defined(FDV_ATTINY85)
        if ((TIFR & _BV(TOV0)) && (t < 255)) ++m;
      #else
        if ((TIFR0 & _BV(TOV0)) && (t < 255)) ++m;
      #endif
      return ((m << 8) + t) * (64 / (F_CPU/1000000L));
    }
    return 0;   // avoid compiler warning
  }


  ////////////////////////////////////////////////////////////////
  // Delay for the given number of microseconds without using interrupts
  inline void delayMicroseconds(uint32_t us)
  {
    #if F_CPU == 16000000L
      if (--us == 0)
        return;
      us <<= 2;
      us -= 2;
    #elif F_CPU == 8000000L
      if (--us == 0)
        return;
      if (--us == 0)
        return;
      us <<= 1;
      us--;
    #else
      #error Unsupported F_CPU
    #endif
    // busy wait
    __asm__ __volatile__ (
      "1: sbiw %0,1" "\n\t" // 2 cycles
      "brne 1b" : "=w" (us) : "0" (us) // 2 cycles
    );
  }


  ////////////////////////////////////////////////////////////////
  // delay_ms
  inline void delay_ms(uint32_t ms)
  {
    while(ms)
    {
      _delay_ms(0.96);
      ms--;
    }
  }


  ////////////////////////////////////////////////////////////////
  // measurePulseValue()
  inline uint32_t measurePulseValue()
  {
	  ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	  {
		  return s_specialMeasureValue;
	  }
	  return 0;		  
  }


  ////////////////////////////////////////////////////////////////
  // measurePulse
  // returns pulse length in microseconds (uS)
  // Resolution is 16uS
  // timeout measured in microseconds (uS)
  inline uint32_t measurePulse(Pin const* pin, uint8_t state, uint32_t timeout)
  {
	  static uint8_t const RESOLUTION = 16; // in uS
	  
	  ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	  {
	    // save timer0 prescaler value
		  uint8_t const prevTCCR0B = TCCR0B;
		  
		  // set timer0 prescaler to 1 (no prescaling)
		  // each interrupt count (1/16000000)*256 = 16uS
		  TCCR0B &= ~(1 << CS00) & ~(1 << CS01) & ~(1 << CS02);
		  TCCR0B |= (1 << CS00);
      s_specialMeasure = 1;
	    s_specialMeasureValue = 0;
		  timeout /= RESOLUTION;	    	  
      
	    sei(); // IRQ enabled
	  
	    // wait for the pulse to start
	    while (pin->read() != state && timeout > measurePulseValue());
	  
	    // wait for the pulse to stop	    
		  cli();  // IRQ disabled
		  uint32_t const t0 = s_specialMeasureValue;
		  sei();  // IRQ enabled
	    while (pin->read() == state && timeout > measurePulseValue());

      // interrupts disabled
		  cli();  // IRQ disabled

      // cleanup		  
		  s_specialMeasure = false;
		  TCCR0B = prevTCCR0B;
		  
		  uint32_t const ret = measurePulseValue();		  
		  return (timeout > ret? (ret-t0) : ret) * RESOLUTION;
	  }
  }
  
  
  
  
  ////////////////////////////////////////////////////////////////
  // TimeOut class (uses interrupts and taskmanager)
  // ex. TimeOut(200)  <- after 200ms TimeOut() returns true

  class TimeOut
  {    
    
  public:
  
    TimeOut(uint32_t time)
    {
      m_taskIndex = TaskManager::add(time, timeOutFunc, NULL, true);
    }
    
    ~TimeOut()
    {
      TaskManager::remove(m_taskIndex); // remove accepts m_taskIndex==0xFF
    }
    
    operator bool()
    {
      return m_taskIndex == 0xFF || TaskManager::get(m_taskIndex).m_everyMillisecs == 0xFFFFFFFF;
    }
    
    void reset(uint32_t time)
    {
      TaskManager::remove(m_taskIndex);  // remove accepts m_taskIndex==0xFF
      m_taskIndex = TaskManager::add(time, timeOutFunc, NULL, true);
    }
  
  private:  
  
    static void timeOutFunc(uint8_t taskIndex);

    uint8_t m_taskIndex;
  };

  
  ////////////////////////////////////////////////////////////////
  // SoftTimeOut class
  // ex. SoftTimeOut(200)  <- after 200ms SoftTimeOut() returns true
  // note: disable interrupts for each "bool()" call!

  class SoftTimeOut
  {
    
  public:
    
    SoftTimeOut(uint32_t time)
      : m_timeOut(time), m_startTime(millis())
    {
    }
    
    operator bool()
    {
      return millisDiff(m_startTime, millis()) > m_timeOut;
    }
    
    void reset(uint32_t time)
    {
      m_timeOut   = time;
      m_startTime = millis();
    }
    
  private:
    
    uint32_t m_timeOut;
    uint32_t m_startTime;
  };

  


  
} // fdv namespace



#endif  // FDV_TIMESCHED_H

