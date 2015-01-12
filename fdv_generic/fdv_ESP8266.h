/*
# Created by Fabrizio Di Vittorio (fdivitto@gmail.com)
# Copyright (c) 2015 Fabrizio Di Vittorio.
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


/*
note:
  tested with version 0019000902
*/


#ifndef FDV_ESP8266_H_
#define FDV_ESP8266_H_

#include <inttypes.h>

#include <avr/io.h>
#include <avr/interrupt.h>

#include "fdv_webframework.h"


namespace fdv
{

	static char const STR_OK[]  PROGMEM = "OK";
	static char const STR_ATP[] PROGMEM = "AT+";
	static char const STR_RST[] PROGMEM = "RST";
	static char const STR_GMR[] PROGMEM = "GMR";
	static char const STR_CWMODE[] PROGMEM = "CWMODE";
	static char const STR_CWJAP[] PROGMEM = "CWJAP";
	static char const STR_ATE0[] PROGMEM = "ATE0";
	static char const STR_ATE1[] PROGMEM = "ATE1";
	static char const STR_CIFSR[] PROGMEM = "CIFSR";
	static char const STR_CIPMUX[] PROGMEM = "CIPMUX";
	static char const STR_CIPSERVER[] PROGMEM = "CIPSERVER";
	static char const STR_CIPSEND[] PROGMEM = "CIPSEND";
	static char const STR_CIPCLOSE[] PROGMEM = "CIPCLOSE";





	///////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////

	class ESP8266 : public ISender
	{

		public:			
		
			static uint8_t const FIRMWAREVERSIONLENGTH = 10;
			static uint8_t const MAXIPADDRESSLENGTH    = 15;
	
	
		public:
			
			explicit ESP8266(SerialBase* serial, SerialBase* debugSerial) :
			  m_serial(serial), m_debug(debugSerial), m_isAvailable(false), m_serverListener(NULL)
			{
				m_isAvailable = setEcho(false);
				
				/*char firmVer[FIRMWAREVERSIONLENGTH + 1];
				m_isAvailable = getFirmwareVersion(firmVer);*/
			}
			
			
			bool isAvailable()
			{
				return m_isAvailable;
			}
			
			
			bool setEcho(bool value)
			{
				if (value)
					m_serial->write_P(STR_ATE1);
				else
					m_serial->write_P(STR_ATE0);
				m_serial->writeNewLine();
				// wait reply
				return waitForOK();
			}
			
			
			bool reset()
			{
				bool ret = exec(STR_RST);
				m_serial->consume(2500);
				setEcho(false);
				return ret;
			}
								

			// return zero terminated firmware version
			bool getFirmwareVersion(char outValue[FIRMWAREVERSIONLENGTH + 1])
			{
				return exec(STR_GMR, NULL, 0, outValue, FIRMWAREVERSIONLENGTH);
			}
			
			
			enum WiFiMode
			{
				Client,
				AccessPoint,
				ClientAccessPoint
			};
			
			bool setWiFiMode(WiFiMode mode)
			{
				char const* p[1];
				switch (mode)
				{
					case Client:
						p[0] = "1";
						break;
					case AccessPoint:
						p[0] = "2";
						break;						
					case ClientAccessPoint:
						p[0] = "3";
						break;
				}
				return exec(STR_CWMODE, p, 1) && reset();
			}
			
			
			bool joinAccessPoint(char const* SSID, char const* password)
			{
				// add quotes to SSID
				uint8_t len_SSID = strlen(SSID);				
				char qSSID[len_SSID + 3];
				memcpy(qSSID + 1, SSID, len_SSID);
				qSSID[0] = '\"';
				qSSID[len_SSID + 1] = '\"';
				qSSID[len_SSID + 2] = 0;
				
				// add quotes to password
				uint8_t len_password = strlen(password);
				char qPassword[len_password + 3];
				memcpy(qPassword + 1, password, len_password);
				qPassword[0] = '\"';
				qPassword[len_password + 1] = '\"';
				qPassword[len_password + 2] = 0;				
				
				char const* p[2] = {qSSID, qPassword};
				return exec(STR_CWJAP, p, 2, NULL, 0, 6000);
			}
			
			
			bool getIPAddress(char* outValue)
			{
				return exec(STR_CIFSR, NULL, 0, outValue, MAXIPADDRESSLENGTH);
			}
			
			
			bool setMultipleConnections(bool value)
			{
				char const* p[1] = {value? "1" : "0"};
				return exec(STR_CIPMUX, p, 1);
			}
			
			
			bool listen(char const* port, IReceiver* listener)
			{
				m_serverListener = listener;
				char const* p[2] = {"1", port};
				return exec(STR_CIPSERVER, p, 2);
			}
			
			
			// process incoming data
			void yield()
			{
				while (m_serial->available() > 0)
				{
					if (m_serial->peek() == '+')
					{
						if (m_serial->available() > 7)
						{
							uint8_t cmd[5];
							m_serial->read(cmd, 5);
							if (memcmp_P(cmd, PSTR("+IPD,"), 5) == 0)
							{
								// get ID
								uint8_t ID = m_serial->read() - '0';
								// get length
								m_serial->read();	// bypass ','
								uint16_t len = m_serial->readUInt32();
								m_serial->read();	// bypass ':'
								// get data
								uint8_t data[len];
								for (uint16_t i = 0; i != len; ++i)
								{
									if (!m_serial->waitForData())
										return;	// timeout!
									data[i] = m_serial->read();									
								}
								// call listener object
								if (m_serverListener)
									m_serverListener->receive(ID, data, len, this);
							}
						}
					}
					else
						m_serial->read();	// discard this char
				}
			}
			
			
			// implements ISender
			void prepareSend(uint8_t ID, uint16_t dataLength)
			{
				char ID_str[2];
				Utility::fmtUInt32(ID, ID_str, 2);
				char dataLength_str[6];
				Utility::fmtUInt32(dataLength, dataLength_str, 6);
				char const* p[2] = {ID_str, dataLength_str};
				exec(STR_CIPSEND, p, 2, NULL, 0, 0);
				m_serial->waitFor(PSTR(">"));						
			  m_debug->write_P(PSTR("prepareSend "));
				m_debug->write(dataLength_str);
				m_debug->writeNewLine();
			}
			
			
			// implements ISender
			void sendChunk(char const* chunk, uint16_t chunkLength)
			{
				m_serial->write((uint8_t const*)chunk, chunkLength);				
			}
			
			
			// implements ISender
			void sendChunk_P(PGM_P chunk)
			{
				m_serial->write_P(chunk);
			}
			
			
			// implements ISender			
			bool closeConnection(uint8_t ID)
			{
				char ID_str[2];
				Utility::fmtUInt32(ID, ID_str, 2);
				char const* p[1] = {ID_str};
				return exec(STR_CIPCLOSE, p, 1);
			}
			
			
		private:
			
			
			bool exec(PGM_P cmd, char const* params[] = NULL, uint8_t paramsCount = 0, char* result = NULL, uint8_t maxResultLen = 0, uint32_t timeOut = 500)
			{
				// send AT
				m_serial->write_P(STR_ATP);
				// send command
				m_serial->write_P(cmd);
				if (paramsCount > 0)
				{
					m_serial->write('=');
					for (uint8_t i = 0; i != paramsCount; ++i)
					{
						m_serial->write(params[i]);
						if (i != paramsCount - 1)
							m_serial->write(',');
					}
				}
				// CR+LF
				m_serial->writeNewLine();
				// result
				if (result)
				{
					uint8_t i = 0;
					while (i != maxResultLen)
					{
						if (!m_serial->waitForData(timeOut))
							break;	// timeout
						uint8_t c = m_serial->read();
						if (c == 0x0D || c == 0x0A)
							break;
						result[i++] = c;
					}
					result[i] = 0;
				}
				// wait reply
				if (timeOut > 0)
					return waitForOK(timeOut);
				else
					return true;
			}
			
			
			bool waitForOK(uint32_t timeOut = 500)
			{
				return m_serial->waitFor(STR_OK, timeOut) and m_serial->waitForNewLine(timeOut);
			}
			
			
		private:
		
			SerialBase*      m_serial;
			SerialBase*      m_debug;
			bool             m_isAvailable;
			IReceiver* m_serverListener;
		
	};


}	// end of namespace fdv




#endif /* FDV_ESP8266_H_ */