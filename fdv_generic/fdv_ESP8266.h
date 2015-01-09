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


	class ESP8266
	{

		public:			
		
			static uint8_t const FIRMWAREVERSIONLENGTH = 10;
	
	
		public:
			
			explicit ESP8266(SerialBase* serial) :
			  m_serial(serial), m_isAvailable(false)
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
				exec(STR_GMR, NULL, 0, 0);
				TimeOut timeOut(500);
				uint8_t i = 0;
				while (i != FIRMWAREVERSIONLENGTH && !timeOut)
				{
					if (m_serial->available() > 0)
					{
						uint8_t c = m_serial->read();
						if (c == 0x0D || c == 0x0A)
							break;
						outValue[i++] = c;
					}
				}
				outValue[i] = 0;
				return waitForOK();
			}
			
			
			enum WiFiMode
			{
				Client,
				AccessPoint
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
				return exec(STR_CWJAP, p, 2, 6000);
			}
			
			
		private:
			
			
			bool exec(PGM_P cmd, char const* params[] = NULL, uint8_t paramsCount = 0, uint32_t timeOut = 500)
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
		
			SerialBase* m_serial;
			bool        m_isAvailable;
		
	};


}	// end of namespace fdv




#endif /* FDV_ESP8266_H_ */