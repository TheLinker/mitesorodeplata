#include "consola.h"

int main ()
{
		//inicializo una estructura nipc para la comunicacion

	   /** ---------------------------------------------ME CONECTO AL PPD---------*/
	sleep(1);
	printf("Arranco la consola\n");

	int cliente, lengthCliente, resultado;
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

	printf("Se ha conectado con el PPD \n");

	while (1)
		atenderComando(cliente);


	//close ( cliente );  //hay que cerrarlo el socket en este momento????????
}
/*--------------------------------------------------------------------------------*/

//	while(1)
	//{
		//if(0 == atenderComando(/* estructura nipc ,  el tengo q pasarle el socket por el q se comunican */))
		//{
			//conectarPpd();
		//}
	//}
	//return 0;
//}


int atenderComando(int cliente)/*Se llama por cada comando. Devuelve cant de bytes enviados o recibidos*/
{
	char *funcion, *parametros, cantparametros[100], resp[1024];
	int j, cantparam = 0;
	char comando[TAM_COMANDO];
	unsigned int i = 0;
	//unsigned int cantEnv = 1;

	memset(comando, '\0', sizeof(comando));

	printf("\nIngrese comando:\n");
	fgets(comando, TAM_COMANDO, stdin);
	comando[strlen(comando) - 1] = 0;

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

			send(cliente,comando,strlen(comando),0); //LO AGREGUE

			recv(cliente,resp,strlen(resp),0); //LO CAMBIE

			funcInfo(resp);

		}else

		if(strcmp(funcion, "clean") == 0)
		{
			if(NULL == parametros) //Control de parametros
			{
				//errParam();
				//return cantEnv;
			}

			//if (0 == errParamClean(parametros)){
			sprintf(comando, "%s(%s)", funcion, parametros);
			//if(!(cantEnv = enviar(comando)))
				//return cantEnv;
			send(cliente,comando,strlen(comando),0); //LO CAMBIE
			//send(cliente,resp,strlen(resp),0); original

			recv(cliente,resp,sizeof(resp),0); //LO CAMBIE

			funcClean(resp);
				//}else
			//printf("El ingreso de parametros a sido invalido");

		}else

		if(strcmp(funcion, "trace") == 0)
		{

			memset(cantparametros, '\0', 100);
			strncpy(cantparametros, parametros, 100);

			if (NULL != cantparametros)
			{
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
						recv(cliente,resp,sizeof(resp),0);
						funcTrace(resp);
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
	printf("La posicion actual de la cabeza es: %s", resp);

	return;
}

void funcClean(char *resp)
{
	printf("%s", resp);
	return;
}

void funcTrace(char *resp)
{
	int32_t a, psect, ssect, pnextsect, snextsect, pposactual, sposactual, pposinit, sposinit, pposactual2;
	char trace[200000], aux[20];
	double tiempo;

	a = 0;

	memset(trace, '\0', 200000);

	psect = atoi(strtok(resp, ","));
	ssect = atoi(strtok(NULL, ","));
	pnextsect = atoi(strtok(NULL, ","));
	snextsect = atoi(strtok(NULL, ","));
	pposactual = atoi(strtok(NULL, ","));
	sposactual = atoi(strtok(NULL, ","));
	tiempo = atof(strtok(NULL, "\0"));
	pposinit = pposactual;
	sposinit = sposactual;

	if(pposactual != psect)
	{
		a = 1;
		if(pposactual > psect)
		{
			for( ;pposactual<=1023 ; pposactual++)				//TODO
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
			for( ;sposactual<=1023 ; sposactual++)				//TODO
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

	printf("Posición actual: %d:%d\n", pposinit, sposinit);
	printf("Sector solicitado: %d:%d\n", psect, ssect);
	printf("Sectores recorridos: %s\n", trace);
	printf("Tiempo consumido: %g ms\n", tiempo);
	printf("Próximo sector: %d:%d\n\n", pnextsect, snextsect);


	return;
}


//int errParamClean(/*parametros*/)
//{

	//return 0;
//}

//int errParamTrace(/*parametros*/)
//{

	//return 0;
//}
