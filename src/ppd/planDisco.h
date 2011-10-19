#ifndef PLANDISCO_H_
#define PLANDISCO_H_

#include "../common/nipc.h"

#define TAM_SECT 512

typedef struct
{
	unsigned int	sect;
	int		 		oper;
	char			buffer[TAM_SECT];
} ped_t;

typedef struct
{
	ped_t				ped;
	struct cola_t *		sig;
} cola_t;

void insertCscan(nipc_packet, int);

void insertFifo(nipc_packet);

ped_t msjtoped(nipc_packet);

#endif /* PLANDISCO_H_ */
