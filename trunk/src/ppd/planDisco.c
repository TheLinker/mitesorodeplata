#include "planDisco.h"
#include "ppd.h"

extern config_t vecConfig;
//extern sem_t semEnc;
extern int32_t sectxpis;

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

void insertNStepScan(nipc_packet msj, int cantPedidos, cola_t** largaptr, cola_t** acotadaptr, int posCab, nipc_socket socket)
{
	cola_t *newptr = 0;

	newptr = initPtr();
	msjtocol(msj, newptr, socket);

	if (cantPedidos < 10)
	{
		insertOrdNStep(acotadaptr, newptr);
		cantPedidos ++;
	}
	else
		insertAlFinalNStep(largaptr, newptr);

	return;
}


void insertAlFinalNStep(cola_t **largaptr, cola_t *newptr)
{
	cola_t ** aux;

	//si la lista largaptr no tiene elementos, tomo newptr como su primer elemento

	if(&largaptr == NULL)
	{
		largaptr = &newptr;
		aux = &newptr;
	}
	else
	{
		(*aux) -> sig =  (cola_t *) newptr;
		aux = &newptr;
	}

	return;

}

void insertOrdNStep (cola_t **acotadaptr, cola_t *newptr)
{

	cola_t * aux = 0;

	//printf("%d, %d, ENCOLA \n", newptr->ped.oper, newptr->ped.sect);

	if (acotadaptr == NULL)
	{
		acotadaptr = &newptr;
		//printf("acotadaPtr: %s %s", *newptr, *acotadaptr);
	}
	else if (newptr -> ped.sect <= (*acotadaptr) -> ped.sect)
	{
		newptr -> sig = (struct cola_t *) *acotadaptr ;
		acotadaptr = &newptr;
	}
	else if (newptr -> ped.sect > (*acotadaptr) -> ped.sect)
	{
		aux = *acotadaptr;
		while ( (aux -> sig != NULL ) && (aux -> ped. nextsect < newptr -> ped .sect) )
		{
			aux = (cola_t *) aux -> sig;
		}
		newptr -> sig = aux -> sig;
		aux -> sig = (struct cola_t *) newptr;
	}

	return;
}


ped_t * desencolarNStepScan(cola_t ** acotadaptr, cola_t ** largaptr, int32_t cantPedidos)
{
	ped_t * pedidoSalida = NULL;
	cola_t * aux;
	int i;

	if (cantPedidos == 0)
	{
		acotadaptr = largaptr;
		//adelanto largaptr hasta la posicion 10, ahi hago null el siguiente de la posicion 10 y pongo largaptr en la posicion 11
		for (i = 0 ; i < 10 ; i++)
		{
			largaptr = (struct cola_t *)(*largaptr)->sig;
		}
		aux = *largaptr;
		largaptr = (struct cola_t *)(*largaptr)->sig;
		(*aux).sig = NULL;

		cantPedidos = 10;
		ordenarLista(acotadaptr);
	}
	else
	{
		if(NULL != *acotadaptr)
		{
			pedidoSalida = (void*)acotadaptr;
			*acotadaptr = (*acotadaptr)->sig;
		}
	}

	return pedidoSalida;
}

void ordenarLista(cola_t** acotadaptr)
{
	
	return;
}


//////////////////////////
///   Otras funciones  ///
//////////////////////////


void insertOrd (cola_t ** colaptr, cola_t * newptr)
{
	/*
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

	return;*/


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
		if(NULL != ordptr->sig)
		{	
			newptr->sig = ordptr->sig;
			ordptr->sig = newptr;
			ordptr->ped.nextsect = newptr->ped.sect;
			ordptr = (cola_t*) ordptr->sig;
			//newptr->ped.nextsect = ordptr->ped.sect;
			newptr->ped.nextsect = 12;
		}else
		{
			newptr->ped.nextsect = -1;
					newptr->sig = ordptr->sig;
			ordptr->sig = newptr;
			ordptr->ped.nextsect = newptr->ped.sect;
			
		}
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



double timemovdisco(int32_t sector, int32_t posCab)
{
	int pisrec = 0;
	int sectrec = 0;
	double tiempo = 0;

	if (pista(sector) < pista(posCab))
		pisrec =  vecConfig.pistas + pista(sector) - pista(posCab);
	else
		pisrec = pista(sector) - pista(posCab);

	if(sectpis(sector) < sectpis(posCab))
		sectrec = sectxpis + sectpis(sector) - sectpis(posCab);
	else
		sectrec = sectpis(sector) - sectpis(posCab);

	tiempo = sectrec * timesect() + pisrec * vecConfig.tiempoentrepistas;

	return tiempo;
}

void moverCab(int32_t sect)
{
	//sem_wait(&semEnc);
	vecConfig.posactual = sect;
	//sem_post(&semEnc);
}

void obtenerrecorrido(int32_t sect, char * trace, int32_t posCab)
{
	div_t s,p;
	char aux[20];
	int32_t a, psect, ssect, pposactual, sposactual, pposactual2, cant, i=0;

	memset(trace, '\0', 2000);
	s = div(sect, sectxpis);
	p = div(posCab, sectxpis);

	psect= s.quot;
	ssect= s.rem;
	pposactual= p.quot;
	sposactual= p.rem;
	pposactual2= p.quot;
	
	cant = obtenerRecCant(psect, ssect, pposactual, sposactual);

	if(pposactual != psect)
	{
		a = 1;
		if(pposactual > psect)
		{
			for( ;pposactual<vecConfig.pistas ; pposactual++)				
			{
				memset(aux, '\0', 20);
				if(cant>20)
				{
					if(i == 9 || i == cant)
						sprintf(aux, "%d:%d ", pposactual,sposactual);
					else	
						sprintf(aux, "%d:%d, ", pposactual,sposactual);
					if((i<10) || (i>(cant-10)))
					{
						strcat(trace, aux);
						if (i==9)
							strcat(trace, " ... ");
					}
				i++;
				}else
				{
					memset(aux, '\0', 20);
					sprintf(aux, "%d:%d, ", pposactual,sposactual);
					strcat(trace, aux);
				}

			}
			pposactual = 0;
		}

		for( ; pposactual<=psect; pposactual++)
		{
			memset(aux, '\0', 20);
			if(cant>20)
			{
				if(i == 9 || i == cant)
					sprintf(aux, "%d:%d ", pposactual,sposactual);
				else	
					sprintf(aux, "%d:%d, ", pposactual,sposactual);
				if((i<10) || (i>(cant-10)))
				{
					strcat(trace, aux);
					if (i==9)
						strcat(trace, "... ");
				}
				i++;
			}else
			{
				memset(aux, '\0', 20);
				sprintf(aux, "%d:%d, ", pposactual,sposactual);
				strcat(trace, aux);
			}
			pposactual2 = pposactual;
		}


	}
	
	if(sposactual != ssect)
	{
		if (a == 1)
			sposactual++;
		if(sposactual > ssect)
		{
			for( ;sposactual<sectxpis ; sposactual++)				
			{
				memset(aux, '\0', 20);
				if(cant>20)
				{
					if(i == 9 || i == cant)
						sprintf(aux, "%d:%d ", pposactual2,sposactual);
					else
						sprintf(aux, "%d:%d, ", pposactual2,sposactual);
					if((i<10) || (i>(cant-10)))
					{
						strcat(trace, aux);
						if (i==9)
							strcat(trace, " ... ");
					}
				i++;
				}else
				{
					memset(aux, '\0', 20);
					sprintf(aux, "%d:%d, ", pposactual2,sposactual);
					strcat(trace, aux);
				}

			}
			sposactual = 0;
		}

		for( ; sposactual<=ssect; sposactual++)
		{
			memset(aux, '\0', 20);
			if(cant>20)
			{
				if(i == 9 || i == cant)
					sprintf(aux, "%d:%d ", pposactual2,sposactual);
				else	
					sprintf(aux, "%d:%d, ", pposactual2,sposactual);
				if((i<10) || (i>(cant-10)))
				{
					strcat(trace, aux);
					if (i==9)
						strcat(trace, " ... ");
				}
			i++;
			}else
			{
				memset(aux, '\0', 20);
				sprintf(aux, "%d:%d, ", pposactual2,sposactual);
				strcat(trace, aux);
			}
			}
	}
	return;
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
		if(i<20)
		{
			busqptr = (* saltoprt);
			while((busqptr != NULL) && (i<20))
			{
				cola[i] = busqptr->ped.sect;
				i++;
				busqptr = (cola_t*) busqptr->sig;
			}
		}
	}else if((*saltoprt) != NULL)
	{
			busqptr = (*saltoprt);
		while((busqptr != NULL) && (i<20) )
		{
			cola[i] = busqptr->ped.sect;
			i++;
			busqptr = (cola_t*) busqptr->sig;
		}
	}
}

int tamcola(cola_t ** headprt, cola_t ** saltoprt)
{
	cola_t * busqptr;
	int32_t i=0;

	busqptr = (* headprt);
	while(NULL != busqptr)
	{
		i++;
		busqptr = (cola_t *) busqptr->sig;
	}
	busqptr = (* saltoprt);
	while(NULL != busqptr)
	{
		i++;
		busqptr =(cola_t *) busqptr->sig;
	}

	return i;
}

int32_t obtenerRecCant(int32_t psect,int32_t ssect,int32_t pposactual,int32_t sposactual)
{	
	int32_t cantrec = 0;
	
	if (psect < pposactual)
		cantrec = cantrec +  vecConfig.pistas + psect - pposactual +1;
	else
		cantrec = cantrec + psect - pposactual;

	if(ssect <  sposactual)
		cantrec = cantrec + sectxpis + ssect - sposactual +1;
	else
		cantrec = cantrec + ssect - sposactual;

	return cantrec;
}
