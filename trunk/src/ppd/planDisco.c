#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "planDisco.h"


void insertCscan(nipc_packet msj, cola_t* headprt, cola_t* saltoptr, int posCab)
{
	cola_t *newptr;
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

void insertOrd (cola_t * colaptr, cola_t * newptr)
{
	cola_t* ordptr;

	if(NULL == colaptr || newptr->ped.sect < colaptr->ped.sect)
	{
		newptr->sig = colaptr;
		colaptr = newptr;
	}else
	{
		ordptr = colaptr;
		while((NULL != ordptr->sig) && (newptr->ped.sect > ordptr->ped.sect))
			ordptr = ordptr->sig;
		colaptr->sig = ordptr->sig;
		ordptr->sig = colaptr;
	}

	return;
}

void msjtocol(nipc_packet msj, cola_t * newptr)
{
	newptr->ped.sect = msj.payload.sector;
	newptr->ped.oper = msj.type;
	strcpy(newptr->ped.buffer, (char *) msj.payload.contenido);

}


cola_t * initPtr()
{
	cola_t * newptr;

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

/*cola_t * initSaltoPrt()
{
	cola_t * saltoprt;
	saltoprt = (cola_t *) malloc (sizeof(cola_t));
	   if(saltoprt==NULL)
	    {
	        printf("Memoria RAM Llena");
	        exit(0);
	    }else
	    {
	    	memset(saltoprt->ped.buffer, '\0', TAM_SECT);
	    	saltoprt->ped.oper = 0;
	    	saltoprt->ped.sect = 99999; //Se Tiene que calcular el cual es el ultimo sector de todos
	    	saltoprt->sig = NULL;
	    }

	 return saltoprt;
}*/


void insertFifo(nipc_packet msj, cola_t * headptr)
{

	return;
}
