// 2010 by Fabrizio Di Vittorio (fdivitto@tiscali.it)




#ifndef FDV_UDPCLIENT_H
#define FDV_UDPCLIENT_H



#include <string.h>
#include <stdlib.h>
#include <alloca.h>
#include <inttypes.h>
#include <avr/pgmspace.h>


#include "../fdv_generic/fdv_string.h"
#include "../fdv_generic/fdv_algorithm.h"
#include "../fdv_generic/fdv_datetime.h"
#include "../fdv_generic/fdv_timesched.h"
#include "fdv_ethernet.h"
#include "fdv_socket.h"


namespace fdv
{

  
  ////////////////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////////////////
  // UDPClient
  
  class UDPClient
  {
    
  public:

    UDPClient(Ethernet* ethernet) :
      m_ethernet(ethernet)
    {
      // get a socket for UDP transmission
      m_socket = m_ethernet->findFreeSocket();
      uint16_t port = m_ethernet->findFreePort();
      if (m_socket!=255 && port!=0)
        ::socket(m_ethernet->W5100(), m_socket, SnMR::UDP, port, 0);
      else
        m_socket = 255; // fail state
    }
    
    
    ~UDPClient()
    {
      if (m_socket != 255)
        ::close(m_ethernet->W5100(), m_socket);
    }
    
    
    bool send(char const* destIP, uint16_t destPort, uint8_t const* buffer, uint16_t size)
    {
      if (m_socket != 255)
      {
        uint8_t addr[4];
        Ethernet::parseAddr(destIP, addr, 10, 4);
        return ::sendto(m_ethernet->W5100(), m_socket, buffer, size, &addr[0], destPort) == size;
      }
      else
      {
        return false;
      }
    }
    
    
    uint16_t get(uint8_t* buffer, uint16_t size)
    {
      if (m_socket != 255)
      {
        size = min(dataAvailable(), size);
        uint8_t addr[4];
        uint16_t port;
        return ::recvfrom(m_ethernet->W5100(), m_socket, buffer, size, &addr[0], &port);
      }
        return 0;
    }
    

    uint16_t dataAvailable()
    {
      if (m_socket != 255)
        return m_ethernet->W5100()->readSnRX_RSR(m_socket);
      else
        return 0;
    }
    
    
  private:
    
    Ethernet* m_ethernet;
    SOCKET    m_socket;
    
  };

  
  
  
  
  
} // end of fdv namespace


#endif  // FDV_UDPCLIENT_H
