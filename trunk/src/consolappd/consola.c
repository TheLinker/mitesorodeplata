#include "consola.h"

//NO TRABAJAMOS CON NIPC

int main ()
{
	//inicializo una estructura nipc para la comunicacion
	//conectarseAlPdd
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
		//if(!(bytesEnvRec = enviayrecibemsj( estructura nipc , comando)))
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
		sprintf(comando, "%s(%s)", funcion, parametros);
		//if(!(cantEnv = enviayrecibemsj( estructura nipc , comando)))
			//return cantEnv;
		funcClean(/* estructura nipc */);

	}else

	if(strcmp(funcion, "trace") == 0)
	{
		if(NULL == parametros) //Control de parametros
		{
			//errParam();
			return cantEnv;
		}
		//payloadDescriptor = CLEAN;
		sprintf(comando, "%s(%s)", funcion, parametros);
		//if(!(cantEnv = enviayrecibemsj( estructura nipc , comando)))
			//return cantEnv;
		funcTrace(/* estructura nipc */);

	}

	return cantEnv;
}


void conectarPpd(void)
{

	return;
}

void funcInfo(void)
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
