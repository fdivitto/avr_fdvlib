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



#ifndef FDV_SCRIPT_H_
#define FDV_SCRIPT_H_


/*

DIGIT = '0' | '1' | '2' | '3' | '4' | '5' | '6' | '7' | '8' | '9'


ALPHA = 'a...z' | 'A...Z' | '_'


HEXDIGIT =  'a...f' | 'A...F' | '0' | '1' | '2' | '3' | '4' | '5' | '6' | '7' | '8' | '9'


BINDIGIT = '0' | '1'


constant = "0x" HEXDIGIT {HEXDIGIT}
| "0b" BINDIGIT {BINDIGIT}
| DIGIT {DIGIT} [ '.' DIGIT {DIGIT} ]


escape = '\' ascii_char


evar = '$' variable


eexp = "$(" expression ")"


string_literal = '"' {( escape | ascii_char | eexp | evar )} '"'


unparsed_string_literal = ''' {ascii_char} '''


identifier = ALPHA {(ALPHA | DIGIT)}


variable = identifier [ '[' expression ']' ]


primary_expression = variable
| constant
| string_literal
| unparsed_string_literal
| '(' expression ')'
| '{' [expression] {',' expression} '}'


postfix_expression = variable ("--" | "++")
| identifier '(' [assignment_expression] {',' assignment_expression} ')'
| primary_expression


unary_expression = postfix_expression
| ("--" | "++") variable
| ('+' | '-' | '!', '~') unary_expression
| "sizeof" '(' variable ')'
| '&' variable       (note: get pointer of variable)
| '*' variable       (note: get value of pointer)


multiplicative_expression = unary_expression {('*' | '/' | '%') unary_expression}


additive_expression = multiplicative_expression {('+' | '-') multiplicative_expression}


shift_expression = additive_expression {("<<" | ">>") additive_expression}


relational_expression = shift_expression {('<' | '>' | '<=' | '>=') shift_expression}


equality_expression = relational_expression {('==' | '!=') relational_expression}


and_expression = equality_expression {'&' equality_expression}


exclusive_or_expression = and_expression {'^' and_expression}


inclusive_or_expression = exclusive_or_expression {'|' exclusive_or_expression}


logical_and_expression = inclusive_or_expression {"&&" inclusive_or_expression}


logical_or_expression = logical_and_expression {"||" logical_and_expression}


conditional_expression = logical_or_expression ['?' expression ':' expression]


identifier_assign = variable '='


assignment_expression = {identifier_assign} conditional_expression


expression = assignment_expression


expression_statement = [expression] ';'


compound_statement = '{' {statement} '}'


selection_statement = "if" '(' expression ')' statement ["else" statement]


iteration_statement = "while" '(' expression ')' statement
| "do" statement "while" '(' expression ')' ';'
| "for" '(' expression_statement expression ';' [expression] ')' statement


jump_statement = "continue" ';'
| "break" ';'


output_statement = ':' expression {':' expression} ';'


statement = "<?"
| "?>"
| "//" all_chars EOL
| output_statement
| compound_statement
| expression_statement
| selection_statement
| iteration_statement
| jump_statement


script = {statement}



*******************************************************************************

IMPLEMENTATION SAMPLE:

#include "fdv_script.h"

using namespace fdv;

struct Output
{
void write(char c)
{
cout << c;
}
void write(char const* str)
{
cout << str;
}
};

int main()
{
TextInput textInput(
"<?"
"x=10;"
"write(x);"
"?>"
);
Output output;
RunTime<Output> globalRunTime(&output);
if (parseScript(&globalRunTime, &textInput))
cout << "ok!" << endl;
}

*******************************************************************************


SCRIPT SAMPLES:
************************
Sample #0:

<?
for (i=0; i<10; ++i)
{
?>

i = <? $i; ?>\n

<?
}
?>

*************************
Sample #1:

<?
for (i=0; i<10; ++i)
{
: "i = " : str(i);
}
?>
*************************
Sample #2:

<?
for (i=0; i<10; ++i)
{
:"i = $i";
}
?>
*************************
Sample #3:

<?
for (i=0; i<10; ++i)
write("i = $i\n");
?>
*************************
Sample #4, expresions inside "$(...)":

<?
for (i=0; i<10; ++i)
write("i = $(i/2)\n");
?>
*************************
Sample #5, comments:

<?
// comment
x=1; // another comment
?>
************************
Sample #6, arrays:
<?
a = {};              // create empty array
arrayadd(&a, "one"); // add "one" (necessary for functions which require a reference to array)
b[10] = "hello";     // direct create and assign array (creation not required)
:"a=$a  b=$b";       // print arrays
:"b[10]=$b[10]";     // print single array item
?>
************************
Sample #7, pointers:
<?
a = 10;
b = &b;  // "b" is a reference (pointer) to "a"
c = *b + 1;  // *b=a=10 then c=11
?>
************************
Sample #8, inizializing array:
<?
ar0 = {};  // empty array
ar1 = {1, 2, 3};
ar2 = {"one", 2, "tre"};
ar3 = {one, {sub1, sub2}};
************************
Sample #9, write and read array in INI file:
<?
ar1 = {1, 2, 3, 4};
iniwritestring("CONF.INI", "myarray", str(arr));
...
ar1 = eval(inireadstring("CONF.INI", "myarray", "{}"));
?>


*/



// TODO: due using of size_t on AVR script and html page is limited to 64K
// TODO: comments cannot exist inside IF..ELSE

#include <stdlib.h>
#include <ctype.h>

#include "../fdv_sdlib/fdv_sdcard.h"
#include "fdv_variant.h"



namespace fdv
{

  ///////////////////////////////////////////////////////////////////////////////////////////////
  // InputBase
  // Abstract input class

  struct InputBase
  {
    virtual char get() = 0;
    virtual void next() = 0;
    virtual size_t pos() = 0;
    virtual void pos(size_t value) = 0;
    virtual bool isEOF() = 0;
    virtual string const extract(size_t from) = 0;
    virtual void extract(size_t from, char* result) = 0;
  };


  ///////////////////////////////////////////////////////////////////////////////////////////////
  // TextInput
  // Input from text string

  class TextInput : public InputBase
  {

  public:

    TextInput(char const* text) :
        m_text(text), m_pos(0)
        {
        }

        char get()
        {
          //debug << "get() m_pos=" << m_pos << " c=" << m_text[m_pos] << " " << uint16_t(m_text[m_pos]) << ENDL;
          return m_text[m_pos];
        }

        void next()
        {
          ++m_pos;
        }

        size_t pos()
        {
          return m_pos;
        }

        void pos(size_t value)
        {
          m_pos = value;
        }

        bool isEOF()
        {
          return m_text[m_pos] == 0;
        }

        string const extract(size_t from)
        {
          string ret(m_pos-from, ' ');
          extract(from, ret.c_str());
          return ret;
        }

        void extract(size_t from, char* result)
        {
          memcpy(result, &m_text[from], m_pos-from);
        }

  private:

    char const* m_text;
    size_t      m_pos;
    size_t      m_len;
  };


  ///////////////////////////////////////////////////////////////////////////////////////////////
  // FileInput
  // Input from File

  struct BufferedFileInput : public InputBase
  {
    BufferedFileInput(File* file) :
  m_file(file), m_bufPos(0), m_fileSize(m_file->size()), m_filePos(0xFFFFFFFF)
  {
    pos(0);
  }

  char get()
  {
    return m_buffer[m_bufPos];
  }

  void next()
  {
    ++m_bufPos;
    if (m_bufPos==BUFLEN)
      pos(m_filePos + m_bufPos);
  }

  size_t pos()
  {
    return m_filePos + m_bufPos;
  }

  void pos(size_t value)
  {
    if (value < m_filePos || value >= (m_filePos+BUFLEN))
    {
      m_filePos = value;
      m_bufPos = 0;
      m_file->position(m_filePos);
      m_file->read(&m_buffer[0], BUFLEN);
    }
    else
      m_bufPos = value - m_filePos;
  }

  bool isEOF()
  {
    return m_filePos + m_bufPos >= m_fileSize;
  }

  string const extract(size_t from)
  {
    string ret(pos()-from, ' ');
    extract(from, ret.c_str());
    return ret;
  }

  void extract(size_t from, char* result)
  {
    if (from > m_filePos)
    {
      memcpy(result, &m_buffer[from-m_filePos], pos()-from);
    }
    else
    {
      m_file->position(from);
      m_file->read(result, pos()-from);
    }
    // no need to restore position
  }

  private:

    static uint8_t const BUFLEN = 16;

    File*    m_file;
    char     m_buffer[BUFLEN];
    uint8_t  m_bufPos;
    uint32_t m_fileSize;
    uint32_t m_filePos;
  };


  struct UnBufferedFileInput : public InputBase
  {
    UnBufferedFileInput(File* file)
      : m_file(file)
    {
    }

    char get()
    {
      char b;
      m_file->read(&b, 1);
      m_file->seek(-1, File::SK_CUR);
      return b;
    }

    void next()
    {
      m_file->seek(1, File::SK_CUR);
    }

    size_t pos()
    {
      return m_file->position();
    }

    void pos(size_t value)
    {
      m_file->position(value);
    }

    bool isEOF()
    {
      return m_file->isEOF();
    }

    string const extract(size_t from)
    {
      string ret(pos()-from, ' ');
      extract(from, &ret[0]);
      return ret;
    }

    void extract(size_t from, char* result)
    {
      size_t cpos = pos();
      pos(from);
      while (pos() != cpos)
      {
        *result++ = get();
        next();
      }
    }

  private:
    File* m_file;
  };


  ///////////////////////////////////////////////////////////////////////////////////////////////
  // Variable

  struct Variable
  {
    Variable(char const* name_, Variant const& value_) :
  name(name_), value(value_)
  {
  }

  string  name;
  Variant value;
  };


  ///////////////////////////////////////////////////////////////////////////////////////////////
  // RunTime

  template <typename OutputT, typename LibraryT>
  class RunTime
  {
  public:

    static size_t const NOTFOUND = 0xFFFF;

    typedef LibraryT LibraryType;
    typedef OutputT OutputType;

    RunTime(OutputT* output_, LibraryT* library_, RunTime* parent_)
      : output(output_), library(library_), parent(parent_)
    {
    }

    void addVariable(Variable const& var)
    {
      //debug << "addVariable name=" << var.name << "  value=" << var.value.toString() << ENDL;
      vars.push_back(var);
    }

    void setVariableArrayValue(size_t vindex, size_t arrayIndex, Variant const& value)
    {
      vector<Variant>& arr = vars[vindex].value.arrayVal();
      if (arrayIndex >= arr.size())
        arr.resize(arrayIndex+1);
      arr[arrayIndex] = value;
    }

    void setVariable(char const* name, size_t arrayIndex, Variant const& value)
    {
      size_t i = findVariable(name);
      if (i==NOTFOUND)
      {
        // new variable
        if (arrayIndex==0xFFFF)
        {
          // simple variable
          addVariable(Variable(name, value));
        }
        else
        {
          // array
          addVariable(Variable(name, Variant(vector<Variant>())));  // add empty array
          setVariableArrayValue(vars.size()-1, arrayIndex, value);
        }
      }
      else
      {
        // update variable
        if (arrayIndex==0xFFFF)
        {
          // simple variable
          vars[i] = Variable(name, value);
        }
        else
        {
          // array
          setVariableArrayValue(i, arrayIndex, value);
        }
      }
    }

    // note: returns NULL if not found
    // note: arrayIndex=0xFFFF if this is "not" an array index
    Variant* getVariableValue(char const* name, size_t arrayIndex = 0xFFFF)
    {
      size_t i = findVariable(name);
      // not found
      if (i==NOTFOUND)
        return NULL;
      // simple variable (or array without index)
      if (arrayIndex==0xFFFF)
        return &vars[i].value;
      // array item
      return &(vars[i].value.arrayVal()[arrayIndex]);
    }

    // note: returns NOTFOUND if not found
    size_t findVariable(char const* name)
    {
      for (size_t i=0; i!=vars.size(); ++i)
        if (vars[i].name == name)
          return i;
      return NOTFOUND;
    }

    bool execFunc(char const* funcName, Variant* result)
    {
      return library->execFunc(funcName, *this, result);
    }

    vector<Variable> vars;
    OutputT*         output;
    LibraryT*        library;
    RunTime*         parent;
  };



  ///////////////////////////////////////////////////////////////////////////////////////////////
  // Script

  template <typename RunTimeT>
  class Script
  {

  public:

    Script(RunTimeT* globalRunTime, InputBase* input) :
        m_runTime(globalRunTime),
          m_input(input),
          m_progStatus(ST_RUN),
          m_incode(false)
        {
        }


  private:


    class PosSaver
    {
    public:
      PosSaver(InputBase* input) :
          m_input(input), m_pos(input->pos())
          {
          }

          ~PosSaver()
          {
            restore();
          }

          bool release()
          {
            m_input = NULL;
            return true;
          }

          void restore()
          {
            if (m_input)
              m_input->pos( m_pos );
          }

          size_t savedPos()
          {
            return m_pos;
          }

          void reset()
          {
            m_pos = m_input->pos();
          }

    private:
      InputBase* m_input;
      size_t     m_pos;
    };


    void bypassSpaces()
    {
      while ( !m_input->isEOF() && isspace(m_input->get()) )
        m_input->next();
    }


    // Parsers all single chars ('+', '-', ';', etc...), bypassing leading spaces
    bool parse_1char(char c, char* result)
    {
      if (parse_1char(c))
      {
        *result = c;
        return true;
      }
      return false;
    }


    // Parsers all single chars ('+', '-', ';', etc...), bypassing leading spaces
    bool parse_1char(char c)
    {
      bypassSpaces();
      if (m_input->get() == c)
      {
        m_input->next();
        return true;
      }
      return false;
    }


    // like parse_1char, but checks other sign after
    bool parse_1charCheckNext(char c, char ifNextIsNot)
    {
      bypassSpaces();
      if (m_input->get() == c)
      {
        size_t p1 = m_input->pos();
        m_input->next();
        if (m_input->get() == ifNextIsNot)
        {
          m_input->pos(p1);  // restore pos
          return false;
        }
        return true;
      }
      return false;
    }


    bool parse_1charCheckNext(char c, char ifNextIsNot, char* result)
    {
      if (parse_1charCheckNext(c, ifNextIsNot))
      {
        *result = c;
        return true;
      }
      return false;
    }


    // Parsers N chars ("if", "while", "&&", etc...), bypass leading spaces
    bool parse_nchar(char const* s, char* result)
    {
      bypassSpaces();
      size_t pos = m_input->pos();
      bool r = parse_nchar(s);
      if (r)
        m_input->extract(pos, result);
      return r;
    }


    // Parsers N chars ("if", "while", "&&", etc...), bypass leading spaces
    bool parse_nchar(char const* s)
    {
      PosSaver psaver(m_input);
      bypassSpaces();
      while (*s != 0)
      {
        if (*s++ != m_input->get())
          return false;
        m_input->next();
      }
      return psaver.release();
    }


    bool isAlpha(char c)
    {
      return (c>='a' && c<='z') || (c>='A' && c<='Z') || (c=='_');
    }


    bool isDigit(char c)
    {
      return c>='0' && c<='9';
    }


    bool isHexDigit(char c)
    {
      return (c>='0' && c<='9') || (c>='a' && c<='f') || (c>='A' && c<='F');
    }


    bool isBinDigit(char c)
    {
      return c=='0' || c=='1';
    }


    // ALPHA = 'a...z' | 'A...Z' | '_'
    bool parse_ALPHA()
    {
      if (isAlpha(m_input->get()))
      {
        m_input->next();
        return true;
      }
      return false;
    }


    // DIGIT = '0' | '1' | '2' | '3' | '4' | '5' | '6' | '7' | '8' | '9'
    bool parse_DIGIT()
    {
      if (isDigit(m_input->get()))
      {
        m_input->next();
        return true;
      }
      return false;
    }


    // HEXDIGIT =  'a...f' | 'A...F' | '0' | '1' | '2' | '3' | '4' | '5' | '6' | '7' | '8' | '9'
    bool parse_HEXDIGIT()
    {
      if (isHexDigit(m_input->get()))
      {
        m_input->next();
        return true;
      }
      return false;
    }


    // BINDIGIT = '0' | '1'
    bool parse_BINDIGIT()
    {
      if (isBinDigit(m_input->get()))
      {
        m_input->next();
        return true;
      }
      return false;
    }


    // identifier = ALPHA {ALPHA}
    bool parse_identifier(string& result)
    {
      bypassSpaces();
      PosSaver psaver(m_input);
      // ALPHA
      if (parse_ALPHA())
      {
        // {ALPHA}
        while (parse_ALPHA() || parse_DIGIT());
        result = m_input->extract(psaver.savedPos());
        return psaver.release();
      }
      return false;
    }


    // constant = "0x" HEXDIGIT {HEXDIGIT}
    //          | "0b" BINDIGIT {BINDIGIT}
    //          | DIGIT {DIGIT} [ '.' DIGIT {DIGIT} ]
    bool parse_constant(Variant* result)
    {
      bypassSpaces();
      PosSaver psaver(m_input);

      // "0x" HEXDIGIT {HEXDIGIT}
      if (parse_nchar("0x"))
      {
        psaver.reset(); // bypass 0x (not actually needed for 0x)
        while (parse_HEXDIGIT());
        result->shrink( strtoul(m_input->extract(psaver.savedPos()).c_str(), NULL, 16) );
        return psaver.release();
      }

      // "0b" BINDIGIT {BINDIGIT}
      if (parse_nchar("0b"))
      {
        psaver.reset(); // bypass 0b
        while (parse_BINDIGIT());
        result->shrink( strtoul(m_input->extract(psaver.savedPos()).c_str(), NULL, 2) );
        return psaver.release();
      }

      // DIGIT {DIGIT} [ '.' DIGIT {DIGIT} ]
      if (parse_DIGIT())
      {
        // {DIGIT}
        while (parse_DIGIT());

        // [ '.' DIGIT {DIGIT} ]
        if (m_input->get() == '.')
        {
          m_input->next();
          if (parse_DIGIT())
          {
            while (parse_DIGIT());
            result->floatVal() = atof( m_input->extract(psaver.savedPos()).c_str() ); // FLOAT
            return psaver.release();
          }
          else
            return false;
        }
        result->shrink( strtoul(m_input->extract(psaver.savedPos()).c_str(), NULL, 10) );
        return psaver.release();
      }
      return false;
    }


    // escape = '\' ascii_char
    bool parse_escape(char* c)
    {
      PosSaver psaver(m_input);

      // '\' ascii_char
      if (m_input->get() == '\\')
      {
        m_input->next();
        if (m_input->isEOF())
          return false;
        char b = m_input->get();
        m_input->next();
        switch (b)
        {
        case 'n':
          *c = 0x0A;
          break;
        case 'r':
          *c = 0x0D;
          break;
        case '0':
          *c = 0x00;
          break;
        case 'x': // note: only \xHH is supported (not \xHHHH)
          char hex[3];
          hex[0] = m_input->get(); m_input->next(); if (m_input->isEOF()) return false;
          hex[1] = m_input->get(); m_input->next();
          hex[2] = 0;
          *c = strtoul(hex, NULL, 16);
          break;
        default:
          *c = b;
        }
        return psaver.release();
      }

      return false;
    }


    // evar = '$' variable
    bool parse_evar(string& result, bool exec)
    {
      PosSaver psaver(m_input);

      if (m_input->get() == '$')
      {
        m_input->next();
        string vname;
        size_t aindex;
        if (parse_variable(vname, &aindex, exec))
        {
          if (exec)
          {
            Variant* v = m_runTime->getVariableValue(vname.c_str(), aindex);
            if (v!=NULL)
              result = v->toString();
            else
              return false;
          }
          return psaver.release();
        }
      }

      return false;
    }


    // eexp = "$(" expression ')'
    bool parse_eexp(string& result, bool exec)
    {
      PosSaver psaver(m_input);

      if (m_input->get() == '$')
      {
        m_input->next();
        if (m_input->get() == '(')
        {
          m_input->next();
          Variant r;
          if (parse_expression(&r, exec) && parse_1char(')'))
          {
            if (exec)
              result = r.toString();
            return psaver.release();
          }
          else
            return false;
        }
      }

      return false;
    }


    // string_literal = '"' {( escape | ascii_char | eexp | evar )} '"'
    bool parse_string_literal(string* result, bool exec)
    {
      PosSaver psaver(m_input);

      bypassSpaces();
      if (m_input->get() == '\"')  // check for Double Quote
      {
        m_input->next();
        result->clear();
        for (;;)
        {
          if (m_input->isEOF())
            return false;
          char c = m_input->get();
          string sr;
          if (parse_escape(&c))
            result->push_back(c);
          else if (parse_eexp(sr, exec))
            result->append(sr);
          else if (parse_evar(sr, exec))
            result->append(sr);
          else if(c == '\"')  // check for ending Double Quote
          {
            m_input->next();
            break;
          }
          else
          {
            result->push_back( m_input->get() );
            m_input->next();
          }
        }
        return psaver.release();
      }

      return false;
    }


    // unparsed_string_literal = ''' {ascii_char} '''
    bool parse_unparsed_string_literal(string* result)
    {
      PosSaver psaver(m_input);

      bypassSpaces();
      if (m_input->get() == '\'') // check for Single Quote
      {
        m_input->next();
        for (;;)
        {
          if (m_input->isEOF())
            return false;
          char c = m_input->get();
          m_input->next();
          if (c == '\'')  // check for ending Single Quote
            break;
          else
            result->push_back(c);
        }
        return psaver.release();
      }

      return false;
    }


    // variable = identifier [ '[' expression ']' ]
    // note: *arrayIndex=0xFFFF if it is "not" an array index
    bool parse_variable(string& result, size_t* arrayIndex, bool exec)
    {
      PosSaver psaver(m_input);

      if (parse_identifier(result))
      {
        psaver.reset(); // to avoid spaces loss in case parse_1char() fails
        if (parse_1char('['))
        {
          Variant f;
          if (!parse_expression(&f, exec) || !parse_1char(']'))
            return false;
          *arrayIndex = f.toUInt32();
        }
        else
        {
          *arrayIndex = 0xFFFF;
          psaver.restore(); // to avoid spaces loss in this case
        }
        return psaver.release();
      }

      return false;
    }


    // primary_expression = variable
    //                    | constant
    //                    | string_literal
    //                    | parse_unparsed_string_literal
    //                    | '(' expression ')'
    //                    | '{' [expression] {',' expression} '}'
    bool parse_primary_expression(Variant* result, bool exec)
    {
      PosSaver psaver(m_input);

      // variable
      string vname;
      size_t aindex;
      if (parse_variable(vname, &aindex, exec))
      {
        if (exec)
        {
          Variant* v = m_runTime->getVariableValue(vname.c_str(), aindex);
          if (v!=NULL)
            *result = *v;
          else
            return false;
        }
        return psaver.release();
      }

      // constant
      if (parse_constant(result))
        return psaver.release();

      // string_literal
      string sresult;
      if (parse_string_literal(&sresult, exec))
      {
        if (exec)
          *result = Variant(sresult);
        return psaver.release();
      }

      // unparsed_string_literal
      if (parse_unparsed_string_literal(&sresult))
      {
        if (exec)
          *result = Variant(sresult);
        return psaver.release();
      }

      // '(' expression ')'
      if (parse_1char('('))
      {
        parse_expression(result, exec);
        if (!parse_1char(')'))
          return false;
        return psaver.release();
      }

      // '{' [expression] {',' expression} '}'
      // note: array creation and initialization
      if (parse_1char('{'))
      {
        result->clear();
        result->type(Variant::ARRAY); // start with empty array
        Variant item;
        if (parse_expression(&item, exec))
        {
          if (exec)
            result->arrayVal().push_back(item);
          while (parse_1char(',') && parse_expression(&item, exec))
            if (exec)
              result->arrayVal().push_back(item);
        }
        if (!parse_1char('}'))
          return false;
        return psaver.release();
      }


      return false;
    }


    // postfix_expression = variable ("--" | "++")
    //                    | identifier '(' [assignment_expression] {',' assignment_expression} ')'
    //                    | primary_expression
    bool parse_postfix_expression(Variant* result, bool exec)
    {
      PosSaver psaver(m_input);

      // variable ("--" | "++")
      string sresult;
      size_t aindex;
      char nc[2];
      if (parse_variable(sresult, &aindex, exec)
        && (parse_nchar("--", nc) || parse_nchar("++", nc)))
      {
        if (exec)
        {
          Variant* v = m_runTime->getVariableValue(sresult.c_str(), aindex);
          if (v != NULL)
          {
            *result = *v;
            if (strncmp("--", nc, 2)==0)
              --(*v);
            else if (strncmp("++", nc, 2)==0)
              ++(*v);
          }
          else
            return false;
        }
        return psaver.release();
      }
      else
        psaver.restore();

      // identifier '(' [assignment_expression] {',' assignment_expression} ')'
      // note: perform function call
      if (parse_identifier(sresult) && parse_1char('('))
      {
        RunTimeT localRunTime(m_runTime->output, m_runTime->library, m_runTime);
        Variant r;
        if (parse_assignment_expression(&r, exec))
          if (exec)
            localRunTime.addVariable( Variable("", r) );
        while (parse_1char(',') && parse_assignment_expression(&r, exec))
          if (exec)
            localRunTime.addVariable( Variable("", r) );
        if (!parse_1char(')'))
          return false;
        if (exec)
        {
          // executes function
          if (!localRunTime.execFunc(sresult.c_str(), result))
            return false;
        }
        return psaver.release();
      }
      else
        psaver.restore();

      // primary_expression
      if (parse_primary_expression(result, exec))
        return psaver.release();

      return false;
    }


    // unary_expression = postfix_expression
    //                  | ("--" | "++") variable
    //                  | ('+' | '-' | '!' | '~') unary_expression
    //                  | "sizeof" '(' variable ')'
    //                  | '&' variable       (note: get pointer of variable)
    //                  | '*' variable       (note: get value of pointer)
    bool parse_unary_expression(Variant* result, bool exec)
    {
      PosSaver psaver(m_input);

      // postfix_expression
      if (parse_postfix_expression(result, exec))
        return psaver.release();

      // ("--" | "++") variable
      char nc[2];
      string vname;
      size_t aindex;
      if ((parse_nchar("--", nc) || parse_nchar("++", nc))
        && parse_variable(vname, &aindex, exec))
      {
        if (exec)
        {
          Variant* v = m_runTime->getVariableValue(vname.c_str(), aindex);
          if (v != NULL)
          {
            if (strncmp("--", nc, 2)==0)
              --(*v);
            else if (strncmp("++", nc, 2)==0)
              ++(*v);
            *result = *v;
          }
          else
            return false;
        }
        return psaver.release();
      }
      else
        psaver.restore();

      // ('+' | '-' | '!' | '~') unary_expression
      char op;
      if (parse_1char('+', &op)
        || parse_1char('-', &op)
        || parse_1char('!', &op)
        || parse_1char('~', &op))
      {
        if (parse_unary_expression(result, exec))
        {
          if (exec)
          {
            switch (op)
            {
            case '+':
              // nothing to do
              break;
            case '-':
              *result = -(*result);
              break;
            case '!':
              *result = !(*result);
              break;
            case '~':
              *result = ~(*result);
              break;
            }
          }
          return psaver.release();
        }
        else
          return false;
      }

      // "sizeof" '(' variable ')'
      if (parse_nchar("sizeof"))
      {
        if (parse_1char('(') && parse_variable(vname, &aindex, exec) && parse_1char(')'))
        {
          if (exec)
          {
            Variant* v = m_runTime->getVariableValue(vname.c_str(), aindex);
            if (v != NULL)
              result->shrink( v->size() );
            else
              return false;
          }
          return psaver.release();
        }
        else
          return false;
      }

      // '&' variable       (note: get pointer of variable)
      if (parse_1char('&'))
      {
        if (parse_variable(vname, &aindex, exec))
        {
          if (exec)
          {
            Variant* v = m_runTime->getVariableValue(vname.c_str(), aindex);
            if (v != NULL)
              result->refVal() = v;
            else
              return false;
          }
          return psaver.release();
        }
        else
          return false;
      }


      // '*' variable       (note: get r-value of pointer)
      if (parse_1char('*'))
      {
        if (parse_variable(vname, &aindex, exec))
        {
          if (exec)
          {
            Variant* v = m_runTime->getVariableValue(vname.c_str(), aindex);
            if (v != NULL)
              *result = *(v->refVal());
            else
              return false;
          }
          return psaver.release();
        }
        else
          return false;
      }


      return false;
    }


    // multiplicative_expression = unary_expression {('*' | '/' | '%') unary_expression}
    bool parse_multiplicative_expression(Variant* result, bool exec)
    {
      PosSaver psaver(m_input);

      // unary_expression
      if (parse_unary_expression(result, exec))
      {
        // {('*' | '/' | '%') unary_expression}
        char op;
        while (parse_1char('*', &op)
          || parse_1char('/', &op)
          || parse_1char('%', &op))
        {
          Variant f;
          if (parse_unary_expression(&f, exec))
          {
            if (exec)
            {
              switch (op)
              {
              case '*':
                *result = *result * f;
                break;
              case '/':
                *result = *result / f;
                break;
              case '%':
                *result = *result % f;
                break;
              }
            }
          }
          else
            return false;
        }
        return psaver.release();
      }

      return false;
    }


    // additive_expression = multiplicative_expression {('+' | '-') multiplicative_expression}
    bool parse_additive_expression(Variant* result, bool exec)
    {
      PosSaver psaver(m_input);

      // multiplicative_expression
      if (parse_multiplicative_expression(result, exec))
      {
        // {('+' | '-') multiplicative_expression}
        char op;
        while (parse_1char('+', &op) || parse_1char('-', &op))
        {
          Variant t;
          if (parse_multiplicative_expression(&t, exec))
          {
            if (exec)
            {
              *result = (op=='+'? *result + t : *result - t);
            }
          }
          else
            return false;
        }
        return psaver.release();
      }

      return false;
    }


    // shift_expression = additive_expression {("<<" | ">>") additive_expression}
    bool parse_shift_expression(Variant* result, bool exec)
    {
      PosSaver psaver(m_input);

      if (parse_additive_expression(result, exec))
      {
        char op[2];
        while (parse_nchar("<<", op) || parse_nchar(">>", op))
        {
          Variant f;
          if (parse_additive_expression(&f, exec))
          {
            if (exec)
            {
              if (strncmp("<<", op, 2)==0)
                *result = (*result << f);
              else if (strncmp(">>", op, 2)==0)
                *result = (*result >> f);
            }
          }
          else
            return false;
        }
        return psaver.release();
      }

      return false;
    }


    // relational_expression = shift_expression {('<' | '>' | '<=' | '>=') shift_expression}
    bool parse_relational_expression(Variant* result, bool exec)
    {
      PosSaver psaver(m_input);

      if (parse_shift_expression(result, exec))
      {
        char op[2];
        op[1] = 0;  // final zero in case of '<' and '>'
        while (parse_nchar("<=", op)
          || parse_nchar(">=", op)
          || parse_1charCheckNext('<', '<', op)
          || parse_1charCheckNext('>', '>', op))
        {
          Variant f;
          if (parse_shift_expression(&f, exec))
          {
            if (exec)
            {
              if (strncmp("<=", op, 2)==0)
                *result = (*result <= f);
              else if (strncmp(">=", op, 2)==0)
                *result = (*result >= f);
              else if (strncmp("<", op, 1)==0)
                *result = (*result < f);
              else if (strncmp(">", op, 1)==0)
                *result = (*result > f);
            }
          }
          else
            return false;
        }
        return psaver.release();
      }

      return false;
    }


    // equality_expression = relational_expression {('==' | '!=') relational_expression}
    bool parse_equality_expression(Variant* result, bool exec)
    {
      PosSaver psaver(m_input);

      if (parse_relational_expression(result, exec))
      {
        char op[2];
        while (parse_nchar("==", op) || parse_nchar("!=", op))
        {
          Variant f;
          if (parse_relational_expression(&f, exec))
          {
            if (exec)
            {
              if (strncmp("==", op, 2)==0)
                *result = (*result == f);
              else if (strncmp("!=", op, 2)==0)
                *result = (*result != f);
            }
          }
          else
            return false;
        }
        return psaver.release();
      }

      return false;
    }


    // and_expression = equality_expression {'&' equality_expression}
    bool parse_and_expression(Variant* result, bool exec)
    {
      PosSaver psaver(m_input);

      // equality_expression
      if (parse_equality_expression(result, exec))
      {
        // {'&' equality_expression}
        while (parse_1charCheckNext('&', '&'))
        {
          Variant t;
          if (parse_equality_expression(&t, exec))
          {
            if (exec)
              *result = *result & t;
          }
          else
            return false;
        }
        return psaver.release();
      }

      return false;
    }


    // exclusive_or_expression = and_expression {'^' and_expression}
    bool parse_exclusive_or_expression(Variant* result, bool exec)
    {
      PosSaver psaver(m_input);

      // and_expression
      if (parse_and_expression(result, exec))
      {
        // {'^' and_expression}
        while (parse_1char('^'))
        {
          Variant t;
          if (parse_and_expression(&t, exec))
          {
            if (exec)
              *result = *result ^ t;
          }
          else
            return false;
        }
        return psaver.release();
      }

      return false;
    }


    // inclusive_or_expression = exclusive_or_expression {'|' exclusive_or_expression}
    bool parse_inclusive_or_expression(Variant* result, bool exec)
    {
      PosSaver psaver(m_input);

      // exclusive_or_expression
      if (parse_exclusive_or_expression(result, exec))
      {
        // {'|' exclusive_or_expression}
        while (parse_1charCheckNext('|', '|'))
        {
          Variant t;
          if (parse_exclusive_or_expression(&t, exec))
          {
            if (exec)
              *result = *result | t;
          }
          else
            return false;
        }
        return psaver.release();
      }

      return false;
    }


    // logical_and_expression = inclusive_or_expression {"&&" inclusive_or_expression}
    bool parse_logical_and_expression(Variant* result, bool exec)
    {
      PosSaver psaver(m_input);

      // inclusive_or_expression
      if (parse_inclusive_or_expression(result, exec))
      {
        // {"&&" inclusive_or_expression}
        while (parse_nchar("&&"))
        {
          Variant t;
          if (parse_inclusive_or_expression(&t, exec))
          {
            if (exec)
              result->uint8Val() = result->toBool() && t.toBool();
          }
          else
            return false;
        }
        return psaver.release();
      }

      return false;
    }


    // logical_or_expression = logical_and_expression {"||" logical_and_expression}
    bool parse_logical_or_expression(Variant* result, bool exec)
    {
      PosSaver psaver(m_input);

      // logical_and_expression
      if (parse_logical_and_expression(result, exec))
      {
        // {"||" logical_and_expression}
        while (parse_nchar("||"))
        {
          Variant t;
          if (parse_logical_and_expression(&t, exec))
          {
            if (exec)
              result->uint8Val() = result->toBool() || t.toBool();
          }
          else
            return false;
        }
        return psaver.release();
      }

      return false;
    }


    // conditional_expression = logical_or_expression ['?' expression ':' expression]
    bool parse_conditional_expression(Variant* result, bool exec)
    {
      PosSaver psaver(m_input);

      if (parse_logical_or_expression(result, exec))
      {
        if (parse_1char('?'))
        {
          Variant f1, f2;
          if (!parse_expression(&f1, exec && result->toBool()==true)
            || !parse_1char(':')
            || !parse_expression(&f2, exec && result->toBool()==false))
            return false;
          *result = result->toBool()==true? f1 : f2;
        }
        return psaver.release();
      }

      return false;
    }


    // identifier_assign = variable '='
    bool parse_identifier_assign(string& vname, size_t* aindex, bool exec)
    {
      PosSaver psaver(m_input);

      if (parse_variable(vname, aindex, exec) && parse_1charCheckNext('=', '='))
        return psaver.release();

      return false;
    }


    // assignment_expression = {identifier_assign} conditional_expression
    bool parse_assignment_expression(Variant* result, bool exec)
    {
      PosSaver psaver(m_input);

      // just to bypass identifiers
      string vname;
      size_t aindex;
      while (parse_identifier_assign(vname, &aindex, exec));

      if (parse_conditional_expression(result, exec))
      {
        if (exec)
        {
          // reparse identifiers to do actual work
          psaver.restore();
          while (parse_identifier_assign(vname, &aindex, exec))
            m_runTime->setVariable( vname.c_str(), aindex, *result );
          // bypass conditional expression
          Variant f;
          parse_conditional_expression(&f, false);
        }
        return psaver.release();
      }

      return false;
    }


  public:

    // expression = logical_or_expression
    bool parse_expression(Variant* result, bool exec)
    {
      PosSaver psaver(m_input);

      // logical_or_expression
      if (parse_assignment_expression(result, exec))
        return psaver.release();

      return false;
    }


  private:


    // expression_statement = [expression] ';'
    bool parse_expression_statement(bool exec)
    {
      PosSaver psaver(m_input);

      Variant f;
      if (!parse_expression(&f, exec))
        return false;

      if (parse_1char(';'))
        return psaver.release();

      return false;
    }


    // compound_statement = '{' {statement} '}'
    bool parse_compound_statement(bool exec)
    {
      PosSaver psaver(m_input);

      if (parse_1char('{'))
      {
        while (!parse_1char('}'))
        {
          if (!parse_statement(exec))
            return false;
        }
        return psaver.release();
      }

      return false;
    }


    // selection_statement = "if" '(' expression ')' statement ["else" statement]
    bool parse_selection_statement(bool exec)
    {
      PosSaver psaver(m_input);

      Variant f;
      if (parse_nchar("if")
        && parse_1char('(')
        && parse_expression(&f, exec)
        && parse_1char(')')
        && parse_statement(exec && f.toBool()==true))
      {
        if (parse_nchar("else"))
        {
          if (!parse_statement(exec && f.toBool()==false))
            return false;
        }
        return psaver.release();
      }

      return false;
    }


    // iteration_statement = "while" '(' expression ')' statement
    //                     | "do" statement "while" '(' expression ')' ';'
    //                     | "for" '(' expression ';' expression ';' [expression] ')' statement
    bool parse_iteration_statement(bool exec)
    {
      PosSaver psaver(m_input);

      // "while" '(' expression ')' statement
      if (parse_nchar("while") && parse_1char('('))
      {
        size_t p1 = m_input->pos();
        while (true)
        {
          m_progStatus = ST_RUN;
          Variant f;
          if (!parse_expression(&f, exec) || !parse_1char(')'))
            return false;
          if (!parse_statement(exec && f.toBool()==true))
            return false;
          if (!exec || f.toBool()==false || m_progStatus==ST_BREAK)
            break;
          m_input->pos(p1);
        }
        m_progStatus = ST_RUN;
        return psaver.release();
      }
      else
        psaver.restore();

      // "do" statement "while" '(' expression ')' ';'
      if (parse_nchar("do"))
      {
        size_t p1 = m_input->pos();
        Variant f;
        do
        {
          m_progStatus = ST_RUN;
          m_input->pos(p1);
          if (!parse_statement(exec))
            return false;
          if (!parse_nchar("while")
            || !parse_1char('(')
            || !parse_expression(&f, exec)
            || !parse_1char(')')
            || !parse_1char(';'))
            return false;
        } while (f.toBool()==true && exec && m_progStatus!=ST_BREAK);
        m_progStatus = ST_RUN;
        return psaver.release();
      }

      // "for" '(' expression_statement expression ';' [expression] ')' statement
      if (parse_nchar("for")
        && parse_1char('(')
        && parse_expression_statement(exec))
      {
        size_t p1 = m_input->pos();
        for (;;)
        {
          m_progStatus = ST_RUN;
          m_input->pos(p1);
          Variant f1;
          if (!parse_expression(&f1, exec))
            f1 = Variant((uint8_t)true);  // evaluate true when missing test condition
          if (!parse_1char(';'))
            return false;
          Variant f2;
          size_t p2 = m_input->pos();
          parse_expression(&f2, false); // bypass
          if (!parse_1char(')'))
            return false;
          if (!parse_statement(exec && f1.toBool()==true))
            return false;
          if (f1.toBool()==false || !exec || m_progStatus==ST_BREAK)
            break;
          m_input->pos(p2);
          if (!parse_expression(&f2, exec))
            return false;
        }
        m_progStatus = ST_RUN;
        return psaver.release();
      }
      else
        psaver.restore();

      return false;
    }


    // jump_statement = "continue" ';'
    //                | "break" ';'
    bool parse_jump_statement(bool exec)
    {
      PosSaver psaver(m_input);

      // "continue" ';'
      if (parse_nchar("continue") && parse_1char(';'))
      {
        if (exec)
          m_progStatus = ST_CONTINUE;
        return psaver.release();
      }

      // "break" ';'
      if (parse_nchar("break") && parse_1char(';'))
      {
        if (exec)
          m_progStatus = ST_BREAK;
        return psaver.release();
      }

      return false;
    }


    // output_statement = ':' expression {':' expression} ';'
    bool parse_output_statement(bool exec)
    {
      PosSaver psaver(m_input);

      if (parse_1char(':'))
      {
        Variant result;
        if (!parse_expression(&result, exec))
          return false;
        if (exec && m_runTime->output)
          m_runTime->output->write( result.toString().c_str() );
        while (parse_1char(':'))
        {
          if (!parse_expression(&result, exec))
            return false;
          if (exec && m_runTime->output)
            m_runTime->output->write( result.toString().c_str() );
        }
        if (!parse_1char(';'))
          return false;
        return psaver.release();
      }

      return false;
    }


    // output until "<?" or EOF
    void directOutput(bool exec)
    {
      while (!m_input->isEOF())
      {
        char c = m_input->get();
        m_input->next();
        if (!m_input->isEOF())
        {
          if (c=='<' && m_input->get()=='?')
          {
            m_input->next();  // bypass '?'
            m_incode = true;
            break;
          }
          else if (exec && m_runTime->output)
            m_runTime->output->write( c );
        }
      }
    }


    // statement = "<?"
    //           | "?>"
    //           | "//" all_chars EOL
    //           | output_statement
    //           | compound_statement
    //           | expression_statement
    //           | selection_statement
    //           | iteration_statement
    //           | jump_statement
    bool parse_statement(bool exec)
    {
      PosSaver psaver(m_input);

      exec = exec && m_progStatus==ST_RUN;  // to handle "break" and "continue"

      if (m_incode)
      {

        // "?>"
        if (parse_nchar("?>"))
        {
          m_incode = false;
          directOutput(exec); // this handles also "<?"
          return psaver.release();
        }

        // "//" all_chars EOL
        if (parse_nchar("//"))
        {
          while (!m_input->isEOF())
          {
            char c = m_input->get();
            m_input->next();
            if (c==0x0A)  // LF
              break;
            if (c==0x0D && !m_input->isEOF() && m_input->get()==0x0A)  // CR+LF
            {
              m_input->next();  // bypass LF
              break;
            }
          }
          return psaver.release();
        }

        // output_statement
        if (parse_output_statement(exec))
          return psaver.release();

        // compound_statement
        if (parse_compound_statement(exec))
          return psaver.release();

        // expression_statement
        if (parse_expression_statement(exec))
          return psaver.release();

        // selection_statement
        if (parse_selection_statement(exec))
          return psaver.release();

        // iteration_statement
        if (parse_iteration_statement(exec))
          return psaver.release();

        // jump_statement
        if (parse_jump_statement(exec))
          return psaver.release();

      }
      else
      {
        directOutput(exec);  // this handles also "<?"
        return psaver.release();
      }

      return false;
    }



  public:

    // script = {statement}
    bool parse_script()
    {
      PosSaver psaver(m_input);

      while (!m_input->isEOF())
      {
        if (!parse_statement(true))
          return false;
      }
      return psaver.release();
    }


  private:

    enum ProgStatus {ST_RUN, ST_BREAK, ST_CONTINUE};

    RunTimeT*  m_runTime;
    InputBase* m_input;
    ProgStatus m_progStatus;
    bool       m_incode;  // true if inside "<?"..."?>" block

  };


  template <typename RunTimeT>
  inline bool parseScript(RunTimeT* runtime, InputBase* input)
  {
    return Script<RunTimeT>(runtime, input).parse_script();
  }


} // end of fdv namespace


#endif /* FDV_SCRIPT_H_ */
