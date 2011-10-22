#ifndef __NIPC_H
#define __NIPC_H

#define FALSE 0
#define TRUE !FALSE

#include <stdint.h>

enum {
	nipc_handshake = '0',
	nipc_req_read = '1',
	nipc_req_write = '2',
	nipc_error = '3',
};

typedef struct payload_t{
        uint32_t  sector;
	char   contenido[512];
    } __attribute__ ((packed)) payload_t;


typedef union nipc_packet {
    char buffer[519];
    struct {
        char    type;
	uint16_t   len;
	payload_t  payload;
    } __attribute__ ((packed));
} nipc_packet;

typedef uint32_t nipc_socket;

nipc_socket 	create_socket(char *host, uint16_t port);
uint32_t 	send_socket(nipc_packet *packet, nipc_socket socket);
uint32_t   	recv_socket(nipc_packet *packet, nipc_socket socket);
void 		nipc_listen(nipc_socket sock);
void		nipc_close(nipc_socket sock);

#endif //__NIPC_H

