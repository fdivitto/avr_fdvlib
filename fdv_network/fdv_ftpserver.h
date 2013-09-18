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




#ifndef FDV_FTPSERVER_H
#define FDV_FTPSERVER_H


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
#include "fdv_tcpserver.h"


namespace fdv
{

  // forward declaration
  class FTPDataServer;
  
  // forward declaration
  static ITCPHandler* FTPDataServerCreator(void* userData);
  


  ////////////////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////////////////
  // FTPServer
  
  class FTPServer : public ITCPHandler
  {
    
  public:
    
    static uint8_t const MAXLINE = 64;
    
    
    enum DataCmd
    {
      DATA_CMD_NONE,
      DATA_CMD_MLSD,
      DATA_CMD_RETR,
      DATA_CMD_LIST,
      DATA_CMD_RNFR,  // not actually a datacommand (handled directly by FTPServer)
      DATA_CMD_STOR
    };
    

    Socket* sck;

    
    // data connection command (used also for RNFR-RNTO)
    string  dataCmdParams;  
    DataCmd dataCmd;        
    
    
    explicit FTPServer(void* userData) :
      dataCmd(DATA_CMD_NONE), m_authenticated(false)
    {      
    }    

    
    virtual ~FTPServer()
    {
    }


    virtual void initConnection(Socket* socket)
    {
      sck = socket;
      m_linePos = 0;

      // setup data server
      uint16_t port = sck->ethernet()->findFreePort();
      if (port == 0)
        sck->close();  // failed to open data port
      else
      {
        m_dataServer.ethernet(sck->ethernet());
        m_dataServer.maxConnections(1);
        m_dataServer.userData(this);
        m_dataServer.handlerCreator(FTPDataServerCreator);
        m_dataServer.port(port);
      }
    }
    

    virtual bool authenticate(char const* user, char const* password) = 0;
    
    
    virtual string const getCurrentDirectory() = 0;
    
    
    virtual void setCurrentDirectory(char const* dirname) = 0;

    
    virtual void mlsd(Socket* dataSocket, char const* dirname) = 0;
    
    
    virtual void list(Socket* dataSocket, char const* dirname) = 0;
    
    
    virtual void fileRead(Socket* dataSocket, char const* fullPath) = 0;
    
    
    virtual void fileWrite(Socket* dataSocket, char const* fullPath) = 0;
    
    
    virtual void fileMove(char const* srcPath, char const* dstPath) = 0;
    
    
    virtual bool fileExists(char const* path) = 0;
    
    
    virtual void fileDelete(char const* path) = 0;
    
    
    virtual void dirCreate(char const* path) = 0;
    
    
    virtual void dirDelete(char const* path) = 0;
    
    
    virtual void endConnection()
    {
    }
    
    
    void listening()
    {      
      m_dataServer.listen();  // active only when m_dataServer.port() is not zero
    }
    
    
    void receiveData()
    {            
      uint16_t dataSize = sck->dataAvailable();
      if (m_linePos+dataSize >= MAXLINE)
      {
        // buffer overrun
        sck->write_P(PSTR("Don't hack me!\n"));
        sck->close();
        return;
      }
      
      sck->read(&m_line[m_linePos], dataSize);
      m_linePos += dataSize;
      m_line[m_linePos] = '\0';
      
      // replace /n and /r with 0
      bool retFound = false;
      for (uint8_t i=0; i<m_linePos; ++i)
        if (m_line[i]=='\n' || m_line[i]=='\r')
        {
          m_line[i] = '\0';
          retFound = true;
        }
      
      if (!retFound)
        return;  // waiting for \n or \r
      
      // process command
      if (isCMD_P(&m_line[0], PSTR("USER ")))
        cmd_user(&m_line[5]);
      else if (isCMD_P(&m_line[0], PSTR("PASS ")))
        cmd_pass(&m_line[5]);
      else if (isCMD_P(&m_line[0], PSTR("SYST")))
        cmd_syst();
      else if (isCMD_P(&m_line[0], PSTR("FEAT")))
        cmd_feat();
      else if (isCMD_P(&m_line[0], PSTR("QUIT")))
        cmd_quit();
      else if (m_authenticated)
      {
        if (isCMD_P(&m_line[0], PSTR("PWD")))
          cmd_pwd();
        else if (isCMD_P(&m_line[0], PSTR("TYPE ")))
          cmd_type(&m_line[5]);
        else if (isCMD_P(&m_line[0], PSTR("PASV")))
          cmd_pasv();
        else if (isCMD_P(&m_line[0], PSTR("MLSD")))
          cmd_mlsd(&m_line[5]);
        else if (isCMD_P(&m_line[0], PSTR("CWD ")))
          cmd_cwd(&m_line[4]);
        else if (isCMD_P(&m_line[0], PSTR("RETR ")))
          cmd_retr(&m_line[5]);
        else if (isCMD_P(&m_line[0], PSTR("STOR ")))
          cmd_stor(&m_line[5]);
        else if (isCMD_P(&m_line[0], PSTR("LIST ")))
          cmd_list(&m_line[5]);
        else if (isCMD_P(&m_line[0], PSTR("LIST")))
          cmd_list(&m_line[4]);
        else if (isCMD_P(&m_line[0], PSTR("RNFR ")))
          cmd_rnfr(&m_line[5]);
        else if (isCMD_P(&m_line[0], PSTR("RNTO ")))
          cmd_rnto(&m_line[5]);
        else if (isCMD_P(&m_line[0], PSTR("DELE ")))
          cmd_dele(&m_line[5]);
        else if (isCMD_P(&m_line[0], PSTR("MKD ")))
          cmd_mkd(&m_line[4]);
        else if (isCMD_P(&m_line[0], PSTR("RMD ")))
          cmd_rmd(&m_line[4]);
        else if (isCMD_P(&m_line[0], PSTR("ABOR")))
          cmd_abor();
        else if (m_line[0]!=0)
          sendResultCode(500, false);
      }      	
      else if (m_line[0]!=0)
        sendResultCode(500, false);
              
      m_linePos = 0;      
    }    
    
    
  private:
    
    bool isCMD_P(char const* line, prog_char const* cmd)
    {
      uint8_t l = strlen_P(cmd);
      return strncasecmp_P(line, cmd, l) == 0;
    }
    
    
    void sendResultCode(uint16_t code, bool success)
    {
      *sck << toString(code);
      sck->write_P(success? PSTR(" OK\r\n") : PSTR(" error\r\n"));
    }
    
    
    void cmd_user(char const* params)
    {
      m_user = params;
      sendResultCode(331, true);
    }
    
    
    void cmd_pass(char const* params)
    {
      if (authenticate(&m_user[0], params))
      {
        m_authenticated = true;
        sendResultCode(230, true);
      }
      else
      {
        sendResultCode(530, false);
        sck->close();
      }
    }
    
    
    void cmd_syst()
    {
      //sck->write_P(PSTR("215 UNIX Type: L8\r\n"));
      sck->write_P(PSTR("215 AVR8\r\n"));
    }
    
    
    void cmd_feat()
    {
      sck->write_P(PSTR("211-Extensions supported:\r\n"
                        " PASV\r\n"
                        " MLST Type*;Size*;Modify*;\r\n"
                        "211 End.\r\n"
                   ));
    }
    
    
    void cmd_pwd()
    {
      sck->write_P(PSTR("257 \""));
      *sck << getCurrentDirectory();
      sck->write_P(PSTR("\"\r\n"));
    }
    
    
    // not actually implemented
    void cmd_type(char const* params)
    {
      if (strcmp_P(params, PSTR("I"))==0)
        sendResultCode(200, true);
      else if (strcmp_P(params, PSTR("A"))==0)
        sendResultCode(200, true);
      else
      {
        sendResultCode(500, false);
      }
    }
    
    
    // (ip,ip,ip,ip,high_prt,low_port)
    void cmd_pasv()
    {
      sck->write_P(PSTR("227 ("));
      *sck << sck->ethernet()->getLocalIP(',') << ',';
      *sck << (uint16_t)((m_dataServer.port() >> 8) & 0xFF) << ',' << (uint16_t)(m_dataServer.port() & 0xFF);
      sck->write_P(PSTR(")\r\n"));
    }


  private:
    
    // Type*;Size*;Modify*;
    void cmd_mlsd(char const* params)
    {
      dataCmd       = DATA_CMD_MLSD;
      dataCmdParams = params;
      //sendResultCode(150, true);
    }
    
    
    void cmd_list(char const* params)
    {
      dataCmd       = DATA_CMD_LIST;
      dataCmdParams = params;
      //sendResultCode(150, true);
    }
    
    
    void cmd_cwd(char const* params)
    {
      setCurrentDirectory(params);
      *sck << "250 \'"<< getCurrentDirectory() << "\'\r\n";
    }
    
    
    void cmd_retr(char const* params)
    {
      dataCmd       = DATA_CMD_RETR;
      dataCmdParams = params;
      //sendResultCode(150, true);
    }
    
    
    void cmd_stor(char const* params)
    {
      dataCmd       = DATA_CMD_STOR;
      dataCmdParams = params;
    }

    
    void cmd_rnfr(char const* params)
    {      
      if (fileExists(params))
      {
        dataCmd       = DATA_CMD_RNFR;
        dataCmdParams = params;
        sendResultCode(350, true);
      }
      else
      {
        sendResultCode(550, false);
      }
    }
    
    
    void cmd_rnto(char const* params)
    {      
      if (dataCmd!=DATA_CMD_RNFR)
      {
        sendResultCode(503, false);
      }
      else
      {
        fileMove(&dataCmdParams[0], params);
        sendResultCode(250, true);
        dataCmd = DATA_CMD_NONE;
        dataCmdParams.clear();
      }
    }

    
    void cmd_dele(char const* params)
    {    
      fileDelete(params);
      sendResultCode(250, true);
    }
    
    
    void cmd_mkd(char const* params)
    {
      dirCreate(params);
      sendResultCode(257, true);
    }
    
    
    void cmd_rmd(char const* params)
    {
      dirDelete(params);
      sendResultCode(250, true);
    }
    
    
    void cmd_abor()
    {
      dataCmd = DATA_CMD_NONE;
      sendResultCode(225, true);
    }
    
    
    void cmd_quit()
    {
      sendResultCode(221, true);
      sck->close();
    }
    
    
  private:
    
    bool                     m_authenticated;
    char                     m_line[MAXLINE+1];
    uint8_t                  m_linePos;
    string                   m_user;
    TCPServer                m_dataServer;
    
  };
  
  
  
  
  ////////////////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////////////////
  // FTPDataServer
  
  class FTPDataServer : public ITCPHandler
  {

  public:
    
    Socket*    sck;
    FTPServer* ftpServer;
    
    
    explicit FTPDataServer(void* userData) :
      ftpServer((FTPServer*)userData)
    {      
    }    
    
    
    void initConnection(Socket* socket)
    {
      sck = socket;
      exec();
    }
    
    
    void endConnection()
    { 
    }
    
        
    void receiveData()
    {
    }

    void listening()
    {
      exec();
    }
    
        
    
  public:
    
    void exec()
    { 

      if (!sck->connected())
        return;
      
      //debug << "FTPDataServer::listening() - " << "dataCmd=" << (uint16_t)ftpServer->dataCmd << ENDL;

      switch (ftpServer->dataCmd)
      {
        case FTPServer::DATA_CMD_MLSD:
          cmd_mlsd();
          break;

        case FTPServer::DATA_CMD_RETR:
          cmd_retr();
          break;

        case FTPServer::DATA_CMD_STOR:
          cmd_stor();
          break;          
          
        case FTPServer::DATA_CMD_LIST:
          cmd_list();
          break;
          
        case FTPServer::DATA_CMD_NONE:
        case FTPServer::DATA_CMD_RNFR:
          break;
      }
    }
    
    
  private:
    
    void complete226()
    {      
      sck->flush();
      delay(50);           
      sck->close();      

      ftpServer->sck->write_P(PSTR("226 completed\r\n"));            
      
      ftpServer->dataCmd = FTPServer::DATA_CMD_NONE;      
      ftpServer->dataCmdParams.clear();      
    }
    
    void send150()
    {
      ftpServer->sck->write_P(PSTR("150 OK\r\n"));
    }
    

  public:
    
    void cmd_mlsd()
    {
      send150();
      ftpServer->mlsd(sck, &ftpServer->dataCmdParams[0]);      
      complete226();
    }
    
    
    void cmd_list()
    {
      send150();
      ftpServer->list(sck, &ftpServer->dataCmdParams[0]);
      complete226();
    }
    
    
    void cmd_retr()
    {
      send150();
      ftpServer->fileRead(sck, &ftpServer->dataCmdParams[0]);
      complete226();
    }
    
    
    void cmd_stor()
    {
      send150();
      ftpServer->fileWrite(sck, &ftpServer->dataCmdParams[0]);
      complete226();      
    }
    
  };
  
  
  static ITCPHandler* FTPDataServerCreator(void* userData)
  {
    return new FTPDataServer(userData);
  }


} // end of fdv namespace


#endif  // FDV_FTPSERVER_H

