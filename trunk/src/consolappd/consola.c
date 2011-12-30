#include "consola.h"

int32_t pistas,sectores,sectxpis, calcSect;

int main ()
{
		//inicializo una estructura nipc para la comunicacion

	   /** ---------------------------------------------ME CONECTO AL PPD---------*/
	printf("Arranco la consola\n");
	char infodisc[30];
	int32_t cliente, lengthCliente, resultado;
	struct sockaddr_un direccionServidor;
	struct sockaddr* punteroServidor;

	punteroServidor = ( struct sockaddr* ) &direccionServidor;
	lengthCliente = sizeof (direccionServidor );

	cliente = socket ( AF_UNIX, SOCK_STREAM, 0 );	/* creo socket UNIX*/

	direccionServidor.sun_family = AF_UNIX;

	/* tipo de dominio server */
	strcpy ( direccionServidor.sun_path, "/tmp/archsocketconsola" );  /* nombre server */



	do
	{
		resultado = connect ( cliente, punteroServidor, lengthCliente );

		if ( resultado == -1 )
			sleep(1);   /* reintento */

	}   while ( resultado == -1 );
	
	recv(cliente, infodisc, sizeof(infodisc),0);
	pistas = (atoi(strtok(infodisc,","))-1);
	sectores = (atoi(strtok(NULL,"\0")) -(pistas+1));
	sectxpis = sectores/(pistas+1);
	calcSect = ((sectores/(pistas+1)) +1);

	printf("Se ha conectado con el PPD \n");
	
	while (1)
		atenderComando(cliente);
}

int atenderComando(int cliente)/*Se llama por cada comando. Devuelve cant de bytes enviados o recibidos*/
{
	char *funcion, *parametros, cantparametros[100], resp[1024];
	int j, cantparam = 0;
	char comando[TAM_COMANDO];
	unsigned int i = 0;
	//unsigned int cantEnv = 1;

	memset(comando, '\0', sizeof(comando));
	memset(resp, '\0', 1024);

	printf("\nIngrese comando: ");
	fgets(comando, TAM_COMANDO, stdin);
	comando[strlen(comando) - 1] = 0;
	if(strcmp(comando,"")==0)
		send(cliente,comando,strlen(comando),0);
	
	funcion = strtok(comando, " ");
	parametros = strtok(NULL, "\0");

	for(i = 0; (funcion != NULL) && (funcion[i] != '\0'); i++)
		funcion[i] = tolower(funcion[i]);

	if(funcion != NULL) {
		if(strcmp(funcion, "info") == 0)
		{

			sprintf(comando, "%s()", funcion);
			//if(!(bytesEnvRec = send(comando)))
			//return cantEnv;
			//send(cliente,resp,strlen(resp),0); //ORIGINAL

			send(cliente,comando,strlen(comando),0); //LO AGREGU

			recv(cliente,resp,sizeof(resp),0); //LO CAMBIE

			funcInfo(resp);

		}else

		if(strcmp(funcion, "clean") == 0)
		{
			if(NULL != parametros) //Control de parametros
			{
				sprintf(comando, "%s(%s)", funcion, parametros);
				send(cliente,comando,strlen(comando),0); //LO CAMBIE
				recv(cliente,resp,sizeof(resp),0); //LO CAMBIE
				
				funcClean(resp);
			}
			else
				printf("El ingreso de parametros a sido invalido\n");

		}else

		if(strcmp(funcion, "trace") == 0)
		{
			if (parametros != NULL)
			{
				memset(cantparametros, '\0', 100);
				strncpy(cantparametros, parametros, 100);
				
				strtok(cantparametros , " ");
				do
				{
					cantparam ++;
				}
				while( NULL != strtok(NULL , " "));

				if(6 > cantparam)
				{
					sprintf(comando, "%s(%s)", funcion, parametros);
					send(cliente,comando,strlen(comando),0);
					
					for(j=0; j<cantparam; j++)
					{	
						usleep(500);
						recv(cliente,resp,sizeof(resp),0);
						if (strcmp(resp,"ERROR")!=0)
							funcTrace(resp);
						else
							printf("Los parametros son incorrectos\n");
						  
					}
				}else
				printf("La función trace espera como maximo 5 sectores\n");

			}else
			printf("La función trace espera una lista de sectores\n");
		} else
		printf("Has ingresado un comando no reconocido: %s\n", comando);
	}
	return 1;
}


void conectarPpd(void)
{

	return;
}

void funcInfo(char *resp)
{
	printf("La posicion actual de la cabeza es: %s\n", resp);
	return;
}

void funcClean(char *resp)
{
	printf("%s", resp);
	return;
}

void funcTrace(char *resp)
{
	int32_t a, i=0, psect, ssect, pnextsect, snextsect, pposactual, sposactual, pposinit, sposinit, pposactual2, cant;
	char trace[1024], * aux, * cola;
	double tiempo;

 	a = 0; 	
	cola = (char *) malloc(500);
	memset(cola, '\0', 500);
	aux = (char *) malloc(30);
	memset(trace, '\0', 1024);
	memset(aux, '\0', 30);
	
	cola = strtok(resp, ")"); 
	psect = atoi(strtok(NULL, ","));
	ssect = atoi(strtok(NULL, ","));
	pnextsect = atoi(strtok(NULL, ","));
	snextsect = atoi(strtok(NULL, ","));
	pposactual = atoi(strtok(NULL, ","));
	sposactual = atoi(strtok(NULL, ","));
	tiempo = atof(strtok(NULL, "\n "));
	pposinit = pposactual;
	sposinit = sposactual;
		

	cant = obtenerRecCant(psect, ssect, pposactual, sposactual);

	if(pposactual != psect)
	{
		a = 1;
		if(pposactual > psect)
		{
			for( ;pposactual<=pistas ; pposactual++)
			{
				memset(aux, '\0', 30);
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
			memset(aux, '\0', 30);
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
			for( ;sposactual<=sectxpis ; sposactual++)				
			{
				memset(aux, '\0', 30);
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
			memset(aux, '\0', 30);
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

	printf("Cola de Peticiones: %s\n", cola);
	printf("Posición actual: %d:%d\n", pposinit, sposinit);
	printf("Sector solicitado: %d:%d\n", psect, ssect);
	printf("Sectores recorridos: %s\n", trace);
	printf("Tiempo consumido: %g ms\n", tiempo);
	printf("Próximo sector: %d:%d\n\n", pnextsect, snextsect);
	
	free(aux);
	//free(cola);

	return;
}


int32_t obtenerRecCant(int32_t psect,int32_t ssect,int32_t pposactual,int32_t sposactual)
{	
	int32_t cantrec = 0;
	
	if (psect < pposactual)
		cantrec = cantrec +  pistas + psect - pposactual +1;
	else
		cantrec = cantrec + psect - pposactual;

	if(ssect <  sposactual)
		cantrec = cantrec + sectxpis + ssect - sposactual +1;
	else
		cantrec = cantrec + ssect - sposactual;

	return cantrec;
}
//int errParamClean(/*parametros*/)
//{

	//return 0;
//}

//int errParamTrace(/*parametros*/)
//{

	//return 0;
//}
