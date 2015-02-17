/*
# Created by Fabrizio Di Vittorio (fdivitto2013@gmail.com)
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
			virtual void sendChunk_P(PGM_P chunk, uint16_t chunkLength) = 0;
			virtual bool closeConnection(uint8_t ID) = 0;

			void sendString(char const* string)
			{
				sendChunk(string, strlen(string));
			}
			
			void sendString_P(PGM_P string)
			{
				sendChunk_P(string, strlen_P(string));
			}
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

	class StringItem
	{
		public:
		
			enum MemoryType { RAM, Flash };
		
		public:		
		
			StringItem() :
				m_next(NULL), m_data(NULL), m_dataLength(0), m_memoryType(RAM)
			{				
			}
		
			// stringLength = 0 -> autocalculate
			StringItem(StringItem const* previous, void const* string, uint16_t stringLength, MemoryType memoryType) :
				m_next(NULL), m_data(string), m_dataLength(stringLength), m_memoryType(memoryType)
			{
				if (m_dataLength == 0 && m_data)
					m_dataLength = (m_memoryType == RAM? strlen(getString()) : strlen_P(getString_P()));
				if (previous)
					previous->setNext(this);
			}
			
			// set next item
			void setNext(StringItem const* next) const
			{
				m_next = next;
			}
			
			// get next item
			StringItem const* getNext() const
			{
				return m_next;
			}
			
			void setData(void const* data, uint16_t length, MemoryType memoryType)
			{
				m_data       = data;
				m_dataLength = length;
				m_memoryType = memoryType;
			}
			
			void setData(StringItem const& item)
			{
				m_data       = item.m_data;
				m_dataLength = item.m_dataLength;
				m_memoryType = item.m_memoryType;
			}
			
			// get raw data
			void const* getData() const
			{
				return m_data;
			}
			
			// get raw data as RAM string
			char const* getString() const
			{
				return (char const*)m_data;
			}
			
			// get raw data as FLASH string
			PGM_P getString_P() const
			{
				return (PGM_P)m_data;
			}
			
			// get RAM or FLASH string length
			uint16_t getStringLength() const
			{
				return m_dataLength;
			}
			
			// get memory type
			MemoryType getMemoryType() const
			{
				return m_memoryType;
			}
		
			// number of linked objects (this included)
			uint16_t getLinkedCount() const
			{
				uint16_t cnt = 0;
				for (StringItem const* current = this; current; current = current->m_next)
					++cnt;
				return cnt;				
			}
			
			// get cumulative lengths from this up to list end
			uint16_t getLinkedLength() const
			{
				uint16_t len = 0;
				for (StringItem const* current = this; current; current = current->m_next)
					len += current->m_dataLength;
				return len;
			}
									
		private:
		
			StringItem const mutable* m_next;
			void const*               m_data;
			uint16_t                  m_dataLength;
			MemoryType                m_memoryType;
	};
	
	
	///////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////

	class SendStringItem : public StringItem
	{
		public:

			SendStringItem()
			{	
			}

			SendStringItem(StringItem const* previous, void const* string, uint16_t stringLength, MemoryType memory) :
				StringItem(previous, string, stringLength, memory)
			{				
			}

			// send from this up to end of links to ISender object
			void send(ISender* sender) const
			{
				
				for (StringItem const* current = this; current; current = current->getNext())
				{
					if (current->getData())	// has data?
					{
						if (current->getMemoryType() == RAM)
							sender->sendChunk(current->getString(), current->getStringLength());
						else
							sender->sendChunk_P(current->getString_P(), current->getStringLength());
					}
				}
			}
		
	};	


	///////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////
	
	static char const STR_HTTP1_1[] PROGMEM              = "HTTP/1.1 ";
	static char const STR_NEWLINE[] PROGMEM              = "\r\n";
	static char const STR_CONTENTLENGTH[] PROGMEM        = "Content-Length: ";
	static char const STR_CONTENTTYPE_TEXTHTML[] PROGMEM = "Content-Type: Text/html";
	static char const STR_CONNECTIONCLOSE[] PROGMEM      = "Connection: close";
	static char const STR_GET[] PROGMEM                  = "GET";
	static char const STR_HEAD[] PROGMEM                 = "HEAD";
	static char const STR_HTTP200[] PROGMEM              = "200 OK";
	static char const STR_HTTP404[] PROGMEM              = "404 Not Found";
	static char const STR_HTTP307[] PROGMEM              = "307 Temporary Redirect";
	static char const STR_LOCATION[] PROGMEM             = "Location: ";
	
	
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
				WebFramework*       framework;
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
				
				request.framework = this;
				request.ID        = ID;
				
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
				reply(request, STR_HTTP404, NULL, 0, NULL, 0, SendStringItem(NULL, PSTR("Not found"), 0, StringItem::Flash));
			}
			

			// headers and headers_P can be NULL
			// content and content_P can be NULL
			void reply(Request const* request, PGM_P status, char const* headers[], uint8_t headersCount, PGM_P headers_P[], uint8_t headersCount_P, SendStringItem const& content)
			{
				// prepare content length as string
				uint16_t contentLength = content.getLinkedLength();
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
				for (uint8_t i = 0; i != headersCount_P; ++i)
					dataLength += strlen_P(headers_P[i]) + 2;	// +2 -> \r\n
				// content
				dataLength += 2;	// +2 -> \r\n
				if (strcmp_P(request->method, STR_HEAD) != 0)	// if not HEAD send content
					dataLength += contentLength;
				
				// prepare to send actual data
				m_sender->prepareSend(request->ID, dataLength);				
				// status lines
				m_sender->sendString_P(STR_HTTP1_1);
				m_sender->sendString_P(status);
				m_sender->sendString_P(STR_NEWLINE);
				// Content-Length
				m_sender->sendString_P(STR_CONTENTLENGTH);
				m_sender->sendChunk(contentLength_str, strlen(contentLength_str));
				m_sender->sendString_P(STR_NEWLINE);
				// Connection: close
				m_sender->sendString_P(STR_CONNECTIONCLOSE);
				m_sender->sendString_P(STR_NEWLINE);
				// additional headers
				for (uint8_t i = 0; i != headersCount; ++i)
				{
					m_sender->sendChunk(headers[i], strlen(headers[i]));
					m_sender->sendString_P(STR_NEWLINE);
				}
				for (uint8_t i = 0; i != headersCount_P; ++i)
				{
					m_sender->sendString_P(headers_P[i]);
					m_sender->sendString_P(STR_NEWLINE);
				}
				// content
				m_sender->sendString_P(STR_NEWLINE);
				if (strcmp_P(request->method, STR_HEAD) != 0)	// if not HEAD send content
				{
					content.send(m_sender);
				}
				// finalize
				m_sender->closeConnection(request->ID);				
			}
			
		private:
		
			ISender*     m_sender;
			Route const* m_routes;
			uint8_t      m_routesCount;
		
	};
	
	
	///////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////	
	
	inline void WebResponseRedirect(WebFramework::Request const* request, PGM_P newServer)
	{
		char location[strlen_P(STR_LOCATION) + strlen_P(newServer) + strlen(request->requestedPage) + 1];
		strcpy_P(location, STR_LOCATION);
		strcat_P(location, newServer);
		strcat(location, request->requestedPage);
		char const* headers[] = {location};
		request->framework->reply(request, STR_HTTP307, headers, 1, NULL, 0, SendStringItem(NULL, NULL, 0, StringItem::RAM));		
	};


	///////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////
	
	inline void WebResponseHTML(WebFramework::Request const* request, SendStringItem const& HTMLContent)
	{
		PGM_P headers_P[] = {STR_CONTENTTYPE_TEXTHTML};
		request->framework->reply(request, STR_HTTP200, NULL, 0, headers_P, 1, HTMLContent);
	}


	///////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////

	inline void WebResponseTemplateHTML(WebFramework::Request const* request, PGM_P HTMLTemplate, StringItem const& parameters)
	{
		uint16_t paramsCount = parameters.getLinkedCount();
		uint16_t itemsCount = paramsCount * 2 + 1;
		SendStringItem HTMLContent[itemsCount];
		SendStringItem* curItem = &HTMLContent[0];
		
		StringItem const* curpar = &parameters;
		PGM_P startPos = HTMLTemplate;
		PGM_P curpos = HTMLTemplate;
		while (true)
		{
			char curchar = pgm_read_byte(curpos);
			if (curchar == '^')
			{
				if (curpos != startPos)
					(curItem++)->setData(startPos, curpos - startPos, StringItem::Flash);
					//(curItem++)->setData(PSTR("xxx"), 2, StringItem::Flash);
				startPos = curpos + 1;
				(curItem++)->setData(*curpar);
				curpar = curpar->getNext();
			}
			else if (curchar == 0)
			{				
				if (curpos != startPos)
					curItem->setData(startPos, curpos - startPos, StringItem::Flash);
				break;
			}
			++curpos;
		}
		
		// link items
		for (uint16_t i = 0; i != itemsCount - 1; ++i)
			HTMLContent[i].setNext(&HTMLContent[i + 1]);
		
		WebResponseHTML(request, HTMLContent[0]);
	}


}	// end of fdv namespace

#endif /* FDV_WEBFRAMEWORK_H_ */