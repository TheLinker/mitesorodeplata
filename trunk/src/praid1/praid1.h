#ifndef __PRAID1_H_
#define __PRAID1_H_

#include <stdint.h>
#include<pthread.h>
#include <semaphore.h>
#include"nipc.h"
#include "log.h"

#define SIZEBUF 1024

typedef struct _sectores_t{
    uint32_t             sector;
    struct _sectores_t  *sgte;
} __attribute__ ((packed)) sectores_t; // ya que nipc_socket = uint32_t lo vamos a usar para la lista de sock por liberar

struct mensaje_cola{
    uint32_t   type;
    uint32_t   sector;
    uint8_t    contenido[512];
}__attribute__ ((packed));

typedef struct _pedido{
    nipc_socket      sock;
    uint8_t          type;
    uint32_t         sector;
    uint8_t          contenido[512];
    struct _pedido  *sgte;
} __attribute__ ((packed)) pedido;

typedef struct _disco{
    uint8_t          id[20];
    nipc_socket      sock;
    pthread_t        hilo;
	uint8_t          pedido_sincro;
    int32_t          sector_sincro;
    sectores_t      *ya_sincro;
    uint32_t         cantidad_pedidos;
    pedido          *pedidos;
    struct _disco   *sgte;
} __attribute__ ((packed)) disco;

typedef struct _lista_pfs{
    nipc_socket             sock;
	sem_t                   semaforo;
    sectores_t             *escrituras; // lista de escrituras para que solo se le envio una confirmacion
    struct _lista_pfs      *sgte;
} __attribute__ ((packed)) lista_pfs;

typedef struct datos{
    nipc_socket     sock_raid;
    lista_pfs      *pfs_activos;
    disco          *discos;
    uint32_t        max_sector;
    sem_t           semaforo;
    sectores_t     *sock_x_liberar;
}datos;

typedef struct config_t{
    uint8_t     server_host[1024];
    uint16_t    server_port;
    uint16_t    console;
    char        log_path[1024];
    log_output  log_mode;
} __attribute__ ((packed)) config_t;


#endif //__PRAID1_H_
