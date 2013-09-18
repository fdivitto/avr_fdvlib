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





#ifndef FDV_SOCKET_H
#define FDV_SOCKET_H


#include <string.h>
#include <stdlib.h>
#include <alloca.h>
#include <inttypes.h>
#include <avr/pgmspace.h>
  
#include "w5100.h"
#include "socket.h"


#include "../fdv_generic/fdv_string.h"
#include "../fdv_generic/fdv_algorithm.h"
#include "../fdv_generic/fdv_datetime.h"
#include "../fdv_generic/fdv_timesched.h"
#include "fdv_ethernet.h"


namespace fdv
{

  
  ////////////////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////////////////
  // Socket

  class Socket
  {
  public:
    
    // constructor
    explicit Socket(Ethernet* eth, SOCKET s) :
      m_ethernet(eth), m_W5100(eth->W5100()), m_socket(s), m_closing(false)
    {
    }

    
    // writes one byte
    void write(uint8_t b) const
    {
      send(m_W5100, m_socket, &b, 1);
    }
    
    
    void write(char const* buffer, uint16_t size) const
    {
      //cout << "write(char const*, uint16_t) " << size << ":" << string(buffer, buffer+size) << endl;
      while (size>0 && connected() && !closing())
      {
        uint16_t sent = send(m_W5100, m_socket, (uint8_t const*)buffer, size);
        size -= sent;
        buffer += sent;        
      }
    }
    

    void write(char const* buffer) const
    {
      write(buffer, strlen(buffer));
    }
    
    
    void write(string const& buffer) const
    {
      write(&buffer[0], buffer.size());
    }
    
    
    Socket const* write_P(prog_char const* str) const
    {      
      uint16_t l = strlen_P(str);
      if (l > 0)
      {
        void* sbuf = alloca(l);
        memcpy_P(sbuf, str, l);
        write((char const*)sbuf, l);
      }
      return this;
    }    
    
    
    // returns available bytes
    uint16_t dataAvailable() const
    {
      uint16_t r = m_W5100->getRXReceivedSize(m_socket);
      //cout << "dataAvailable() -> " << r << endl;
      return r <= 2048? r : 0;
    }
    
    
    // reads "size" bytes
    // returns received data size
    uint16_t read(void* buf, uint16_t size) const
    {
      return recv(m_W5100, m_socket, (uint8_t*)buf, size);
    }
    
    
    // reads one byte
    uint8_t read() const
    {
      uint8_t b;
      recv(m_W5100, m_socket, &b, 1);
      return b;
    }
    
    
    uint8_t status() const
    {
      return m_socket==255? SnSR::CLOSED : m_ethernet->sockInternalStatus(m_socket);
    }
    
    
    SockStatus statusEx() const
    {
      return m_ethernet->sockStatus(m_socket);
    }


    void flush()
    {
      while (connected() && m_W5100->getTXFreeSize(m_socket) != W5100Class::SSIZE);
    }
    
    
    bool connected() const
    {
      //return status() == SnSR::ESTABLISHED;
      /*
      uint8_t s = status();
      return !m_closing && !(s == SnSR::LISTEN || s == SnSR::CLOSED || s == SnSR::FIN_WAIT || (s == SnSR::CLOSE_WAIT && !dataAvailable()));
       */
      uint8_t s = status();
      //cout << "s:" << (uint16_t)m_socket << "  connected() -> " << (uint16_t)s << "  dataAvail()=" << dataAvailable() << endl;
      return s == SnSR::ESTABLISHED || (s == SnSR::CLOSE_WAIT && dataAvailable() > 0);
    }
    
    
    void close()
    {
      m_closing = true; // will be actually closed at next related TCPServer.listen() call
    }
    
    
    bool closing() const
    {
      return m_closing;
    }
    
    
    SOCKET handle() const
    {
      return m_socket;
    }
    
    
    Ethernet* ethernet() const
    {
      return m_ethernet;
    }



    // reads local socket port (listening port for server, source port for client)
    uint16_t getLocalPort()
    {
      return m_ethernet->getLocalPort(m_socket);
    }


    // reads remote port
    uint16_t getRemotePort()
    {
      return m_ethernet->getRemotePort(m_socket);
    }


    // reads remote IP address (buf must be uint8_t array, 4 elements)
    void getRemoteIP(uint8_t* buf)
    {
      m_ethernet->getRemoteIP(m_socket, buf);
    }


    // reads remote IP address as string
    string const getRemoteIP(char sep = '.')
    {
      return m_ethernet->getRemoteIP(m_socket, sep);
    }


    // reads local IP address (buf must be uint8_t array, 4 elements)
    void getLocalIP(uint8_t* buf)
    {
      m_ethernet->getLocalIP(buf);
    }


    string const getLocalIP(char sep = '.')
    {
      return m_ethernet->getLocalIP(sep);
    }


  private:
    
    Ethernet*   m_ethernet;
    W5100Class* m_W5100;
    SOCKET      m_socket;
    bool        m_closing;
  };
  

  inline Socket& operator << (Socket& socket, string const& str)
  {
    socket.write(str);
    return socket;
  }

  
  inline Socket& operator << (Socket& socket, char const* str)
  {
    socket.write(str, strlen(str));
    return socket;
  }


  inline Socket& operator << (Socket& socket, char c)
  {
    socket.write(&c, 1);
    return socket;
  }
  
  
  inline Socket& operator << (Socket& socket, uint32_t v)
  {
    char buf[13];
    fmtUInt32(v, &buf[0], 12);
    socket.write(&buf[0]);
    return socket;
  }

  
  inline Socket& operator << (Socket& socket, uint16_t v)
  {
    char buf[7];
    fmtUInt32(v, &buf[0], 6);
    socket.write(&buf[0]);
    return socket;
  }

  
  inline Socket& operator << (Socket& socket, uint8_t v)
  {
    char buf[5];
    fmtUInt32(v, &buf[0], 4);
    socket.write(&buf[0]);
    return socket;
  }

  
  inline Socket& operator << (Socket& socket, float v)
  {
    char str[16];
    fmtDouble(v, 1, &str[0], 15); // default precision "1", otherwise use "toString"
    socket.write(&str[0]);
    return socket;
  }
  
  
  
  

  
  
} // end of fdv namespace


#endif  // FDV_SOCKET_H
