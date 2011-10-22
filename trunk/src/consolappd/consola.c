#include "consola.h"

int main ()
{
		//inicializo una estructura nipc para la comunicacion

	   /** ---------------------------------------------ME CONECTO AL PPD---------*/

	int cliente, lengthCliente, resultado;
	struct sockaddr_un direccionServidor;
	struct sockaddr* punteroServidor;
	char comando[10];

	punteroServidor = ( struct sockaddr* ) &direccionServidor;
	lengthCliente = sizeof (direccionServidor );

	cliente = socket ( AF_UNIX, SOCK_STREAM, 0 );	/* creo socket UNIX*/

	direccionServidor.sun_family = AF_UNIX;

	/* tipo de dominio server */
	strcpy ( direccionServidor.sun_path, "fichero" );  /* nombre server */



	do
	{
		resultado = connect ( cliente, punteroServidor, lengthCliente );

		if ( resultado == -1 )
			sleep(1);   /* reintento */

	}   while ( resultado == -1 );

	while (1)
	{
		atenderComando(cliente);

		//printf("INGRESE COMANDO: ");

		//fgets(comando, TAM_COMANDO, stdin); //ingreso info, clean o trace

		//if(send(cliente, comando, sizeof(comando), 0) == -1)	//envio al proceso ppd el comando que contiene el dato(info,clean o trace)
		//{
		//printf("error enviando");
		//exit(0);
		//}
	}

	close ( cliente );  //hay que cerrarlo el socket en este momento????????
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
	char *funcion, *parametros, *resp;
	char comando[TAM_COMANDO];
	unsigned int i = 0;
	//unsigned int cantEnv = 1;


	strcpy(comando, "");

	if(0 == strcmp(comando, ""))
	{
		printf("\nIngrese comando:\n");
		fgets(comando, TAM_COMANDO, stdin);
		comando[strlen(comando) - 1] = 0;
	}

	funcion = strtok(comando, " ");
	parametros = strtok(NULL, "\0");

	if(NULL == funcion)
	{
		funcion = (char*)malloc(1);
		*funcion = 0;
	}

	for(i = 0; funcion[i] != '\0' ; i++)
		funcion[i] = tolower(funcion[i]);



	if(strcmp(funcion, "info") == 0)
	{

		sprintf(comando, "%s()", funcion);
		//if(!(bytesEnvRec = send(comando)))
			//return cantEnv;
		send(cliente,resp,strlen(resp),0);
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

		send(cliente,resp,strlen(resp),0);
		funcClean(resp);
			//}else
		//printf("El ingreso de parametros a sido invalido");

	}else

	if(strcmp(funcion, "trace") == 0)
	{
		if(NULL == parametros) //Control de parametros
		{
			//errParam();
			//return cantEnv;
		}

		//if(0 == errParamTrace(parametros)){
		sprintf(comando, "%s(%s)", funcion, parametros);
		//if(!(cantEnv = enviar(comando)))
			//return cantEnv;

		send(cliente,comando,strlen(comando),0);

		recv(cliente,resp,sizeof(resp),0);

		funcTrace(resp);
		//}else
		//printf("El ingreso de parametros a sido invalido");
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
	printf("%s", resp);
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
