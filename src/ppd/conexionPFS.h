#ifndef CONEXION_PFS_H_
#define CONEXION_PFS_H_

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/msg.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "getconfig.h"
#include "../common/nipc.h"
#include "../common/utils.h"


typedef struct _pedido{
    nipc_socket      sock;
    uint8_t          type;
    uint32_t         sector;
    uint8_t          contenido[512];
    struct _pedido  *sgte;
} __attribute__ ((packed)) pedido;


typedef struct _pfs{
    uint32_t       sock;
    struct _pfs   *sgte;
} __attribute__ ((packed)) pfs;

typedef struct datos{
    nipc_socket   sock_ppd;
    pfs          *lista_pfs;
 }datos;

void conectarConPFS(config_t);
void liberar_pfs_caido(pfs **pedidos_pfs, nipc_socket sock);

#endif /* CONEXION_PFS_H_ */
