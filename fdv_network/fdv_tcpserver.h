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



#ifndef FDV_TCPSERVER_H
#define FDV_TCPSERVER_H


#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include <math.h>
#include <avr/pgmspace.h>


#include "../fdv_generic/fdv_timesched.h"
#include "fdv_ethernet.h"
#include "fdv_socket.h"



namespace fdv
{

    
  

  ////////////////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////////////////
  // TCPServer


  struct ITCPHandler
  {
    virtual void initConnection(Socket* socket) = 0;
    virtual void endConnection() = 0;
    virtual void listening() = 0;
    virtual void receiveData() = 0;
    virtual ~ITCPHandler() { }
  };


  typedef ITCPHandler* (*HandlerCreator)(void* userData);


  class TCPServer
  {
  public:

    
    // constructor
    TCPServer(Ethernet* ethernet, uint16_t port, uint8_t maxConnections, HandlerCreator handlerCreator, void* userData = NULL) :
      m_ethernet(ethernet), m_port(port), m_maxConnections(maxConnections), m_handlerCreator(handlerCreator), m_userData(userData)
    {
      init();
    }
    
    
    // default constructor (you have to assign "port", "maxConnections", "ethernet", "handlerCreatore" before call listen())
    TCPServer() :
      m_ethernet(NULL), m_port(0), m_maxConnections(0), m_handlerCreator(NULL), m_userData(NULL)
    {
      init();
    }



  private:

    void init()
    {
      for (uint8_t i=0; i!=MAX_SOCK_NUM; ++i)
        m_bindsHandler[i] = NULL;
    }

    
    void closeAllSockets()
    {
      for (SOCKET s=0; s!=MAX_SOCK_NUM; ++s)
      {
        if (m_bindsHandler[s]!=NULL)
          removeBind(s);
        if (ownSocket(s))
        {
          close(m_ethernet->W5100(), s);
          m_ethernet->sockStatus(s, UNUSED);
        }
      }
    }


  public:


    // destructor
    ~TCPServer()
    {
      closeAllSockets();
    }
    

    void handlerCreator(HandlerCreator value)
    {
      m_handlerCreator = value;
    }


    void ethernet(Ethernet* value)
    {
      m_ethernet = value;
    }

        
    void port(uint16_t value)
    {
      m_port = value;
    }
    
    
    uint16_t port()
    {
      return m_port;
    }
    
    
    void maxConnections(uint8_t value)
    {
      m_maxConnections = value;
    }
    
    
    void userData(void* value)
    {
      m_userData = value;
    }
    
    
    void listen()
    {
      if (m_port == 0)
        return; // not still active            
      
      // are there closing sockets?
      for (uint8_t i=0; i!=MAX_SOCK_NUM; ++i)
      {
        if (m_bindsHandler[i]!=NULL && (!m_bindsSocket[i]->connected() || m_bindsSocket[i]->closing()))
        {
          removeBind(i);
          break;  // here, iterator is no more valid
        }
      }

      // are there listening sockets?
      if (countOwnedSockets() < m_maxConnections && !listeningSockets())
      {
        // no sockets listening, we need another one
        SOCKET s = m_ethernet->findFreeSocket();
        if (s != 255)
          listenSocket(s);
      };      
      
      // looks for new connections
      // note about "m_ethernet->sockInternalStatus(s) == SnSR::CLOSE_WAIT":
      //            -> this is the case when a short message is sent (like FTP when sends short files). The packet is sent and CLOSE_WAIT status is immediately set.
      for (SOCKET s=0; s!=MAX_SOCK_NUM; ++s)
        if (ownSocket(s) && 
            (m_ethernet->sockInternalStatus(s) == SnSR::ESTABLISHED || m_ethernet->sockInternalStatus(s) == SnSR::CLOSE_WAIT) &&
            m_ethernet->sockStatus(s) == LISTENING)
        {
          s_networkIdleTime = millis();
          m_ethernet->sockStatus(s, CONNECTED);
          createSocketServerBind(s);          
        }

      // looks for data available
      for (uint8_t i=0; i!=MAX_SOCK_NUM; ++i)
      {
        if (m_bindsHandler[i]!=NULL)
        {
          m_bindsHandler[i]->listening();  // used for chained listening (ie FTP)
          if (m_bindsSocket[i]->dataAvailable() > 0)
          {
            s_networkIdleTime = millis();
            m_bindsHandler[i]->receiveData();
          }
        }
      }
    }
    
            
  private:
      
        
    // responds to the question: are there listening sockets of this server instance?
    bool listeningSockets()
    {
      for (SOCKET s=0; s!=MAX_SOCK_NUM; ++s)
        if (ownSocket(s) && m_ethernet->sockInternalStatus(s)==SnSR::LISTEN)
          return true;
      return false;
    }
    
    
    // returns number of owned sockets (in listen or connected state)
    uint8_t countOwnedSockets()
    {
      uint8_t count = 0;
      for (SOCKET s=0; s!=MAX_SOCK_NUM; ++s)
        if (ownSocket(s))
          ++count;
      return count;
    }
    
    
    // creates and binds the handler object and Socket for specified SOCKET
    void createSocketServerBind(SOCKET s)
    {
      Socket* sck = new Socket(m_ethernet, s);
      ITCPHandler* hdl = m_handlerCreator(m_userData);

      m_bindsSocket[s]  = sck;
      m_bindsHandler[s] = hdl;

      hdl->initConnection(sck);
    }    
    
    
    // removes socket and server bind
    void removeBind(uint8_t bind)
    {
      //debug << "removeBind(" << (uint16_t)bind << "),  port=" << m_bindsSocket[bind]->getLocalPort() << ENDL;
      SOCKET s = m_bindsSocket[bind]->handle();
      ::disconnect(m_ethernet->W5100(), s);
      uint32_t start = millis();
      while (m_ethernet->sockInternalStatus(s) != SnSR::CLOSED && millisDiff(start, millis()) < 2000);
      if (m_ethernet->sockInternalStatus(s) != SnSR::CLOSED)
        ::close(m_ethernet->W5100(), s);
      
      m_ethernet->sockStatus(s, UNUSED);
      m_bindsHandler[bind]->endConnection();

      delete m_bindsHandler[bind];
      m_bindsHandler[bind] = NULL;  // free marker
      delete m_bindsSocket[bind];
    }
    
    
    // switches a socket in listen mode
    void listenSocket(SOCKET s)
    {
      ::socket(m_ethernet->W5100(), s, SnMR::TCP, m_port, 0);
      while (!::listen(m_ethernet->W5100(), s));
      m_ethernet->sockStatus(s, LISTENING);
    }
    
    
    // checks if the specified socket is owned by this tcp server
    bool ownSocket(SOCKET s)
    {
      return m_ethernet->getLocalPort(s) == m_port && m_ethernet->sockStatus(s) != UNUSED;
    }
      
    
  private:
    
    Ethernet*      m_ethernet;
    uint16_t       m_port;
    uint8_t        m_maxConnections;
    HandlerCreator m_handlerCreator;
    void*          m_userData;

    Socket*        m_bindsSocket[MAX_SOCK_NUM];
    ITCPHandler*   m_bindsHandler[MAX_SOCK_NUM];
  };

  
  
  
} // end of fdv namespace


#endif  // FDV_TCPSERVER_H


