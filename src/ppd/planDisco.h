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
	int32_t		 	oper;
	nipc_socket		socket;
	int32_t			nextsect;
	char			buffer[TAM_SECT];
} ped_t;

typedef struct
{
	ped_t				ped;
	struct cola_t *		sig;
} cola_t;

void insertCscan(nipc_packet, cola_t**, cola_t**, int32_t, nipc_socket);

void insertOrd (cola_t ** colaptr, cola_t * newptr);

void insertNStepScan(nipc_packet msj, cola_t ** acotadaptr, cola_t** largaptr,int32_t posCab, nipc_socket socket);

void insertAlFinal(cola_t** largaptr, cola_t * newptr);

ped_t * desencolarNStepScan(cola_t ** acotadaptr);

ped_t * desencolar(cola_t **, cola_t ** );

void msjtocol(nipc_packet msj, cola_t * newptr, nipc_socket socket);

cola_t * initPtr();

cola_t * initSaltoPrt();

int pista(int32_t);

double timemovdisco(int32_t);

double timesect (void);

int sectpis(int32_t);

void moverCab(int32_t sect);

void obtenerrecorrido(int32_t sect, char * trace);

void obtenercola(cola_t ** headprt, cola_t ** saltoprt, int * cola);

int tamcola(cola_t ** headprt, cola_t ** saltoprt);

#endif /* PLANDISCO_H_ */
