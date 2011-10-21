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

void nnCscan(nipc_packet, int);

void insertCscan(cola_t * newptr, cola_t* headprt, cola_t* saltoptr, int posCab);

void insertOrd (cola_t * colaptr, cola_t * newptr);

void insertFifo(nipc_packet);

void msjtocol(nipc_packet msj, cola_t * newptr);

void initCol(cola_t * headprt, cola_t * tailprt);

cola_t * initPtr();

cola_t * initSaltoPrt();

#endif /* PLANDISCO_H_ */
