#include "planDisco.h"
#include "ppd.h"

extern config_t vecConfig;

extern int sectxpis;

///////////////////////////
///      CSCAN          ///
///////////////////////////

void insertCscan(nipc_packet msj, cola_t** headprt, cola_t** saltoptr, int posCab, nipc_socket socket)
{
	cola_t *newptr = 0;
	div_t pisec;

	newptr = initPtr();
	msjtocol(msj, newptr, socket);

	pisec = div(newptr->ped.sect, sectxpis);
	if(pisec.quot >= pista(posCab))
	{
		insertOrd(headprt, newptr);
	}else
	{
		insertOrd(saltoptr, newptr);
	}
	return;
}

ped_t * desencolar(cola_t ** headprt, cola_t ** saltoprt)
{
	ped_t * pout = NULL;

	if(NULL != *headprt)
	{
		pout = *headprt;
		*headprt = (cola_t*) (*headprt)->sig;
	}else
	{
		if(NULL != *saltoprt)
		{
			*headprt = *saltoprt;
			*saltoprt = NULL;
			pout = *headprt;
			*headprt = (cola_t*) (*headprt)->sig;
		}
	}

	//printf("%d, %d DESENCOLA", pout->oper, pout->sect);

	return pout;
}

//////////////////////////
///     N-STEP-SCAN    ///
//////////////////////////

void insertNStepScan(nipc_packet msj, cola_t ** acotadaptr, cola_t** largaptr,int32_t posCab, nipc_socket socket)
{
	/*cola_t *newptr = 0;

	newptr = initPtr();
	msjtocol(msj, newptr, socket);

	if (cantPedidos < 10)
	{
		insertOrd(acotadaptr,newptr);
	}else
	{
		insertAlFinal(largaptr,newptr);
	}
	return;*/
}


void insertAlFinal(cola_t** largaptr, cola_t * newptr)
{
/*	cola_t * primerNodo;
	//si la lista largaptr no tiene elementos, tomo newptr como su primer elemento
	if(largaptr == NULL)
	{
		largaptr = newptr;
		primerNodo = newptr;
	}
	else
	{
		(*largaptr)->sig = newptr;
		(*largaptr)->ped.nextsect = newptr->ped.sect;
		largaptr = newptr;
	}
return;*/
}


ped_t * desencolarNStepScan(cola_t ** acotadaptr)
{
/*	ped_t * pedidoSalida = NULL;

	if(NULL != *acotadaptr)
	{
		pedidoSalida = *acotadaptr;
		*acotadaptr = (*acotadaptr)->sig;
	}

	cantPedidos--;

	return pedidoSalida;*/
}


//////////////////////////
///   Otras funciones  ///
//////////////////////////


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
			ordptr = (cola_t*) ordptr->sig;
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
	cola_t * newptr = NULL;

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



double timemovdisco(int32_t sector)
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

	return tiempo;
}

void moverCab(int32_t sect)
{
	vecConfig.posactual = sect;
}

void obtenerrecorrido(int32_t sect, char * trace)
{
	div_t s,p;
	char aux[20];
	int32_t a, psect, ssect, pposactual, sposactual, pposactual2;

	s = div(sect, sectxpis);
	p = div(vecConfig.posactual, sectxpis);

	psect= s.quot;
	ssect= s.rem;
	pposactual= p.quot;
	sposactual= p.rem;
	pposactual2= p.quot;

	if(pposactual != psect)
		{
			a = 1;
			if(pposactual > psect)
			{
				for( ;pposactual<=1023 ; pposactual++)
						{
							memset(aux, '\0', 20);
							sprintf(aux, "%d:%d, ", pposactual,sposactual);
							strcat(trace, aux);

						}
						pposactual = 0;
			}

			for( ; pposactual<=psect; pposactual++)
					{
						memset(aux, '\0', 20);
						sprintf(aux, "%d:%d, ", pposactual,sposactual);
						strcat(trace, aux);
						pposactual2 = pposactual;
					}


		}

		if(sposactual != ssect)
		{
			if (a == 1)
				sposactual++;
			if(sposactual > ssect)
			{
				for( ;sposactual<=1023 ; sposactual++)
					{
						memset(aux, '\0', 20);
						sprintf(aux, "%d:%d, ", pposactual2,sposactual);
						strcat(trace, aux);

					}
					sposactual = 0;
			}

			for( ; sposactual<=ssect; sposactual++)
						{
							memset(aux, '\0', 20);
							sprintf(aux, "%d:%d, ", pposactual2,sposactual);
							strcat(trace, aux);
						}
		}
}

void obtenercola(cola_t ** headprt, cola_t ** saltoprt, int * cola)
{
	cola_t * busqptr;
	int32_t i=0, j;

	for(j=0;j<20;j++)
		cola[j] = -1;

	if((*headprt) != NULL)
	{
		busqptr = (*headprt);
		while((busqptr != NULL) && (i<20) )
		{
			cola[i] = busqptr->ped.sect;
			i++;
			busqptr = (cola_t*) busqptr->sig;
		}
		if(i<30)
		{
			busqptr = (* saltoprt);
			while((busqptr != NULL) && (i<20))
			{
				cola[i] = busqptr->ped.sect;
				i++;
				busqptr = (cola_t*) busqptr->sig;
			}
		}
	}
}
