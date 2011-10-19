#include "consola.h"

//NO TRABAJAMOS CON NIPC

int main ()
{
	/	//inicializo una estructura nipc para la comunicacion

	   /** ---------------------------------------------ME CONECTO AL PPD----------------------------------------------------

	    int clienteConsola, lengthClienteConsola, resultadoConexion;
	    struct sockaddr_un direccionServidorPpd;
	    struct sockaddr* punteroServidorPpd;

	    punteroServidorPpd = ( struct sockaddr* ) &direccionServidorPpd;
	    lengthClienteConsola = sizeof (direccionServidorPpd );

	    clienteConsola = socket ( AF_UNIX, SOCK_STREAM, 0 );    /* creo socket UNIX*/

	    direccionServidorPpd.sun_family = AF_UNIX;

	    /* tipo de dominio server */
	    //strcpy ( direccionServidorPpd.sun_path, "fichero" );  /* nombre server */ //ver tema fichero

	    printf("%s",direccionServidorPpd.sun_path);

	    do
	    {

	        resultadoConexion = connect ( clienteConsola, punteroServidorPpd, lengthClienteConsola );
	        if ( resultadoConexion == -1 )
	            sleep (1);   /* reintento */

	    }   while ( resultadoConexion == -1 );   //hasta que se conecte al Ppd

	    //leeFichero ( cliente );     /* lee el fichero */

	    close ( clienteConsola );

	    /* cierra el socket */
	        exit (0);     /* buen fin */

	    return 0;

	   /** ---------------------------------------------FIN CONEXION AL PPD----------------------------------------------------

		while(1)
		{
			if(0 == atenderComando(/* estructura nipc ,  el tengo q pasarle el socket por el q se comunican */))
			{
				conectarPpd();
			}
		}
		return 0;
}


int atenderComando(/* estructura nipc, socket comunicador */)/*Se llama por cada comando. Devuelve cant de bytes enviados o recibidos*/
{
	char *funcion, *parametros;
	char comando[TAM_COMANDO];
	unsigned int i = 0;
	unsigned int cantEnv = 1;


	strcpy(comando, "");
	//payloadDescriptor = 0;

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
		//payloadDescriptor = INFO;
		sprintf(comando, "%s()", funcion);
		//if(!(bytesEnvRec = enviar( estructura nipc , comando)))
			//return cantEnv;
		funcInfo(/* estructura nipc */);

	}else

	if(strcmp(funcion, "clean") == 0)
	{
		if(NULL == parametros) //Control de parametros
		{
			//errParam();
			return cantEnv;
		}
		//payloadDescriptor = CLEAN;
		//if (0 == errParamClean(parametros)){
		sprintf(comando, "%s(%s)", funcion, parametros);
		//if(!(cantEnv = enviar( estructura nipc , comando)))
			//return cantEnv;
		funcClean(/* estructura nipc */);
		//}else
		//printf("El ingreso de parametros a sido invalido");

	}else

	if(strcmp(funcion, "trace") == 0)
	{
		if(NULL == parametros) //Control de parametros
		{
			//errParam();
			return cantEnv;
		}
		//payloadDescriptor = TRACE;
		//if(0 == errParamTrace(parametros)){
		sprintf(comando, "%s(%s)", funcion, parametros);
		//if(!(cantEnv = enviar( estructura nipc , comando)))
			//return cantEnv;
		funcTrace(/* estructura nipc */);
		//}else
		//printf("El ingreso de parametros a sido invalido");
	}

	return cantEnv;
}


void conectarPpd(void)
{

	return;
}

void funcInfo()
{

	return;
}

void funcClean(void)
{

	return;
}

void funcTrace(void)
{

	return;
}

int errParamClean(/*parametros*/)
{

	return 0;
}

int errParamTrace(/*parametros*/)
{

	return 0;
}
