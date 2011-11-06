#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "planDisco.h"


void insertCscan(nipc_packet msj, cola_t** headprt, cola_t** saltoptr, int posCab)
{
	cola_t *newptr = 0;
	div_t pisec;

	newptr = initPtr();
	msjtocol(msj, newptr);

	pisec = div(newptr->ped.sect, 100 /*CANTIDAD DE SECTORES POR PISTA (HAY Q PASARSELO)*/);
	if(pisec.quot >= posCab)
	{
		insertOrd(headprt, newptr);
	}else
	{
		insertOrd(saltoptr, newptr);
	}
	return;
}

void insertOrd (cola_t ** colaptr, cola_t * newptr)
{
	cola_t* ordptr = 0;

	printf("%d, %d, ENCOLA \n", newptr->ped.oper, newptr->ped.sect);

	if(NULL == (*colaptr) || newptr->ped.sect < (*colaptr)->ped.sect)
	{
		newptr->sig = (*colaptr);
		(*colaptr) = newptr;
	}else
	{
		ordptr = (*colaptr);
		while((NULL != ordptr->sig) && (newptr->ped.sect > ordptr->ped.sect))
			ordptr = ordptr->sig;
		newptr->sig = ordptr->sig;
		ordptr->sig = newptr;
	}

	return;
}

void msjtocol(nipc_packet msj, cola_t * newptr)
{
	newptr->ped.sect = msj.payload.sector;
	newptr->ped.oper = msj.type;
	memcpy(newptr->ped.buffer, (char *) msj.payload.contenido, TAM_SECT);

}


cola_t * initPtr()
{
	cola_t * newptr = 0;

	newptr = (cola_t *) malloc (sizeof(cola_t));
	   if(newptr==NULL)
	    {
	        printf("Memoria RAM Llena");
	        exit(0);
	    }else
	    {
	    	memset(newptr->ped.buffer, '\0', TAM_SECT);
	    	newptr->ped.oper = 0;
	    	newptr->ped.sect = 0;
	    	newptr->sig = NULL;
	    }

	 return newptr;
}

ped_t * desencolar(cola_t ** headprt, cola_t ** saltoprt)
{
	ped_t * pout = NULL;

	if(NULL != *headprt)
	{
		pout = *headprt;
		*headprt = (*headprt)->sig;
	}else
	{
		if(NULL != *saltoprt)
		{
			pout = *saltoprt;
			*saltoprt = (*saltoprt)->sig;
		}
	}

	//printf("%d, %d DESENCOLA", pout->oper, pout->sect);

	return pout;
}



void insertFifo(nipc_packet msj, cola_t * headptr)
{

	return;
}
