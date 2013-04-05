#ifndef	_SOCKET_H_
#define	_SOCKET_H_

#include "w5100.h"

extern uint8_t socket(W5100Class* W5100, SOCKET s, uint8_t protocol, uint16_t port, uint8_t flag); // Opens a socket(TCP or UDP or IP_RAW mode)
extern void close(W5100Class* W5100, SOCKET s); // Close socket
extern uint8_t connect(W5100Class* W5100, SOCKET s, uint8_t * addr, uint16_t port); // Establish TCP connection (Active connection)
extern void disconnect(W5100Class* W5100, SOCKET s); // disconnect the connection
extern uint8_t listen(W5100Class* W5100, SOCKET s);	// Establish TCP connection (Passive connection)
extern uint16_t send(W5100Class* W5100, SOCKET s, const uint8_t * buf, uint16_t len); // Send data (TCP)
extern uint16_t recv(W5100Class* W5100, SOCKET s, uint8_t * buf, uint16_t len);	// Receive data (TCP)
extern uint16_t peek(W5100Class* W5100, SOCKET s, uint8_t *buf);
extern uint16_t sendto(W5100Class* W5100, SOCKET s, const uint8_t * buf, uint16_t len, uint8_t * addr, uint16_t port); // Send data (UDP/IP RAW)
extern uint16_t recvfrom(W5100Class* W5100, SOCKET s, uint8_t * buf, uint16_t len, uint8_t * addr, uint16_t *port); // Receive data (UDP/IP RAW)

extern uint16_t igmpsend(W5100Class* W5100, SOCKET s, const uint8_t * buf, uint16_t len);

#endif
/* _SOCKET_H_ */
