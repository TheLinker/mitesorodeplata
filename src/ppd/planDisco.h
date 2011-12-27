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

////////////////////////////////////////////////////////////

void insertOrdNStep (cola_t ** colaptr, cola_t * newptr);

void insertNStepScan(nipc_packet msj, int32_t *cantPedidos, cola_t** headptr, cola_t** saltoptr, cola_t ** largaptr, int32_t posCab, nipc_socket socket);

void insertAlFinalNStep(cola_t** largaptr, cola_t * newptr);

ped_t * desencolarNStepScan(cola_t ** headptr, cola_t ** saltoptr, cola_t ** largaptr, int32_t *cantPedidos, int32_t posCab);


////////////////////////////////////////////////////////////

void insertCscan(nipc_packet, cola_t**, cola_t**, int32_t, nipc_socket);

void insertOrd (cola_t ** colaptr, cola_t * newptr);

ped_t * desencolar(cola_t **, cola_t ** );

void msjtocol(nipc_packet msj, cola_t * newptr, nipc_socket socket);

cola_t * initPtr();

cola_t * initSaltoPrt();

int32_t pista(int32_t);

double timemovdisco(int32_t sect, int32_t posCab);

double timesect (void);

int32_t sectpis(int32_t);

void moverCab(int32_t sect);

void obtenerrecorrido(int32_t sect, char * trace, int32_t posCab);

void obtenercola(cola_t ** headptr, cola_t ** saltoptr, int32_t * cola);

int32_t tamcola(cola_t ** headptr, cola_t ** saltoptr);

int32_t obtenerRecCant(int32_t psect,int32_t ssect,int32_t pposactual,int32_t sposactual);

#endif /* PLANDISCO_H_ */
