#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "planDisco.h"


void nnCscan(nipc_packet msj, int posCab)
{
	cola_t *headprt = NULL, *saltoptr, * newptr;

	saltoptr = initSaltoPrt();

	newptr = initPtr();
	msjtocol(msj, newptr);
	insertCscan(newptr, headprt, saltoptr, posCab);

	return;
}

void insertCscan(cola_t * newptr, cola_t* headprt, cola_t* saltoptr, int posCab)
{
	div_t pisec;

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

	if((colaptr->ped.sect == 99999) /*prueba*/ && NULL == colaptr->sig)
		colaptr->sig = newptr;
	else
	{
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

cola_t * initSaltoPrt()
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
	    	saltoprt->ped.sect = 99999; /*Se Tiene que calcular el cual es el ultimo sector de todos*/
	    	saltoprt->sig = NULL;
	    }

	 return saltoprt;
}


//cola_t *creacola(cola_t *ult, nipc_packet msj)
//{
//    cola_t *p;
//    p=initPtr();
//    msjtocol(msj, p);
//    if(ult!=NULL)
    	//(*ult).proximo=p; // Si hay nodo anterior en prox pongo la direccion del nodo actual
//    return p;
//}

int colaVacia(cola_t * headptr)
{
    if(headptr==NULL)
    {
    	return 1;
    }else
    {
    	return 0;
    }
}






void insertFifo(nipc_packet msj)
{

	return;
}
