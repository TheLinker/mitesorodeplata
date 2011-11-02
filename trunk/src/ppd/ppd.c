#include "ppd.h"
#include "../common/nipc.h"
#include "../common/utils.h"


config_t vecConfig;
char * bufferConsola;
int posCabAct, cliente;
nipc_socket ppd_socket;
cola_t *headprt = NULL, *saltoptr = NULL;
size_t len = 100;
FILE * dirArch;

int main()
{
	pthread_t thConsola,thEscucharPedidos, thAtenderpedidos;
	char* mensaje = NULL;
	int  thidConsola, thidEscucharPedidos, thidAtenderpedidos;

	vecConfig = getconfig("config.txt");
	dirArch = abrirArchivoV(vecConfig.rutadisco);
	posCabAct = vecConfig.posactual;

	if(strcmp(vecConfig.modoinit, "CONNECT"))
		conectarConPraid();
	else
		if(strcmp(vecConfig.modoinit, "LISTEN"))
			conectarConPFS(vecConfig);
		else
			printf("Error de modo de inicialización");

	thidConsola = pthread_create( &thConsola, NULL, (void *) escucharConsola, (void*) mensaje);
	thidEscucharPedidos = pthread_create( &thEscucharPedidos, NULL, (void *) escucharPedidos, (void*) mensaje);
	thidAtenderpedidos = pthread_create( &thAtenderpedidos, NULL, (void *) atenderPedido, (void*) mensaje);

	return 1;
}

 void conectarConPraid()  //ver tipos de datos
{
	ppd_socket = create_socket(vecConfig.ippraid,vecConfig.puertopraid);  // ver tipos de datos
}


void escucharPedidos(void)
{
	nipc_packet msj; /*PRUEBA*/

	if(0 == strcmp(vecConfig.algplan, "cscan"))
	{
		while(1)
		{
			recv_socket(&msj, ppd_socket);
			insertCscan(msj, headprt, saltoptr, vecConfig.posactual);
		}
	}else
		//HACER UN WHILE Q ESCUCHE PEDIDOS
		//insertFifo(msj, headprt);

return;
}

void atenderPedido(void)
{
	ped_t ped;

	ped = desencolar(headprt, saltoptr);

	switch(ped.oper)
	{
		case nipc_req_read:
			leerSect(ped.sect);
			break;
		case nipc_req_write:
			escribirSect(ped.sect, ped.buffer);
			break;

		default:
			printf("Error comando PPD");
			break;

	}

}

void escucharConsola()
{
	//while que escuche consola

	int servidor, cliente, lengthServidor, lengthCliente;
	struct sockaddr_un direccionServidor;
	struct sockaddr_un direccionCliente;
	struct sockaddr* punteroServidor;
	struct sockaddr* punteroCliente;


	signal ( SIGCHLD, SIG_IGN );

	punteroServidor = ( struct sockaddr* ) &direccionServidor;
	lengthServidor = sizeof ( direccionServidor );
	punteroCliente = ( struct sockaddr* ) &direccionCliente;
	lengthCliente = sizeof ( direccionCliente );
	servidor = socket ( AF_UNIX, SOCK_STREAM, PROTOCOLO );

	direccionServidor.sun_family = AF_UNIX;    /* tipo de dominio */

	strcpy ( direccionServidor.sun_path, "fichero" );   /* nombre */
	unlink( "fichero" );
   
	if (bind ( servidor, punteroServidor, lengthServidor ) <0)   /* crea el fichero */
  {   
       printf("ERROR BIND");
       exit(1);
  }  

	puts ("\n Estoy a la espera \n");
   
	if (listen ( servidor, 1 ) <0 )
  {  
       printf("ERROR LISTEN");
       exit(1);
  }  

	do	//verifico que se quede esperando la conexion en caso de error

		cliente = accept ( servidor, punteroServidor, &lengthServidor );

	while (cliente == -1);
   
	puts ("\n Acepto la conexion \n");

	while(1)
	{

		if(recv(cliente,comando,sizeof(comando),0) == -1) // recibimos lo que nos envia el cliente
		{
			printf("error recibiendo");
			exit(0);
		}

		atenderConsola(comando);
	}

	close( cliente );

}

void atenderConsola(char comando[100])
{
	char * funcion, * parametros;

	funcion = strtok(comando,"(");
	parametros = strtok (NULL, ")");

	if(0 == strcmp(funcion,"info"))
		{
			funcInfo();
		}
		else
		{
			if(0 == strcmp(funcion,"clean"))
			{
				funcClean(parametros);
			}
			else
			{
			if(0 == strcmp(funcion,"trace"))
			{
				funcTrace(parametros);
			}
			else
			{
				printf("Ha ingresado un comando invalido desde la consola\n");
			}
			}

		}
}


//---------------Funciones PPD------------------//


void leerSect(int sect)
{
	div_t res;
	nipc_packet resp;

	if( (0 >= sect) && (cantSect <= sect))
	{
		dirMap = paginaMap(sect, dirArch);
		res = div(sect, 8);
		dirSect = dirMap + ((res.rem *8 *512 ) - 1);  //NO SE SI VA O NO EL -1    TODO
		memcpy(buffer, dirSect, TAM_SECT);
		if(0 != munmap(dirMap,TAM_PAG))
			printf("Fallo la eliminacion del mapeo\n");

		resp.type = 1;
		resp.payload.sector = sect;
		strcpy((char *) resp.payload.contenido, buffer);
		resp.len = sizeof(resp.payload);

		send_socket(&resp, ppd_socket);    //mando el buffer por el protocolo al raid
	}
	else
	{
		printf("El sector no es valido\n");
	}


}


void escribirSect(int sect, char buffer[512])
{
	div_t res;
	nipc_packet resp;

		if( (0 >= sect) && (cantSect <= sect))
		{
			dirMap = paginaMap(sect, dirArch);
			res = div(sect, 8);
			dirSect = dirMap + ((res.rem *8 *512 ) - 1);  //NO SE SI VA O NO EL -1    TODO
			memcpy(dirSect, buffer, TAM_SECT);
			if(0 != munmap(dirMap,TAM_PAG))
				printf("Fallo la eliminacion del mapeo\n");

			resp.type = 2;
			resp.payload.sector = sect;
			memset(resp.payload.contenido,'\0', TAM_SECT);
			resp.len = sizeof(resp.payload);

			send_socket(&resp, ppd_socket);
		}
		else
		{
			printf("El sector no es valido\n");
		}

}

FILE * abrirArchivoV(char * pathArch)			//Se le pasa el pathArch del config. Se mapea en esta funcion lo cual devuelve la direccion en memoria
{
	if (NULL == (dirArch = fopen(pathArch, "w+")))
	{
		printf("%s\n",pathArch);
		printf("Error al abrir el archivo de mapeo\n");
	}
	else
	{
			printf("El archivo se abrio correctamente\n");
			return dirArch;
	}
return 0;
}

//---------------Funciones Aux PPD------------------//

void * paginaMap(int sect, FILE * dirArch)
{
	div_t res;
	res = div(sect, 8);
	offset = (res.quot * 512);
	dirMap = mmap(NULL,TAM_PAG, PROT_WRITE, MAP_SHARED, (int) dirArch , offset);

	return dirMap;
}

//---------------Funciones Consola------------------//


void funcInfo()
{
	memset(bufferConsola, '\0', TAM_SECT);
	sprintf(bufferConsola, "%d", posCabAct);
	send(cliente,bufferConsola,strlen(bufferConsola),0);
	return;
}

void funcClean(char * parametros)
{
	char * strprimSec, * strultSec;
	int primSec, ultSec;

	strprimSec = strtok(parametros, ":");
	strultSec = strtok(NULL, "\0");
	primSec = atoi(strprimSec);
	ultSec = atoi(strultSec);
	memset(bufferConsola, '\0', TAM_SECT);
	while(primSec <= ultSec)
	{
		escribirSect(primSec, bufferConsola);
		primSec++;
	}

	strcpy(bufferConsola, "Se han limpiado correctamente los sectores");
	send(cliente,bufferConsola,strlen(bufferConsola),0);
	return;
}

void funcTrace(char * parametros)
{
	//int cantparam = 0;
	//simular el disco
	memset(bufferConsola, '\0', TAM_SECT);
	strcpy(bufferConsola, "Funcion en construccion");
	send(cliente,bufferConsola,strlen(bufferConsola),0);
	return;
}

//------------Prueba-----------------//

void msjprueba(nipc_packet * msj)
{
	msj->len = 1024;
	strcpy((char *) msj->payload.contenido, "1234,hola como estas vos...");
	msj->type = 2;

}
