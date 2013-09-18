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




#ifndef FDV_NTPCLIENT_H
#define FDV_NTPCLIENT_H


#include <string.h>
#include <stdlib.h>
#include <alloca.h>
#include <inttypes.h>
#include <avr/pgmspace.h>
  


#include "../fdv_generic/fdv_string.h"
#include "../fdv_generic/fdv_algorithm.h"
#include "../fdv_generic/fdv_datetime.h"
#include "../fdv_generic/fdv_timesched.h"
#include "fdv_socket.h"
#include "fdv_udpclient.h"



namespace fdv
{

  
  ////////////////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////////////////
  // SNTPClient
  // Gets current date/time from a NTP (SNTP) server (default is pool.ntp.org).
  
  class SNTPClient
  {
    
  public:
    
    explicit SNTPClient(Ethernet* ethernet, char const* serverIP = "169.229.70.64", uint16_t port = 123)
      : m_ethernet(ethernet), m_server(serverIP), m_port(port)
    {      
    }
    
    
    bool query(uint8_t timeZone, DateTime& outValue) const
    {
      // send req (mode 3), unicast, version 4
      uint8_t const MODE_CLIENT = 3;
      uint8_t const VERSION = 4;
      uint8_t const BUFLEN = 48;
      uint8_t buf[48];
      memset(&buf[0], 0, BUFLEN);
      buf[0] = MODE_CLIENT | (VERSION << 3);
      UDPClient udp(m_ethernet);
      if ( udp.send(m_server.c_str(), m_port, &buf[0], BUFLEN) )
      {
        // get reply
        delay(300); // wait for 300ms the reply (should be subtracted...)
        if (udp.dataAvailable() >= BUFLEN && udp.get(&buf[0], BUFLEN) == BUFLEN)
        {
          outValue.setNTPDateTime(&buf[40], timeZone);
          return true;  // ok
        }
      }
      return false;  // error
    }
    

  private:

    Ethernet* m_ethernet;
    string    m_server;
    uint16_t  m_port;
  };
  
  
  
  
} // end of fdv namespace


#endif  // FDV_NTPCLIENT_H
