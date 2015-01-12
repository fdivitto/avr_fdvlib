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



#ifndef FDV_WEBFRAMEWORK_H_
#define FDV_WEBFRAMEWORK_H_


#include "fdv_utility.h"


namespace fdv
{


	///////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////

	class ISender
	{
		public:
			virtual void prepareSend(uint8_t ID, uint16_t dataLength) = 0;
			virtual void sendChunk(char const* chunk, uint16_t chunkLength) = 0;
			virtual void sendChunk_P(PGM_P chunk) = 0;
			virtual bool closeConnection(uint8_t ID) = 0;
	};



	///////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////

	class IReceiver
	{
		public:
			virtual void receive(uint8_t ID, uint8_t const* data, uint16_t dataLength, ISender* sender) = 0;			
	};



	///////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////
	
	static char const STR_HTTP1_1[] PROGMEM = "HTTP/1.1 ";
	static char const STR_NEWLINE[] PROGMEM = "\r\n";
	static char const STR_CONTENTLENGTH[] PROGMEM = "Content-Length: ";
	
	
	class WebFramework : public IReceiver
	{
	
		public:
		
			void receive(uint8_t ID, uint8_t const* data, uint16_t dataLength, ISender* sender)
			{
				
			}
			
			
			void reply(uint8_t ID, PGM_P status, char const* headers[], uint8_t headersCount, char const* content, ISender* sender)
			{
				// prepare content length as string
				uint16_t contentLength = strlen(content);
				char contentLength_str[6];
				Utility::fmtUInt32(contentLength, contentLength_str, 6);

				//// calculate data length
				uint16_t dataLength = 0;
				// status line
				dataLength += strlen_P(STR_HTTP1_1) + strlen_P(status) + 2;	// +2 -> \r\n
				// Content-Length
				dataLength += strlen_P(STR_CONTENTLENGTH) + strlen(contentLength_str) + 2;	// +2 -> \r\n
				// additional headers
				for (uint8_t i = 0; i != headersCount; ++i)
					dataLength += strlen(headers[i]) + 2;	// +2 -> \r\n
				// content
				dataLength += 2 + contentLength;	// +2 -> \r\n
				
				// prepare to send actual data
				sender->prepareSend(ID, dataLength);				
				// status lines
				sender->sendChunk_P(STR_HTTP1_1);
				sender->sendChunk_P(status);
				sender->sendChunk_P(STR_NEWLINE);
				// Content-Length
				sender->sendChunk_P(STR_CONTENTLENGTH);
				sender->sendChunk(contentLength_str, strlen(contentLength_str));
				sender->sendChunk_P(STR_NEWLINE);
				// additional headers
				for (uint8_t i = 0; i != headersCount; ++i)
				{
					sender->sendChunk(headers[i], strlen(headers[i]));
					sender->sendChunk_P(STR_NEWLINE);
				}
				// content
				sender->sendChunk_P(STR_NEWLINE);
				sender->sendChunk(content, contentLength);
				// finalize
				delay_ms(10);
				sender->closeConnection(ID);				
			}
		
	};


}	// end of fdv namespace

#endif /* FDV_WEBFRAMEWORK_H_ */