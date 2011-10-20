#ifndef __PRAID1_H_
#define __PRAID1_H_

#include <stdint.h>
#include<pthread.h>

#define SIZEBUF 1024

struct mensajeCola{
	uint32_t type;
	uint32_t sector;
	char 	 contenido[512];
}__attribute__ ((packed));

typedef struct _pedido{
	char type_pedido;
	uint32_t sector;
	struct _pedido *sgte;
} __attribute__ ((packed)) pedido;

typedef struct _disco{
	char  id[20];
	uint32_t sock;
	pthread_t hilo;
	uint32_t cantidad_pedidos;
	pedido   *pedidos;
	struct _disco *sgte;
} __attribute__ ((packed)) disco;

typedef struct _pfs{
		uint32_t sock;
		struct _pfs *sgte;
} __attribute__ ((packed)) pfs;

#endif //__PRAID1_H_
