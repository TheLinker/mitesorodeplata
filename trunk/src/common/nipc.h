#ifndef __NIPC_H
#define __NIPC_H

#define FALSE 0
#define TRUE !FALSE

#include <stdint.h>

enum {
    nipc_handshake = 0,
    nipc_req_packet,
};

typedef struct {
    uint8_t  type;
    uint16_t len;
    uint8_t  payload[1024];
} __attribute__ ((packed)) nipc_packet;

typedef uint32_t nipc_socket;

nipc_socket *nipc_init(uint8_t *host, uint16_t port);
void nipc_close(nipc_socket *socket);
void nipc_send_packet(nipc_packet *packet, nipc_socket *socket);
nipc_packet *nipc_recv_packet(nipc_socket *socket);

#endif //__NIPC_H

