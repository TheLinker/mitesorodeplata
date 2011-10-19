#ifndef __PRAID1_H_
#define __PRAID1_H_

#include <stdint.h>

#define SIZEBUF 1024

struct mensaje{
	uint32_t type_msg;
	uint32_t sector_msg;
}__attribute__ ((packed));

typedef struct _pedido{
	uint8_t type_pedido;
	uint32_t sector;
	struct _pedido *sgte;
} __attribute__ ((packed)) pedido;

typedef struct _disco{
	uint32_t id_disco;
	uint32_t cantidad_pedidos;
	pedido *pedidos;
	struct _disco *sgte;
} __attribute__ ((packed)) disco;


#endif //__PRAID1_H_
