#include "planDisco.h"
#include "ppd.h"

extern config_t vecConfig;

extern int sectxpis;

void insertCscan(nipc_packet msj, cola_t** headprt, cola_t** saltoptr, int posCab, nipc_socket socket)
{
	cola_t *newptr = 0;
	div_t pisec;

	newptr = initPtr();
	msjtocol(msj, newptr, socket);

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

	//printf("%d, %d, ENCOLA \n", newptr->ped.oper, newptr->ped.sect);

	if(NULL == (*colaptr) || newptr->ped.sect < (*colaptr)->ped.sect)
	{
		if(NULL == (*colaptr))
			newptr->ped.nextsect = -1;
		else
			newptr->ped.nextsect = (*colaptr)->ped.sect;
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

void msjtocol(nipc_packet msj, cola_t * newptr, nipc_socket socket)
{
	newptr->ped.sect = msj.payload.sector;
	newptr->ped.oper = msj.type;
	memcpy(newptr->ped.buffer, (char *) msj.payload.contenido, TAM_SECT);
	newptr->ped.socket = socket;
	newptr->ped.nextsect = -1;

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
			*headprt = *saltoprt;  //preguntar si esto esta bien TODO
			*saltoprt = NULL;
			pout = *headprt;
			*headprt = (*headprt)->sig;
		}
	}

	//printf("%d, %d DESENCOLA", pout->oper, pout->sect);

	return pout;
}



void insertFifo(nipc_packet msj, cola_t * headptr)
{

	return;
}


///////////////////////////////////////
// Funciones simulacion tiempo disco //
///////////////////////////////////////

int pista(int sector)
{
	div_t pista;

	pista = div(sector, sectxpis);

	return pista.quot;
}

int sectpis(int sector)
{
	div_t sect;

	sect = div(sector, sectxpis);

	return sect.rem;
}

double timesect (void)
{
	return ((double)sectxpis / vecConfig.rpm);
}



double timemovdisco(int sector)
{
	int pisrec = 0;
	int sectrec = 0;
	double tiempo = 0;

	if (sector < vecConfig.posactual)
		pisrec =  vecConfig.pistas + pista(sector) - pista(vecConfig.posactual);
	else
		pisrec = pista(sector) - pista(vecConfig.posactual);

	if(sectpis(sector) < sectpis(vecConfig.posactual))
		sectrec = sectxpis + sectpis(sector) - sectpis(vecConfig.posactual);
	else
		sectrec = sectpis(sector) - sectpis(vecConfig.posactual);

	tiempo = sectrec * timesect() + pisrec * vecConfig.tiempoentrepistas;

	vecConfig.posactual = sector;

	return tiempo;
}

void moverCab(int sect)
{
	vecConfig.posactual = sect;
}
