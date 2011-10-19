#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "planDisco.h"


void insertCscan(nipc_packet msj, int posCab)
{
	ped_t newped;

	newped = msjtoped(msj);




	return;
}

void insertFifo(nipc_packet msj)
{

	return;
}

ped_t msjtoped(nipc_packet msj)
{
	ped_t  ped;
	char * strsect;

	//ped = (ped_t *) malloc (4+4+512);
	strsect = strtok(msj.payload, ",");
	ped.sect = atoi(strsect);
	memset(ped.buffer, '\0', 512);
	memcpy(ped.buffer, strtok(NULL, "\0"), 512);
	ped.oper = msj.type;

	return ped;
}
