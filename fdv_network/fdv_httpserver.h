// 2010 by Fabrizio Di Vittorio (fdivitto@tiscali.it)



#ifndef FDV_HTTPSERVER_H
#define FDV_HTTPSERVER_H


#include <string.h>
#include <stdlib.h>
#include <alloca.h>
#include <inttypes.h>
#include <avr/pgmspace.h>


#include "../fdv_generic/fdv_string.h"
#include "../fdv_generic/fdv_algorithm.h"
#include "../fdv_generic/fdv_datetime.h"
#include "../fdv_generic/fdv_timesched.h"
#include "../fdv_generic/fdv_base64.h"
#include "fdv_ethernet.h"
#include "fdv_socket.h"
#include "fdv_tcpserver.h"


namespace fdv
{

  
  ////////////////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////////////////
  // HTTPServer
  
  class HTTPServer : public ITCPHandler
  {
    
  public:
    
    static uint16_t const MAXLINE = 250;

    Socket* sck;

    
    explicit HTTPServer(void* userData, bool defaultReplyHeader = true)
      : m_defaultReplyHeader(defaultReplyHeader)
    {
    }
    
    
    virtual ~HTTPServer()
    {
    }


    void initConnection(Socket* socket)
    {      
      sck = socket;
      m_linePos = 0;
    }
    
    
    void endConnection()
    {
      //cout << "HTTPServer.endConnection\n";
    }
    
    
    void listening()
    {      
    }
    

    void receiveData()
    {            
      do
      {
        uint16_t dataSize = min(sck->dataAvailable(), MAXLINE-m_linePos);

        sck->read(&m_line[m_linePos], dataSize);
        m_linePos += dataSize;
                
        // replace /n and /r with 0
        uint16_t endp;
        for (endp=0; endp<m_linePos; ++endp)
          if (m_line[endp]=='\n' || m_line[endp]=='\r')
          {
            m_line[endp++] = '\0';
            if (endp<m_linePos && (m_line[endp]=='\n' || m_line[endp]=='\r'))
              m_line[endp++] = '\0';
            break;
          }
        
        if (endp >= MAXLINE)
          m_line[endp-1] = 0; // overflow, truncate
        
        if (m_line[endp-1] != 0)
          return;  // waiting for \n or \r        
        
        //debug << m_line << ENDL;

        if (strncasecmp_P(m_line, PSTR("GET "), 4)==0)
        {
          // process GET
          char* pos = strchr(&m_line[4], ' ');
          if (pos!=NULL) *pos = 0;
          m_reqPage = string(&m_line[4]);
        }
        else if (strncasecmp_P(m_line, PSTR("COOKIE: "), 8)==0)
        {
          // process COOKIE
          if (m_cookie.size()>0)
            m_cookie.push_back(';');
          m_cookie.append( &m_line[8] );
        }
        else if (strncasecmp_P(m_line, PSTR("AUTHORIZATION: Basic "), 8)==0)
        {
          // process AUTHORIZATION
          decodeAuth( &m_line[21] );
        }
        else if (m_line[0]==0)
        {
          processGET();
          return;
        }

        // discard this line and align other data
        int16_t l = m_linePos - endp;
        if (l>0)
        {
          memmove(&m_line[0], &m_line[endp], l);
          m_linePos = l;
        }
        else
          m_linePos = 0;
      } while (m_linePos > 0);            
    }    
    

    void processGET()
    {
      // check authorization
      if (requirePassword(m_reqPage, m_cookie) && !checkUser(m_reqPage, m_authUser, m_authPass))
      {
        sck->write_P(PSTR( "HTTP/1.1 401 Authorization Required\x0d\x0a"
                           "WWW-Authenticate: Basic\x0d\x0a\x0d\x0a"
                           "401 Unauthorized."));
        terminateConnection();
        return;
      }
      // send header
      if (m_defaultReplyHeader)
        sendDefaultReplyHeader();
      // send page
      commandGET(m_reqPage, m_cookie);
      // end conn
      terminateConnection();
    }


    void terminateConnection()
    {
      sck->flush();
      delay(50);
      sck->close();
    }


    void sendDefaultReplyHeader()
    {
      sck->write_P(PSTR( "HTTP/1.1 200 OK\x0d\x0a"
                         "Connection: close\x0d\x0a"
                         "Content-Type: text/html; charset=UTF-8\x0d\x0a" ));
    }


    void decodeAuth(char const* auth)
    {
      int16_t len = Base64::calcLength(auth);
      string buf(len, '\0');
      Base64::decode(&buf[0], auth);
      char const* p = strchr(&buf[0], ':');
      if (p)
      {
        m_authUser.assign(&buf[0], p);
        m_authPass.assign(p+1);
      }
    }


    // returns true if the page requires a userid/password
    virtual bool requirePassword(string& page, string const& cookie) = 0;

    // returns true if the specified user can get the specified page
    virtual bool checkUser(string& page, string const& user, string const& password) = 0;

    virtual void commandGET(string& page, string const& cookie) = 0;
    
    
  private:
    
    char     m_line[MAXLINE+1];
    uint16_t m_linePos;
    string   m_reqPage;  // from HTTP "Get" header
    string   m_cookie;   // from HTTP "Cookie" header
    string   m_authUser; // from HTTP "Authorization" header
    string   m_authPass; // from HTTP "Authorization" header
    bool     m_defaultReplyHeader;
    
  };
  
  
  
} // end of fdv namespace


#endif  // FDV_HTTPSERVER_H
