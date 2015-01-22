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
			virtual void receive(uint8_t ID, uint8_t* data, uint16_t dataLength) = 0;			
	};



	///////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////
	
	static char const STR_HTTP1_1[] PROGMEM         = "HTTP/1.1 ";
	static char const STR_NEWLINE[] PROGMEM         = "\r\n";
	static char const STR_CONTENTLENGTH[] PROGMEM   = "Content-Length: ";
	static char const STR_CONNECTIONCLOSE[] PROGMEM = "Connection: close";
	static char const STR_GET[] PROGMEM             = "GET";
	static char const STR_HEAD[] PROGMEM            = "HEAD";
	static char const STR_HTTP404[] PROGMEM         = "404 Not Found";
	static char const STR_HTTP307[] PROGMEM         = "307 Temporary Redirect";
	static char const STR_LOCATION[] PROGMEM        = "Location: ";
	
	
	class WebFramework : public IReceiver
	{
		
		public:
		
			struct URLParameter
			{
				char const* key;
				char const* value;
			};
			
			
			struct Request
			{
				uint8_t             ID;			        // connection ID
				char const*         method;	        // ex: GET, POST, etc...
				char const*         requestedPage;	// ex: "/", "/data"...
				char const*         query;					// ex: "par1=xxx&ar2=yyy"
				uint8_t             paramsCount;		// length of params (number of items)
				URLParameter const* params;         // parsed query as key->value
			};
			
			
			typedef void (WebFramework::*PageHandler)(Request const* request);
			
			
			struct Route
			{
				PGM_P       page;
				PageHandler pageHandler;
			};
		
	
		public:
		
			WebFramework(ISender* sender) :
			  m_sender(sender), m_routes(NULL), m_routesCount(0)
			{					
			}
			
			
			void setRoutes(Route const* routes, uint8_t routesCount)
			{
				m_routes      = routes;
				m_routesCount = routesCount;
			}
						
		
			void receive(uint8_t ID, uint8_t* data, uint16_t dataLength)
			{
				Request request;
				
				request.ID = ID;
				
				char* curc = (char*)data;
				char const* dataEnd = (char const*)data + dataLength - 1;
				
				// extract method (GET, POST, etc..)				
				request.method = curc;
				while (*curc != ' ' && curc < dataEnd)
					++curc;
				*curc++ = 0;	// ends method				
				
				// extract requested page and query
				request.requestedPage = curc;
				char* query = NULL;
				request.paramsCount = 0;
				while (*curc != ' ' && curc < dataEnd)
				{
					if (*curc == '?')
					{
						*curc = 0;	// ends requestedPage
						query = curc + 1; 
					}
					if (*curc == '=')
						++request.paramsCount;
					++curc;
				}
				*curc = 0; // ends requestedPage or query
				request.query = query;
				
				// extract parameters from query
				URLParameter params[request.paramsCount];	
				request.params = params;
				curc = query;
				for (uint8_t i = 0; i != request.paramsCount; ++i)
				{
					params[i].key = curc;
					while (*curc != '&' && *curc)
					{
						if (*curc == '=')
						{
							*curc++ = 0;
							params[i].value = curc;
						}
						++curc;
					}
					*curc++ = 0;
				}
				
				if (strcmp_P(request.method, STR_GET) == 0 || strcmp_P(request.method, STR_HEAD) == 0)
					get(&request);				
			}
			
			
			virtual void get(Request const* request)
			{
				for (uint8_t i = 0; i != m_routesCount; ++i)
				{
					if (strcmp_P("*", m_routes[i].page) == 0 || strcmp_P(request->requestedPage, m_routes[i].page) == 0)
					{
						(this->*m_routes[i].pageHandler)(request);
						return;
					}
				}
				// not found
				processNotFound(request);
			}
			
			
			virtual void processNotFound(Request const* request)
			{
				reply(request, STR_HTTP404, NULL, 0, "Not found");
			}
			

			void redirect(Request const* request, PGM_P newServer)
			{
				char location[strlen_P(STR_LOCATION) + strlen_P(newServer) + strlen(request->requestedPage) + 1];
				strcpy_P(location, STR_LOCATION);
				strcat_P(location, newServer);
				strcat(location, request->requestedPage);
				char const* headers[] = {location};
				reply(request, STR_HTTP307, headers, 1, "");
			}
			
			
			void reply(Request const* request, PGM_P status, char const* headers[], uint8_t headersCount, char const* content)
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
				// Connection: close
				dataLength += strlen_P(STR_CONNECTIONCLOSE) + 2;	// +2 -> \r\n
				// additional headers
				for (uint8_t i = 0; i != headersCount; ++i)
					dataLength += strlen(headers[i]) + 2;	// +2 -> \r\n
				// content
				dataLength += 2;	// +2 -> \r\n
				if (strcmp_P(request->method, STR_HEAD) != 0)	// if not HEAD send content
					dataLength += contentLength;
				
				// prepare to send actual data
				m_sender->prepareSend(request->ID, dataLength);				
				// status lines
				m_sender->sendChunk_P(STR_HTTP1_1);
				m_sender->sendChunk_P(status);
				m_sender->sendChunk_P(STR_NEWLINE);
				// Content-Length
				m_sender->sendChunk_P(STR_CONTENTLENGTH);
				m_sender->sendChunk(contentLength_str, strlen(contentLength_str));
				m_sender->sendChunk_P(STR_NEWLINE);
				// Connection: close
				m_sender->sendChunk_P(STR_CONNECTIONCLOSE);
				m_sender->sendChunk_P(STR_NEWLINE);
				// additional headers
				for (uint8_t i = 0; i != headersCount; ++i)
				{
					m_sender->sendChunk(headers[i], strlen(headers[i]));
					m_sender->sendChunk_P(STR_NEWLINE);
				}
				// content
				m_sender->sendChunk_P(STR_NEWLINE);
				if (strcmp_P(request->method, STR_HEAD) != 0)	// if not HEAD send content
					m_sender->sendChunk(content, contentLength);					
				// finalize
				m_sender->closeConnection(request->ID);				
			}
			
		private:
		
			ISender*     m_sender;
			Route const* m_routes;
			uint8_t      m_routesCount;
		
	};


}	// end of fdv namespace

#endif /* FDV_WEBFRAMEWORK_H_ */