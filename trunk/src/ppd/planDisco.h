#ifndef PLANDISCO_H_
#define PLANDISCO_H_

#include "../common/nipc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TAM_SECT 512

typedef struct
{
	unsigned int	sect;
	int		 		oper;
	nipc_socket		socket;
	int32_t			nextsect;
	char			buffer[TAM_SECT];
} ped_t;

typedef struct
{
	ped_t				ped;
	struct cola_t *		sig;
} cola_t;

void insertCscan(nipc_packet, cola_t**, cola_t**, int, nipc_socket);

void insertOrd (cola_t ** colaptr, cola_t * newptr);

void insertFifo(nipc_packet, cola_t *);

ped_t * desencolar(cola_t **, cola_t ** );

void msjtocol(nipc_packet msj, cola_t * newptr, nipc_socket socket);

cola_t * initPtr();

cola_t * initSaltoPrt();

int pista(int);

double timemovdisco(int);

double timesect (void);

int sectpis(int);

void moverCab(int sect);

#endif /* PLANDISCO_H_ */
