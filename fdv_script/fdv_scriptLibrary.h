// 2010 by Fabrizio Di Vittorio (fdivitto@tiscali.it)

#ifndef FDV_SCRIPTLIBRARY_H_
#define FDV_SCRIPTLIBRARY_H_


#include <string.h>
#include <stdlib.h>
#include <alloca.h>
#include <inttypes.h>
#include <avr/pgmspace.h>

#include "../fdv_generic/fdv_timesched.h"
#include "../fdv_generic/fdv_analog.h"
#include "../fdv_generic/fdv_log.h"
#include "../fdv_sdlib/fdv_sdcard.h"
#include "../fdv_sdlib/fdv_ini.h"
#include "../fdv_script/fdv_scriptSchedule.h"


#define FDV_SCRIPT_SUPPORT_FILES
#define FDV_SCRIPT_SUPPORT_RFLINK
#define FDV_SCRIPT_SUPPORT_PORTS
#define FDV_SCRIPT_SUPPORT_ARRAY
#define FDV_SCRIPT_SUPPORT_STRINGS
#define FDV_SCRIPT_SUPPORT_DATETIME
#define FDV_SCRIPT_SUPPORT_FILESYSTEM


namespace fdv
{



  ///////////////////////////////////////////////////////////////////////////////////////////////
  // ScriptLibrary

  struct ScriptLibrary
  {

    ScriptLibrary(FileSystem* fileSystem) :
      m_fileSystem(fileSystem)
    {
    }


  private:

    #ifdef FDV_SCRIPT_SUPPORT_PORTS

    // portname = 'A', 'B', 'K'...
    // returns associated DDR (DDRA, DDRB, DDRK...)
    static volatile uint8_t* portNameToDDR(char portname)
    {
      volatile uint8_t* CV[] = {&DDRA, &DDRB, &DDRC, &DDRD, &DDRE, &DDRF, &DDRG, &DDRH, &DDRH, &DDRJ, &DDRK, &DDRL};  // DDRH=DDRI
      return CV[portname-65];
    }

    // portname = 'A', 'B', 'K', ...
    // returns associated PIN (PINA, PINB, PINK...)
    static volatile uint8_t* portNameToPIN(char portname)
    {
      volatile uint8_t* CV[] = {&PINA, &PINB, &PINC, &PIND, &PINE, &PINF, &PING, &PINH, &PINH, &PINJ, &PINK, &PINL};  // PINH=PINI
      return CV[portname-65];
    }

    // portname = 'A', 'B', 'K', ...
    // returns associated PORT (PORTA, PORTB, PORTK...)
    static volatile uint8_t* portNameToPORT(char portname)
    {
      volatile uint8_t* CV[] = {&PORTA, &PORTB, &PORTC, &PORTD, &PORTE, &PORTF, &PORTG, &PORTH, &PORTH, &PORTJ, &PORTK, &PORTL};  // PORTH=PORTI
      return CV[portname-65];
    }

    #endif // FDV_SCRIPT_SUPPORT_PORTS


    // checks also params count
    template <typename RunTimeT>
    bool checkParamsType(RunTimeT& runtime, uint8_t index, uint8_t type)
    {
      return runtime.vars.size() > index && (runtime.vars[index].value.type() | type) != 0; // logic OR to accept multiple types
    }


  public:

    template <typename RunTimeT>
    bool execFunc(char const* funcName, RunTimeT& runtime, Variant* result)
    {


      typedef typename RunTimeT::LibraryType LibraryType;
      typedef typename RunTimeT::OutputType  OutputType;



      //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
      //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
      // SYSTEM

      // UINT32 = availmem()
      // Returns free heap
      if (strcmp_P(funcName, PSTR("availmem"))==0)
      {
        result->uint32Val() = getFreeMem();
        return true;
      }

      #ifdef FDV_SCRIPT_SUPPORT_PORTS

      // pinmode_output(STRING pinname)
      // Sets specified pin as output
      // pinname : "B0", "A1", ...
      if (strcmp_P(funcName, PSTR("pinmode_output"))==0)
      {
        if (!checkParamsType(runtime, 0, Variant::STRING))
          return false;
        string& pinname = runtime.vars[0].value.stringVal();
        *portNameToDDR(pinname[0]) |= _BV(pinname[1]-48);
        return true;
      }

      // pinmode_input(STRING pinname)
      // Sets specified pin as input
      // pinname : "B0", "A1", etc...
      if (strcmp_P(funcName, PSTR("pinmode_input"))==0)
      {
        if (!checkParamsType(runtime, 0, Variant::STRING))
          return false;
        string& pinname = runtime.vars[0].value.stringVal();
        *portNameToDDR(pinname[0]) &= ~_BV(pinname[1]-48);
        return true;
      }

      // UINT8 = pinread(STRING pinname)
      // Reads specified digital pin. 0=low 1=high
      // pinname : "B0", "A1", etc...
      if (strcmp_P(funcName, PSTR("pinread"))==0)
      {
        if (!checkParamsType(runtime, 0, Variant::STRING))
          return false;
        string& pinname = runtime.vars[0].value.stringVal();
        result->uint8Val() = (*portNameToPIN(pinname[0]) & _BV(pinname[1]-48))? 1 : 0;
        return true;
      }

      // pinwrite(STRING pinname, UINT8 value)
      // Writes to specified digital pin.
      // pinname : "B0", "A1", etc...
      // value : 0 (low),  1 (high)
      if (strcmp_P(funcName, PSTR("pinwrite"))==0)
      {
        if (!checkParamsType(runtime, 0, Variant::STRING) || !checkParamsType(runtime, 1, Variant::UINT8))
          return false;
        string& pinname = runtime.vars[0].value.stringVal();
        uint8_t value = runtime.vars[1].value.uint8Val();
        if (value==0)
          *portNameToPORT(pinname[0]) &= ~_BV(pinname[1]-48);
        else
          *portNameToPORT(pinname[0]) |= _BV(pinname[1]-48);
        return true;
      }

      // UINT16 = analogread(UINT8 pin [, FLOAT reference=5])
      // Reads analog value (0..1023) from specified analog pin (ADC0...)
      // reference:
      //   1.1  = 1.1V
      //   2.56 = 2.56V
      //   5    = VCC   (DEFAULT)
      //   0    = external
      // Remember to call "Analog::init()"!
      if (strcmp_P(funcName, PSTR("analogread"))==0)
      {
        if (!checkParamsType(runtime, 0, Variant::UINT8))
          return false;
        if (runtime.vars.size() == 2 && !checkParamsType(runtime, 1, Variant::FLOAT))
          return false;
        uint8_t pin = runtime.vars[0].value.uint8Val();
        Analog::Reference ref = Analog::REF_VCC;
        if (runtime.vars.size() == 2)
        {
          float f = runtime.vars[1].value.floatVal();
          if (f == 1.1)
            ref = Analog::REF_INTERNAL1V1;
          else if (f == 2.56)
            ref = Analog::REF_INTERNAL2V56;
          else if (f == 5)
            ref = Analog::REF_VCC;
          else if (f == 0)
            ref = Analog::REF_EXTERNAL;
        }
        result->uint16Val() = Analog::read(pin, ref);
        return true;
      }

      // beep(STRING pin, UINT32 duration_ms, UINT32 frequency_hz)
      // Generates a beep
      if (strcmp_P(funcName, PSTR("beep"))==0)
      {
        if (!checkParamsType(runtime, 0, Variant::STRING) || !checkParamsType(runtime, 1, Variant::UINT32) || !checkParamsType(runtime, 2, Variant::UINT32))
          return false;

        string& pin = runtime.vars[0].value.stringVal();
        uint32_t duration_ms = runtime.vars[1].value.toUInt32();
        uint32_t freq_hz = runtime.vars[2].value.toUInt32();

        *portNameToDDR(pin[0]) |= _BV(pin[1]-48); // mode output

        volatile uint8_t* port = portNameToPORT(pin[0]);
        uint8_t msk_lo = ~_BV(pin[1]-48);
        uint8_t msk_hi = _BV(pin[1]-48);

        uint32_t T = 1.0/freq_hz*1000000;  // period in us
        uint32_t periodsCount = duration_ms * 1000 / T;
        T = T / 2;
        cli();
        for (uint32_t i=0; i<periodsCount; ++i)
        {
          *port &= msk_lo;
          delayMicroseconds(T);
          *port |= msk_hi;
          delayMicroseconds(T);
        }
        sei();
        return true;
      }

      #endif // FDV_SCRIPT_SUPPORT_PORTS


      // debug(STRING msg)
      // Sends a debug message
      if (strcmp_P(funcName, PSTR("debug"))==0)
      {
        if (!checkParamsType(runtime, 0, Variant::STRING))
          return false;
        debug << runtime.vars[0].value.stringVal() << ENDL;
        return true;
      }

      // log(STRING msg)
      // Sends log message
      if (strcmp_P(funcName, PSTR("log"))==0)
      {
        if (!checkParamsType(runtime, 0, Variant::STRING))
          return false;
        Log::add( runtime.vars[0].value.stringVal().c_str() );
        return true;
      }

      // STRING = type(ALLTYPES var)
      // Returns variable type:  "S", "U8", "U16", "U32", "F", "A", "R"
      if (strcmp_P(funcName, PSTR("type"))==0)
      {
        if (runtime.vars.size() < 1)
          return false;
        switch (runtime.vars[0].value.type())
        {
          case Variant::STRING:
            result->stringVal() = "S";
            break;
          case Variant::FLOAT:
            result->stringVal() = "F";
            break;
          case Variant::UINT8:
            result->stringVal() = "U8";
            break;
          case Variant::UINT16:
            result->stringVal() = "U16";
            break;
          case Variant::UINT32:
            result->stringVal() = "U32";
            break;
          case Variant::ARRAY:
            result->stringVal() = "A";
            break;
          case Variant::REFERENCE:
            result->stringVal() = "R";
            break;
          default:
            result->stringVal() = "?";
            break;
        }
        return true;
      }

      // UINT32 = varcount()
      // Returns number of existing variables
      if (strcmp_P(funcName, PSTR("varcount"))==0)
      {
        if (runtime.parent != NULL)
          result->uint32Val() = runtime.parent->vars.size();
        return true;
      }

      // ALLTYPES = getvar(UINT8/16/32 varindex)
      // Returns variable at specified index
      // Example:
      //   - file "sum.s":
      //      <? result = getvar(0) + getvar(1); ?>
      //   - file "test.htm":
      //      <? r = call("sum.s", 5, 7); ?>
      if (strcmp_P(funcName, PSTR("getvar"))==0)
      {
        if (!checkParamsType(runtime, 0, Variant::UINT8 | Variant::UINT16 | Variant::UINT32))
          return false;
        uint32_t varindex = runtime.vars[0].value.toUInt32();
        *result = runtime.parent->vars[varindex].value;
        return true;
      }

      // ALLTYPES = call(STRING filename [,...parameters...])
      // Returns value of variable "result", if exists
      // Example:
      //   - file "sum.s":
      //      <? result = getvar(0) + getvar(1); ?>
      //   - file "test.htm":
      //      <? r = call("sum.s", 5, 7); ?>
      if (strcmp_P(funcName, PSTR("call"))==0)
      {
        if (!checkParamsType(runtime, 0, Variant::STRING))
          return false;
        File file(*m_fileSystem, runtime.vars[0].value.stringVal().c_str(), File::MD_READ);
        if (file.isOpen())
        {
          BufferedFileInput input(&file);
          RunTimeT localRunTime(runtime.output, static_cast<LibraryType*>(this), &runtime);
          // add parameters
          for (uint8_t i=1; i<runtime.vars.size(); ++i)
            localRunTime.vars.push_back( runtime.vars[i] );
          // execute script
          bool ret = parseScript(&localRunTime, &input);
          // retrieve a variable named "result" as result value
          if (localRunTime.findVariable("result") != RunTimeT::NOTFOUND)
            *result = *localRunTime.getVariableValue("result");
          return ret;
        }
        return true;
      }


      // ALLTYPES = eval(STRING expression)
      // Evaluate the expression inside the string.
      // Example:
      //    r = eval("1+1");        // r = 2
      //    r = eval("{1, 2, 3}");  // r = {1, 2, 3}
      if (strcmp_P(funcName, PSTR("eval"))==0)
      {
        if (!checkParamsType(runtime, 0, Variant::STRING))
          return false;
        string& exp = runtime.vars[0].value.stringVal();
        if (exp.size() > 0)
        {
          //exp.append("   ");
          //debug << "eval() exp=" << exp << ENDL;
          TextInput input(exp.c_str());
          Script<RunTimeT> script(&runtime, &input);
          return script.parse_expression(result, true);
        }
        return false;
      }


      //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
      //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
      // MATH

      // UINT32/16/8 = trunc(FLOAT value)
      // Converts floating point value to integer
      if (strcmp_P(funcName, PSTR("trunc"))==0)
      {
        if (!checkParamsType(runtime, 0, Variant::FLOAT))
          return false;
        result->shrink( runtime.vars[0].value.toUInt32() );
        return true;
      }


      //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
      //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
      // ARRAY

      #ifdef FDV_SCRIPT_SUPPORT_ARRAY

      // UINT8/UINT16/UINT32 = arraysize(ARRAY* array)
      // Returns size of array. This is the same of "sizeof(array)", but must be called as "arraysize(&array)".
      // Example:
      //    sz = arraysize(&ar);
      if (strcmp_P(funcName, PSTR("arraysize"))==0)
      {
        if (!checkParamsType(runtime, 0, Variant::REFERENCE))
          return false;
        Variant* ref = runtime.vars[0].value.refVal();
        if (ref)
          result->shrink( ref->size() );
        return true;
      }

      // arrayadd(ARRAY* array, ALLTYPES value)
      // Adds a new type. Note that "array" must be a reference to array.
      // Before add elements to array you have to create one assigning an element or creating an empty array using "arr={};".
      // Example:
      //    ar = {};
      //    arrayadd(ar, "hello");
      //    arrayadd(ar, "world");
      //    ar1[0] = "one";
      //    arrayadd(ar1, "two");
      if (strcmp_P(funcName, PSTR("arrayadd"))==0)
      {
        if (runtime.vars.size() < 2)
          return false;
        if (!checkParamsType(runtime, 0, Variant::REFERENCE))
          return false;
        Variant* array = runtime.vars[0].value.refVal();
        if (array)
          array->arrayVal().push_back( runtime.vars[1].value );
        return true;
      }

      // arrayins(ARRAY* array, UINT index, ALLTYPES value)
      // Insert a new type. Note that "array" must be a reference to array.
      if (strcmp_P(funcName, PSTR("arrayins"))==0)
      {
        if (runtime.vars.size() < 3)
          return false;
        if (!checkParamsType(runtime, 0, Variant::REFERENCE) || !checkParamsType(runtime, 1, Variant::UINT8 | Variant::UINT16 | Variant::UINT32))
          return false;
        Variant* array = runtime.vars[0].value.refVal();
        if (array)
          array->arrayVal().insert(array->arrayVal().begin() + runtime.vars[1].value.toUInt32(), runtime.vars[2].value);
        return true;
      }

      // arraydel(ARRAY* array, UINT index)
      // Removes item at index. Note that "array" must be a reference to array.
      if (strcmp_P(funcName, PSTR("arraydel"))==0)
      {
        if (!checkParamsType(runtime, 0, Variant::REFERENCE) || !checkParamsType(runtime, 1, Variant::UINT8 | Variant::UINT16 | Variant::UINT32))
          return false;
        Variant* array = runtime.vars[0].value.refVal();
        if (array)
          array->arrayVal().erase(array->arrayVal().begin() + runtime.vars[1].value.toUInt32());
        return true;
      }

      #endif // FDV_SCRIPT_SUPPORT_ARRAY

      //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
      //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
      // STRINGS

      #ifdef FDV_SCRIPT_SUPPORT_STRINGS

      // STRING = str(ALLTYPES value [, UINT8 padlen [, STRING padchar="0"]])
      // Converts number to string with optional left pad
      if (strcmp_P(funcName, PSTR("str"))==0)
      {
        if (runtime.vars.size() < 1)
          return false;
        if (runtime.vars.size() == 2)
        {
          if (!checkParamsType(runtime, 1, Variant::UINT8))
            return false;
          result->stringVal() = padLeft( runtime.vars[0].value.toString(), '0', runtime.vars[1].value.uint8Val() );
        }
        else if (runtime.vars.size() == 3)
        {
          if (!checkParamsType(runtime, 1, Variant::UINT8) || !checkParamsType(runtime, 2, Variant::STRING))
            return false;
          result->stringVal() = padLeft( runtime.vars[0].value.toString(), runtime.vars[2].value.stringVal()[0], runtime.vars[1].value.uint8Val() );
        }
        else
          result->stringVal() = runtime.vars[0].value.toString();
        return true;
      }

      // STRING = chr(UINT8 value)
      // Converts ascii value to string (single character)
      if (strcmp_P(funcName, PSTR("chr"))==0)
      {
        if (!checkParamsType(runtime, 0, Variant::UINT8))
          return false;
        result->stringVal() = static_cast<char>( runtime.vars[0].value.toUInt32() );
        return true;
      }

      // UINT8/UINT16/UINT32 = strtoint(STRING str)
      // Converts string to int
      if (strcmp_P(funcName, PSTR("strtoint"))==0)
      {
        if (!checkParamsType(runtime, 0, Variant::STRING))
          return false;
        result->shrink( strtoul(&runtime.vars[0].value.stringVal()[0], NULL, 10) );
        return true;
      }

      // FLOAT = strtofloat(STRING str)
      // Converts string to float
      if (strcmp_P(funcName, PSTR("strtofloat"))==0)
      {
        if (!checkParamsType(runtime, 0, Variant::STRING))
          return false;
        result->floatVal() = strtod(&runtime.vars[0].value.stringVal()[0], NULL);
        return true;
      }

      // UINT = strpos(STRING str, STRING substr)
      // Returns position of substring (0=first position) or 0xFFFF if not found
      if (strcmp_P(funcName, PSTR("strpos"))==0)
      {
        if (!checkParamsType(runtime, 0, Variant::STRING) || !checkParamsType(runtime, 1, Variant::STRING))
          return false;
        char const* str = runtime.vars[0].value.stringVal().c_str();
        char const* substr = runtime.vars[1].value.stringVal().c_str();
        char const* pos = strstr(str, substr);
        result->shrink( pos? pos-str : 0xFFFF );
        return true;
      }

      // STRING = strleft(STRING str, UINT count)
      // Returns left count chars of str.
      if (strcmp_P(funcName, PSTR("strleft"))==0)
      {
        if (!checkParamsType(runtime, 0, Variant::STRING) || !checkParamsType(runtime, 1, Variant::UINT8 | Variant::UINT16 | Variant::UINT32))
          return false;
        result->stringVal().assign( runtime.vars[0].value.stringVal(), runtime.vars[1].value.toUInt32() );
        return true;
      }

      #endif // FDV_SCRIPT_SUPPORT_STRINGS

      //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
      //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
      // OUTPUT HANDLING

      // write(ALLTYPES value)
      // Convert value to string and send to the output
      if (strcmp_P(funcName, PSTR("write"))==0)
      {
        if (runtime.vars.size() < 1)
          return false;
        if (runtime.output)
          runtime.output->write( runtime.vars[0].value.toString().c_str() );
        return true;
      }


      //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
      //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
      // DATETIME

      #ifdef FDV_SCRIPT_SUPPORT_DATETIME

      // UINT32 = millis()
      // Returns milliseconds since startup or last timer overflow (about 50 days)
      if (strcmp_P(funcName, PSTR("millis"))==0)
      {
        result->uint32Val() = millis();
        return true;
      }

      // delay(UINT millis)
      // Delays for milliseconds
      if (strcmp_P(funcName, PSTR("delay"))==0)
      {
        if (runtime.vars.size() < 1)
          return false;
        delay( runtime.vars[0].value.toUInt32() );
        return true;
      }

      // UINT32 = time()
      // Returns current Unix timestamp
      // must be updated before 50 days using settime()
      if (strcmp_P(funcName, PSTR("time"))==0)
      {
        result->uint32Val() = DateTime::now().getUnixDateTime();
        return true;
      }

      // settime(UINT32 timestamp)
      // Adjust current date time
      // must be updated before 50 days
      if (strcmp_P(funcName, PSTR("settime"))==0)
      {
        if (!checkParamsType(runtime, 0, Variant::UINT32))
          return false;
        DateTime::adjustNow( DateTime(runtime.vars[0].value.toUInt32()) );
        return true;
      }

      // UINT8 = seconds(UINT32 timestamp)
      // Extracts seconds from timestamp
      if (strcmp_P(funcName, PSTR("seconds"))==0)
      {
        if (!checkParamsType(runtime, 0, Variant::UINT32))
          return false;
        result->uint8Val() = DateTime( runtime.vars[0].value.toUInt32() ).seconds;
        return true;
      }

      // UINT8 = minutes(UINT32 timestamp)
      // Extracts minutes from timestamp
      if (strcmp_P(funcName, PSTR("minutes"))==0)
      {
        if (!checkParamsType(runtime, 0, Variant::UINT32))
          return false;
        result->uint8Val() = DateTime( runtime.vars[0].value.toUInt32() ).minutes;
        return true;
      }

      // UINT8 = hours(UINT32 timestamp)
      // Extracts hours from timestamp
      if (strcmp_P(funcName, PSTR("hours"))==0)
      {
        if (!checkParamsType(runtime, 0, Variant::UINT32))
          return false;
        result->uint8Val() = DateTime( runtime.vars[0].value.toUInt32() ).hours;
        return true;
      }

      // UINT8 = dayofweek(UINT32 timestamp)
      // Extracts seconds from timestamp. 0=sunday...6=saturday
      if (strcmp_P(funcName, PSTR("dayofweek"))==0)
      {
        if (!checkParamsType(runtime, 0, Variant::UINT32))
          return false;
        result->uint8Val() = DateTime( runtime.vars[0].value.toUInt32() ).dayOfWeek();
        return true;
      }

      // UINT8 = day(UINT32 timestamp)
      // Extracts day of month from timestamp
      if (strcmp_P(funcName, PSTR("day"))==0)
      {
        if (!checkParamsType(runtime, 0, Variant::UINT32))
          return false;
        result->uint8Val() = DateTime( runtime.vars[0].value.toUInt32() ).day;
        return true;
      }

      // UINT8 = month(UINT32 timestamp)
      // Extracts month from timestamp
      if (strcmp_P(funcName, PSTR("month"))==0)
      {
        if (!checkParamsType(runtime, 0, Variant::UINT32))
          return false;
        result->uint8Val() = DateTime( runtime.vars[0].value.toUInt32() ).month;
        return true;
      }

      // UINT16 = year(UINT32 timestamp)
      // Extracts year from timestamp
      if (strcmp_P(funcName, PSTR("year"))==0)
      {
        if (!checkParamsType(runtime, 0, Variant::UINT32))
          return false;
        result->uint16Val() = DateTime( runtime.vars[0].value.toUInt32() ).year;
        return true;
      }

      // UINT32 = timecreate(UINT day, UINT month, UINT year, UINT hours, UINT minutes, UINT seconds)
      // Create a timestamp from date time values
      // Example:
      //    timestamp = timecreate(5, 1, 1973, 10, 0, 0);
      if (strcmp_P(funcName, PSTR("timecreate"))==0)
      {
        for (uint8_t i=0; i<6; ++i)
          if (!checkParamsType(runtime, i, Variant::UINT8 | Variant::UINT16 | Variant::UINT32))
            return false;
        DateTime dt( runtime.vars[0].value.toUInt32(),
                     runtime.vars[1].value.toUInt32(),
                     runtime.vars[2].value.toUInt32(),
                     runtime.vars[3].value.toUInt32(),
                     runtime.vars[4].value.toUInt32(),
                     runtime.vars[5].value.toUInt32() );
        result->uint32Val() = dt.getUnixDateTime();
        return true;
      }

      // STRING = timestr(STRING format, UINT32 timestamp)
      // Formats date/time to string
      // format:
      //    'd' : Day of the month, 2 digits with leading zeros (01..31)
      //    'j' : Day of the month without leading zeros (1..31)
      //    'w' : Numeric representation of the day of the week (0=sunday, 6=saturday)
      //    'm' : Numeric representation of a month, with leading zeros (01..12)
      //    'n' : Numeric representation of a month, without leading zeros (1..12)
      //    'Y' : A full numeric representation of a year, 4 digits (1999, 2000...)
      //    'y' : Two digits year (99, 00...)
      //    'H' : 24-hour format of an hour with leading zeros (00..23)
      //    'i' : Minutes with leading zeros (00..59)
      //    's' : Seconds, with leading zeros (00..59)
      // Example:
      //   $"<p>$(timestr("d/m/Y H:i:s", time()))</p>";
      if (strcmp_P(funcName, PSTR("timestr"))==0)
      {
        if (!checkParamsType(runtime, 0, Variant::STRING) || !checkParamsType(runtime, 1, Variant::UINT32))
          return false;
        DateTime dt( runtime.vars[1].value.toUInt32() );
        result->stringVal() = dt.format( runtime.vars[0].value.stringVal() );
        return true;
      }

      #endif // FDV_SCRIPT_SUPPORT_DATETIME

      //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
      //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
      // FILE SYSTEM

      #ifdef FDV_SCRIPT_SUPPORT_FILESYSTEM

      // mkdir(STRING fullpath)
      // Creates a new directory
      if (strcmp_P(funcName, PSTR("mkdir"))==0)
      {
        if (!checkParamsType(runtime, 0, Variant::STRING))
          return false;
        m_fileSystem->makeDirectory( runtime.vars[0].value.stringVal().c_str() );
        return true;
      }

      // rmdir(STRING fullpath)
      // Removes a directory
      if (strcmp_P(funcName, PSTR("rmdir"))==0)
      {
        if (!checkParamsType(runtime, 0, Variant::STRING))
          return false;
        m_fileSystem->removeDirectory( runtime.vars[0].value.stringVal().c_str() );
        return true;
      }

      // unlink(STRING fullpath)
      // Removes a file
      if (strcmp_P(funcName, PSTR("unlink"))==0)
      {
        if (!checkParamsType(runtime, 0, Variant::STRING))
          return false;
        m_fileSystem->removeFile( runtime.vars[0].value.stringVal().c_str() );
        return true;
      }

      // copy(STRING source, STRING dest)
      // Copies a file
      if (strcmp_P(funcName, PSTR("copy"))==0)
      {
        if (!checkParamsType(runtime, 0, Variant::STRING) || !checkParamsType(runtime, 1, Variant::STRING))
          return false;
        fileCopy(*m_fileSystem, runtime.vars[0].value.stringVal().c_str(), runtime.vars[1].value.stringVal().c_str());
        return true;
      }

      // rename(STRING source, STRING dest)
      // Renames a file
      if (strcmp_P(funcName, PSTR("rename"))==0)
      {
        if (!checkParamsType(runtime, 0, Variant::STRING) || !checkParamsType(runtime, 1, Variant::STRING))
          return false;
        fileMove(*m_fileSystem, runtime.vars[0].value.stringVal().c_str(), runtime.vars[1].value.stringVal().c_str());
        return true;
      }

      // UINT8 = file_exists(STRING fullpath)
      // Returns 1 if the file/directory exists, 0 otherwise
      if (strcmp_P(funcName, PSTR("file_exists"))==0)
      {
        if (!checkParamsType(runtime, 0, Variant::STRING))
          return false;
        result->uint8Val() = fileExists(*m_fileSystem, runtime.vars[0].value.stringVal().c_str());
        return true;
      }

      // STRING = tempname()
      // Returns a temporary file name. The file is NOT created.
      if (strcmp_P(funcName, PSTR("tempname"))==0)
      {
        result->stringVal() = getTempFilename(*m_fileSystem);
        return true;
      }

      #endif // FDV_SCRIPT_SUPPORT_FILESYSTEM


      //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
      //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
      // FILES

      #ifdef FDV_SCRIPT_SUPPORT_FILES

      // UINT16 = fopen(STRING fullpath, STRING mode)
      // Opens a file
      // mode:
      //   'r'  Open for reading only; place the file pointer at the beginning of the file.
      //   'r+' Open for reading and writing; place the file pointer at the beginning of the file.
      //   'w'  Open for writing only; place the file pointer at the beginning of the file and truncate the file to zero length. If the file does not exist, attempt to create it.
      //   'w+' Open for reading and writing; place the file pointer at the beginning of the file and truncate the file to zero length. If the file does not exist, attempt to create it.
      //   'a'  Open for writing only; place the file pointer at the end of the file. If the file does not exist, attempt to create it.
      //   'a+' Open for reading and writing; place the file pointer at the end of the file. If the file does not exist, attempt to create it.
      if (strcmp_P(funcName, PSTR("fopen"))==0)
      {
        if (!checkParamsType(runtime, 0, Variant::STRING) || !checkParamsType(runtime, 1, Variant::STRING))
          return false;
        result->uint16Val() = 0;
        uint8_t mode = 0xFF;
        char m0 = runtime.vars[1].value.stringVal()[0];
        char m1 = runtime.vars[1].value.stringVal().size() > 1? runtime.vars[1].value.stringVal()[1] : ' ';
        switch (m0)
        {
          case 'r':
            mode = File::MD_READ | (m1=='+'? File::MD_WRITE : 0);
            break;
          case 'w':
            mode = File::MD_CREATE | File::MD_TRUNC | File::MD_WRITE | (m1=='+'? File::MD_READ : 0);
            break;
          case 'a':
            mode = File::MD_APPEND | (m1=='+'? File::MD_WRITE : 0);
            break;
        }
        if (mode!=0xFF)
        {
          File* file = new File(*m_fileSystem, runtime.vars[0].value.stringVal().c_str(), mode);
          if (file->isOpen())
            result->uint16Val() = uint16_t(file);
          else
            delete file;
        }
        return true;
      }

      // fclose(UINT16 handle)
      // Closed a file
      if (strcmp_P(funcName, PSTR("fclose"))==0)
      {
        if (!checkParamsType(runtime, 0, Variant::UINT16))
          return false;
        File* file = (File*)runtime.vars[0].value.uint16Val();
        delete file;
        return true;
      }

      // UINT8 = feof(UINT16 handle)
      // Tests for EOF
      if (strcmp_P(funcName, PSTR("feof"))==0)
      {
        if (!checkParamsType(runtime, 0, Variant::UINT16))
          return false;
        File* file = (File*)runtime.vars[0].value.uint16Val();
        result->uint8Val() = file==NULL || file->isEOF();
        return true;
      }

      // UINT8 = fgetc(UINT16 handle)
      // Reads a character as ASCII number
      if (strcmp_P(funcName, PSTR("fgetc"))==0)
      {
        if (!checkParamsType(runtime, 0, Variant::UINT16))
          return false;
        File* file = (File*)runtime.vars[0].value.uint16Val();
        uint8_t ret = 0;
        file->read(&ret, 1);
        result->uint8Val() = ret;
        return true;
      }

      // STRING = fgets(UINT16 handle)
      // Reads a line (until EOL or EOF)
      if (strcmp_P(funcName, PSTR("fgets"))==0)
      {
        if (!checkParamsType(runtime, 0, Variant::UINT16))
          return false;
        File* file = (File*)runtime.vars[0].value.uint16Val();
        result->stringVal() = file->readLine();
        return true;
      }

      // UINT32 = ftell(UINT16 handle)
      // Returns the current position of the file read/write pointer
      if (strcmp_P(funcName, PSTR("ftell"))==0)
      {
        if (!checkParamsType(runtime, 0, Variant::UINT16))
          return false;
        File* file = (File*)runtime.vars[0].value.uint16Val();
        result->uint32Val() = file->position();
        return true;
      }

      // UINT32 = fseek(UINT16 handle, UINT32 position)
      // Sets current position (actually only SET is supported, so "whence" is not present)
      // Returns new position
      if (strcmp_P(funcName, PSTR("fseek"))==0)
      {
        if (!checkParamsType(runtime, 0, Variant::UINT16) || !checkParamsType(runtime, 1, Variant::UINT32))
          return false;
        File* file = (File*)runtime.vars[0].value.uint16Val();
        file->seek(runtime.vars[1].value.toUInt32(), File::SK_SET);
        result->uint32Val() = file->position();
        return true;
      }

      // UINT32 = fsize(UINT16 handle)
      // Returns file size
      if (strcmp_P(funcName, PSTR("fsize"))==0)
      {
        if (!checkParamsType(runtime, 0, Variant::UINT16))
          return false;
        File* file = (File*)runtime.vars[0].value.uint16Val();
        result->uint32Val() = file->size();
        return true;
      }

      // ftruncate(UINT16 handle, UINT32 newsize)
      // Truncates file to the specified size
      if (strcmp_P(funcName, PSTR("ftruncate"))==0)
      {
        if (!checkParamsType(runtime, 0, Variant::UINT16) || !checkParamsType(runtime, 1, Variant::UINT32))
          return false;
        File* file = (File*)runtime.vars[0].value.uint16Val();
        file->truncate(runtime.vars[1].value.toUInt32());
        return true;
      }

      // STRING = fread(UINT16 handle, UINT16 length)
      // Reads binary data
      if (strcmp_P(funcName, PSTR("fread"))==0)
      {
        if (!checkParamsType(runtime, 0, Variant::UINT16) || !checkParamsType(runtime, 1, Variant::UINT16))
          return false;
        File* file = (File*)runtime.vars[0].value.uint16Val();
        uint16_t len = runtime.vars[1].value.uint16Val();
        result->stringVal().resize(len);
        file->read(&result->stringVal()[0], len);
        return true;
      }

      // fwrite(UINT16 handle, STRING buffer [, UINT16 length])
      // Writes binary data
      if (strcmp_P(funcName, PSTR("fwrite"))==0)
      {
        if (!checkParamsType(runtime, 0, Variant::UINT16) || !checkParamsType(runtime, 1, Variant::STRING))
          return false;
        File* file = (File*)runtime.vars[0].value.uint16Val();
        string& buffer = runtime.vars[1].value.stringVal();
        uint16_t len = (runtime.vars.size() == 3 ? runtime.vars[2].value.toUInt32() : buffer.size());
        file->write(&buffer[0], len);
        return true;
      }

      #endif // FDV_SCRIPT_SUPPORT_FILES

      //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
      //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
      // CONFIGURATION FILES (INI)

      // STRING inireadstring(STRING filename, STRING key, STRING defaultValue)
      // STRING inireadstring(STRING filename, UINT32 position)
      // Reads value of key or position from specified ini file. To get position use findkey.
      if (strcmp_P(funcName, PSTR("inireadstring"))==0)
      {
        if (runtime.vars.size() < 2)
          return false;
        if (!checkParamsType(runtime, 0, Variant::STRING))
          return false;
        Ini ini(m_fileSystem->sdcard(), runtime.vars[0].value.stringVal().c_str());
        // first overload
        if (runtime.vars[1].value.type() == Variant::STRING)
        {
          if (!checkParamsType(runtime, 2, Variant::STRING))
            return false;
          result->stringVal() = ini.readString(runtime.vars[1].value.stringVal().c_str(), runtime.vars[2].value.stringVal().c_str());
        }
        // second overload
        else if (runtime.vars[1].value.type() == Variant::UINT32)
        {
          uint32_t pos = runtime.vars[1].value.uint32Val();
          result->stringVal() = ini.readString(&pos);
        }
        return true;
      }

      // UINT8/16/32 inireaduint(STRING filename, STRING key, UINT defaultValue)
      // Reads value of key from specified ini file.
      if (strcmp_P(funcName, PSTR("inireaduint"))==0)
      {
        if (!checkParamsType(runtime, 0, Variant::STRING) || !checkParamsType(runtime, 1, Variant::STRING))
          return false;
        Ini ini(m_fileSystem->sdcard(), runtime.vars[0].value.stringVal().c_str());
        result->shrink( ini.readUInt32(runtime.vars[1].value.stringVal().c_str(), runtime.vars[2].value.toUInt32()) );
        return true;
      }

      // FLOAT inireadfloat(STRING filename, STRING key, FLOAT defaultValue)
      // Reads float value from specified ini file.
      if (strcmp_P(funcName, PSTR("inireadfloat"))==0)
      {
        if (!checkParamsType(runtime, 0, Variant::STRING) || !checkParamsType(runtime, 1, Variant::STRING) || !checkParamsType(runtime, 2, Variant::FLOAT))
          return false;
        Ini ini(m_fileSystem->sdcard(), runtime.vars[0].value.stringVal().c_str());
        result->floatVal() = ini.readFloat(runtime.vars[1].value.stringVal().c_str(), runtime.vars[2].value.toFloat());
        return true;
      }

      // iniwritestring(STRING filename, STRING key, STRING value)
      // Writes value of key to specified ini file
      if (strcmp_P(funcName, PSTR("iniwritestring"))==0)
      {
        if (!checkParamsType(runtime, 0, Variant::STRING) || !checkParamsType(runtime, 1, Variant::STRING) || !checkParamsType(runtime, 2, Variant::STRING))
          return false;
        Ini ini(m_fileSystem->sdcard(), runtime.vars[0].value.stringVal().c_str());
        ini.writeString(runtime.vars[1].value.stringVal().c_str(), runtime.vars[2].value.stringVal().c_str());
        return true;
      }

      // iniwriteuint(STRING filename, STRING key, UINT value)
      // Writes value of key to specified ini file
      if (strcmp_P(funcName, PSTR("iniwriteuint"))==0)
      {
        if (!checkParamsType(runtime, 0, Variant::STRING) || !checkParamsType(runtime, 1, Variant::STRING) || !checkParamsType(runtime, 2, Variant::UINT8 | Variant::UINT16 | Variant::UINT32))
          return false;
        Ini ini(m_fileSystem->sdcard(), runtime.vars[0].value.stringVal().c_str());
        ini.writeUInt32(runtime.vars[1].value.stringVal().c_str(), runtime.vars[2].value.toUInt32());
        return true;
      }

      // iniwritefloat(STRING filename, STRING key, FLOAT value)
      // Writes value of key to specified ini file
      if (strcmp_P(funcName, PSTR("iniwritefloat"))==0)
      {
        if (!checkParamsType(runtime, 0, Variant::STRING) || !checkParamsType(runtime, 1, Variant::STRING) || !checkParamsType(runtime, 2, Variant::FLOAT))
          return false;
        Ini ini(m_fileSystem->sdcard(), runtime.vars[0].value.stringVal().c_str());
        ini.writeFloat(runtime.vars[1].value.stringVal().c_str(), runtime.vars[2].value.toFloat());
        return true;
      }

      // UINT32 inibypass(STRING filename, UINT position)
      // Bypasses key at specified position
      if (strcmp_P(funcName, PSTR("inibypass"))==0)
      {
        if (!checkParamsType(runtime, 0, Variant::STRING) || !checkParamsType(runtime, 1, Variant::UINT8 | Variant::UINT16 | Variant::UINT32))
          return false;
        Ini ini(m_fileSystem->sdcard(), runtime.vars[0].value.stringVal().c_str());
        uint32_t pos = runtime.vars[1].value.toUInt32();
        ini.readString(&pos);
        result->uint32Val() = pos;
        return true;
      }

      // iniremovekey(STRING filename, STRING key)
      // Removes all keys found
      if (strcmp_P(funcName, PSTR("iniremovekey"))==0)
      {
        if (!checkParamsType(runtime, 0, Variant::STRING) || !checkParamsType(runtime, 1, Variant::STRING))
          return false;
        Ini ini(m_fileSystem->sdcard(), runtime.vars[0].value.stringVal().c_str());
        ini.removeKey(runtime.vars[1].value.stringVal().c_str());
        return true;
      }

      // UINT32 inifindkey(STRING filename, UINT32 position, STRING key, UINT8 exactMatch)
      // Finds a key from position. Returns found position.
      // Returns 0xFFFFFFFF if not found
      //
      // Example:
      //   ini = "CONF.INI";
      //   pos = 0;
      //   while ( (pos=inifindkey(ini, pos, "script_", 0)) != 0xFFFFFFFF )
      //   {
      //     key = inireadkey(ini, pos);
      //     value = inireadstring(ini, pos);
      //     $"<p>KEY=$key VALUE=$value</p>";
      //     pos = inibypass(ini, pos);
      //   }
      if (strcmp_P(funcName, PSTR("inifindkey"))==0)
      {
        if (!checkParamsType(runtime, 0, Variant::STRING) || !checkParamsType(runtime, 1, Variant::UINT8 | Variant::UINT16 | Variant::UINT32) || !checkParamsType(runtime, 2, Variant::STRING) || !checkParamsType(runtime, 3, Variant::UINT8))
          return false;
        Ini ini(m_fileSystem->sdcard(), runtime.vars[0].value.stringVal().c_str());
        uint32_t pos = runtime.vars[1].value.toUInt32();
        bool r = ini.findKey(&pos, runtime.vars[2].value.stringVal().c_str(), runtime.vars[3].value.toUInt32());
        result->uint32Val() = r? pos : 0xFFFFFFFF;
        return true;
      }

      // STRING inireadkey(STRING filename, UINT position)
      // Reads key at position (use with inifindkey)
      if (strcmp_P(funcName, PSTR("inireadkey"))==0)
      {
        if (!checkParamsType(runtime, 0, Variant::STRING) || !checkParamsType(runtime, 1, Variant::UINT8 | Variant::UINT16 | Variant::UINT32))
          return false;
        Ini ini(m_fileSystem->sdcard(), runtime.vars[0].value.stringVal().c_str());
        result->stringVal() = ini.readKey( runtime.vars[1].value.toUInt32() );
        return true;
      }

      // reloadevents(STRING filename)
      // Reloads events from specified ini file
      if (strcmp_P(funcName, PSTR("reloadevents"))==0)
      {
        if (!checkParamsType(runtime, 0, Variant::STRING))
          return false;
        Ini ini(m_fileSystem->sdcard(), runtime.vars[0].value.stringVal().c_str());
        ScriptScheduler<OutputType, LibraryType>::refresh(ini);
        return true;
      }



      //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
      //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
      // RF link

      #ifdef FDV_SCRIPT_SUPPORT_RFLINK

      // UINT8 = rf_getlocalid()
      // Returns local device id
      if (strcmp_P(funcName, PSTR("rf_getlocalid"))==0)
      {
        result->uint8Val() = WirelessRPC::localDeviceID();
        return true;
      }

      // UINT8 = rf_gettemp(UINT8 deviceid, FLOAT* temperature)
      // Calls method METHOD_GET_TEMPERATURE.
      // Note: variation to METHOD_GET_TEMPERATURE, "temperature=numerator/denominator"
      // Returns 0 on failure, 1 on success.
      if (strcmp_P(funcName, PSTR("rf_gettemp"))==0)
      {
        if (!checkParamsType(runtime, 0, Variant::UINT8) || !checkParamsType(runtime, 1, Variant::REFERENCE))
          return false;
        uint8_t* results = NULL;
        uint8_t resultsSize;
        result->uint8Val() = WirelessRPC::call(WirelessRPC::localDeviceID(), runtime.vars[0].value.uint8Val(), WirelessRPC::METHOD_GET_TEMPERATURE, NULL, 0, &results, &resultsSize);
        if (result->uint8Val())
        {
          runtime.vars[1].value.refVal()->floatVal() = static_cast<float>(*((int16_t*)results+0)) / static_cast<float>(*((int16_t*)results+1));
          freeEx(results);
        }
        return true;
      }

      // UINT8 = rf_getco(UINT8 deviceid, UINT16* co_low, UINT16* co_high)
      // Calls method METHOD_GET_CARBON_MONOXIDE.
      // Returns 0 on failure, 1 on success
      if (strcmp_P(funcName, PSTR("rf_getco"))==0)
      {
        if (!checkParamsType(runtime, 0, Variant::UINT8) || !checkParamsType(runtime, 1, Variant::REFERENCE) || !checkParamsType(runtime, 2, Variant::REFERENCE))
          return false;
        uint8_t* results = NULL;
        uint8_t resultsSize;
        result->uint8Val() = WirelessRPC::call(WirelessRPC::localDeviceID(), runtime.vars[0].value.uint8Val(), WirelessRPC::METHOD_GET_CARBON_MONOXIDE, NULL, 0, &results, &resultsSize);
        if (result->uint8Val())
        {
          runtime.vars[1].value.refVal()->uint16Val() = *((uint16_t*)results+0);
          runtime.vars[2].value.refVal()->uint16Val() = *((uint16_t*)results+1);
          freeEx(results);
        }
        return true;
      }

      // UINT8 = rf_boilerset(UINT8 deviceid, UINT8 newstate)
      // Calls method METHOD_BOILER_ACTIVE.
      // Returns 0 on failure, 1 on success
      if (strcmp_P(funcName, PSTR("rf_boilerset"))==0)
      {
        if (!checkParamsType(runtime, 0, Variant::UINT8) || !checkParamsType(runtime, 1, Variant::UINT8))
          return false;
        uint8_t* results = NULL;
        uint8_t resultsSize;
        bool v = runtime.vars[1].value.uint8Val();
        result->uint8Val() = WirelessRPC::call(WirelessRPC::localDeviceID(), runtime.vars[0].value.uint8Val(), WirelessRPC::METHOD_BOILER_ACTIVE, (uint8_t*)&v, sizeof(bool), &results, &resultsSize);
        return true;
      }

      // UINT8 = rf_boilerget(UINT8 deviceid, UINT8* currentstate)
      // Calls method METHOD_BOILER_QUERY.
      // Returns 0 on failure, 1 on success
      if (strcmp_P(funcName, PSTR("rf_boilerget"))==0)
      {
        if (!checkParamsType(runtime, 0, Variant::UINT8) || !checkParamsType(runtime, 1, Variant::REFERENCE))
          return false;
        uint8_t* results = NULL;
        uint8_t resultsSize;
        result->uint8Val() = WirelessRPC::call(WirelessRPC::localDeviceID(), runtime.vars[0].value.uint8Val(), WirelessRPC::METHOD_BOILER_QUERY, NULL, 0, &results, &resultsSize);
        if (result->uint8Val())
        {
          runtime.vars[1].value.refVal()->uint8Val() = *((bool*)results);
          freeEx(results);
        }
        return true;
      }

      // UINT8 = rf_getuptime(UINT8 deviceid, UINT32* uptime)
      // Calls method METHOD_SYSTEM_UPTIME.
      // Returns 0 on failure, 1 on success
      if (strcmp_P(funcName, PSTR("rf_getuptime"))==0)
      {
        if (!checkParamsType(runtime, 0, Variant::UINT8) || !checkParamsType(runtime, 1, Variant::REFERENCE))
          return false;
        uint8_t* results = NULL;
        uint8_t resultsSize;
        result->uint8Val() = WirelessRPC::call(WirelessRPC::localDeviceID(), runtime.vars[0].value.uint8Val(), WirelessRPC::METHOD_SYSTEM_UPTIME, NULL, 0, &results, &resultsSize);
        if (result->uint8Val())
        {
          runtime.vars[1].value.refVal()->uint32Val() = *((uint32_t*)results);
          freeEx(results);
        }
        return true;
      }

      // UINT8 = rf_buzzeralarm(UINT8 deviceid, UINT8 beepCount, UINT8 frequency, UINT8 duration)
      // Calls method METHOD_BUZZER_ALARM.
      // frequency is in hz*10 (1=10hz, 10=100hz, 255=2550hz)
      // duration is in ms*10 (1=10ms, 10=100ms, 255=2550ms)
      // Returns 0 on failure, 1 on success
      if (strcmp_P(funcName, PSTR("rf_buzzeralarm"))==0)
      {
        if (!checkParamsType(runtime, 0, Variant::UINT8) || !checkParamsType(runtime, 1, Variant::UINT8) || !checkParamsType(runtime, 2, Variant::UINT8) || !checkParamsType(runtime, 3, Variant::UINT8))
          return false;
        uint8_t* results = NULL;
        uint8_t resultsSize;
        uint8_t params[3];
        params[0] = runtime.vars[1].value.uint8Val();
        params[1] = runtime.vars[2].value.uint8Val();
        params[2] = runtime.vars[3].value.uint8Val();
        result->uint8Val() = WirelessRPC::call(WirelessRPC::localDeviceID(), runtime.vars[0].value.uint8Val(), WirelessRPC::METHOD_BUZZER_ALARM, params, 3, &results, &resultsSize);
        return true;
      }

      #endif // FDV_SCRIPT_SUPPORT_RFLINK


      return false;
    }


  private:

    FileSystem* m_fileSystem;
  };


};


#endif /* FDV_SCRIPTLIBRARY_H_ */
