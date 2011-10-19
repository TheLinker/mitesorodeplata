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

typedef struct payload_t{
        uint32_t  sector;
	uint8_t   contenido[512];
    } __attribute__ ((packed)) payload_t;


typedef union nipc_packet {
    uint8_t buffer[519];
    struct {
        uint8_t    type;
	uint16_t   len;
	payload_t  payload;
    } __attribute__ ((packed));
} nipc_packet;

typedef uint32_t nipc_socket;

nipc_socket 	createSocket(char *host, uint16_t port);
uint32_t 	sendSocket(nipc_packet *packet, nipc_socket socket);
uint32_t   	recvSocket(nipc_packet *packet, nipc_socket socket);

#endif //__SOCK_H

