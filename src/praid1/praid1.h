#ifndef __PRAID1_H_
#define __PRAID1_H_

#include <stdint.h>
#include<pthread.h>
#include "nipc.h"

#define SIZEBUF 1024

struct mensaje_cola{
    uint32_t   type;
    uint32_t   sector;
    uint8_t    contenido[512];
}__attribute__ ((packed));

typedef struct _pedido{
    uint8_t          type;
    uint32_t         sector;
    uint8_t          contenido[512];
    struct _pedido  *sgte;
} __attribute__ ((packed)) pedido;

typedef struct _disco{
    uint8_t          id[20];
    nipc_socket      sock;
    pthread_t        hilo;
    uint32_t         cantidad_pedidos;
    pedido          *pedidos;
    struct _disco   *sgte;
} __attribute__ ((packed)) disco;

typedef struct _pfs{
    uint32_t       sock;
    struct _pfs   *sgte;
} __attribute__ ((packed)) pfs;

union semun 
{ 
	int                   val;
	struct semid_ds      *buf;
	unsigned short int   *array;
	struct seminfo       *__buf;
};

#endif //__PRAID1_H_
