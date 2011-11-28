#ifndef __NIPC_H
#define __NIPC_H

#define FALSE 0
#define TRUE !FALSE

#include <stdint.h>

#define MAX_CONECTIONS 20

enum {
    nipc_handshake  = 0,
    nipc_req_read   = 1,
    nipc_req_write  = 2,
    nipc_error      = 3,
    nipc_CHS        = 4,
    nipc_req_trace  = 5,
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

typedef int32_t nipc_socket;

nipc_socket     create_socket();
int8_t          nipc_bind_socket(nipc_socket socket, char *host, uint16_t port);
int8_t          nipc_connect_socket(nipc_socket socket, char *host, uint16_t port);
int32_t         send_socket(nipc_packet *packet, nipc_socket socket);
int32_t         recv_socket(nipc_packet *packet, nipc_socket socket);
void            nipc_listen(nipc_socket sock);
void            nipc_close(nipc_socket sock);

#endif //__NIPC_H

