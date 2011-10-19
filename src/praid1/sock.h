#ifndef __SOCK_H
#define __SOCK_H

#define FALSE 0
#define TRUE !FALSE

#define HANDSHAKE 0
#define REQ_READ 1
#define REQ_WRITE 2

#include <stdint.h>


enum {
	nipc_handshake = '0',
	nipc_req_read = '1',
	nipc_req_write = '2',
};


typedef union nipc_packet {
    uint8_t buffer[515];
    struct {
        uint8_t  type;
	uint16_t len;
	uint8_t  payload[512];
    } __attribute__ ((packed));
} nipc_packet;

typedef uint32_t nipc_socket;

nipc_socket * 	createSocket(char *host, uint16_t port);
void 		closeSocket(nipc_socket *socket);
void 		sendSocket(nipc_packet *packet, nipc_socket *socket);
nipc_packet * 	recvSocket(nipc_socket *socket);


#endif //__SOCK_H

