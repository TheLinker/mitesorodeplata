#include "ppd.h"
#include "../common/nipc.h"
#include "../common/utils.h"


config_t vecConfig;
char * bufferConsola;
int posCabAct, cliente, dirArch;
nipc_socket ppd_socket, sock_new;
cola_t *headprt = NULL, *saltoptr = NULL;
size_t len = 100;
sem_t semEnc;

int main()
{
	cantSect = 1000000000;
	pthread_t thConsola;
	char* mensaje = NULL;
	int  thidConsola;
	sem_init(&semEnc,1,1);

	vecConfig = getconfig("config.txt");
	dirArch = abrirArchivoV("/home/lucas/workspace2/bin/fat32.disk");
	posCabAct = vecConfig.posactual;

	printf("%s\n", vecConfig.modoinit);

	if(!(strncmp(vecConfig.modoinit, "CONNECT",7)))
	{
		printf("Conexion con praid\n");
		conectarConPraid(vecConfig);
	}
	else
		if(!(strncmp(vecConfig.modoinit, "LISTEN",6)))
		{
			printf("Conexion con Pfs\n");
			//printf("%s\n", vecConfig.ipppd);
			//printf("%d\n",vecConfig.puertoppd);
			conectarConPFS(vecConfig);
		}
		else
			printf("Error de modo de inicialización\n");

	thidConsola = pthread_create( &thConsola, NULL, (void *) escucharConsola, (void*) mensaje);
	//thidEscucharPedidos = pthread_create( &thEscucharPedidos, NULL, (void *) escucharPedidos, (void*) mensaje);
	//thidAtenderpedidos = pthread_create( &thAtenderpedidos, NULL, (void *) atenderPedido, (void*) mensaje);

	return 1;
}

void escucharPedidos(void)
{
	nipc_packet msj; /*PRUEBA*/

	nipc_packet buffer2;
	buffer2.type = 0;
	buffer2.len = 0;
	buffer2.payload.sector = -3;
	strcpy(buffer2.payload.contenido, "lllllllll");
	send_socket(&buffer2 ,sock_new);
	//printf("%s,%d \n", buffer2.payload.contenido, sock_new);
	//printf("------------------------------\n");

	//if(0 == strcmp(vecConfig.algplan, "cscan"))
	//{
		while(1)
		{
			//sleep(1);
			recv_socket(&msj, sock_new);
			printf("%d, %d, %d ENTRA AL INSERT \n", msj.type, msj.len, msj.payload.sector);
			sem_wait(&semEnc);
			insertCscan(msj, &headprt, &saltoptr, vecConfig.posactual);
			sem_post(&semEnc);
		}
	//}else
		//HACER UN WHILE Q ESCUCHE PEDIDOS
		//insertFifo(msj, headprt);

return;
}

void atenderPedido(void)
{

	ped_t * ped;

	while(1)
	{

		sem_wait(&semEnc);
		ped = desencolar(&headprt, &saltoptr);
		sem_post(&semEnc);

		if (ped == NULL)
			continue;
		printf("SECTOR PEDIDO %d \n", ped->sect);

		switch(ped->oper)
		{

			case nipc_req_read:
				leerSect(ped->sect);
				break;
			case nipc_req_write:
				escribirSect(ped->sect, ped->buffer);
				break;

			default:
				printf("Error comando PPD %d \n", ped->oper);
				break;

		}

		free(ped);

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

	if( (0 <= sect) && (cantSect >= sect))
	{
		int env;
		printf("----------------Entro a Leer------------------- \n");
		dirMap = paginaMap(sect, dirArch);
		res = div(sect, 8);
		dirSect = dirMap + ((res.rem  *512));  //NO SE SI VA O NO EL -1    TODO
		memcpy(&buffer, dirSect, TAM_SECT);
		if(0 != munmap(dirMap,TAM_PAG))
			printf("Fallo la eliminacion del mapeo\n");

		resp.type = 1;
		resp.payload.sector = sect;
		memcpy((char *) resp.payload.contenido, &buffer, TAM_SECT);
		resp.len = sizeof(resp.payload);

		printf("ACA SE ENVIA ALGOOOOOOOOOOOO\n");
		env = send_socket(&resp, sock_new);    //mando el buffer por el protocolo al raid

		printf("%d\n ", env);
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


		if((0 <= sect) && (cantSect >= sect))
		{
			printf("----------------Entro a Escribir------------------- \n");
			dirMap = paginaMap(sect, dirArch);
			res = div(sect, 8);
			dirSect = dirMap + ((res.rem *512 ));  //NO SE SI VA O NO EL -1    TODO
			memcpy(dirSect, buffer, TAM_SECT);
			msync(dirMap, TAM_PAG, MS_ASYNC);
			if(0 != munmap(dirMap,TAM_PAG))
				printf("Fallo la eliminacion del mapeo\n");

			resp.type = 2;
			resp.payload.sector = sect;
			memset(resp.payload.contenido,'\0', TAM_SECT);
			resp.len = 4;

			send_socket(&resp, sock_new);
		}
		else
		{
			printf("El sector no es valido\n");
		}

}

int abrirArchivoV(char * pathArch)			//Se le pasa el pathArch del config. Se mapea en esta funcion lo cual devuelve la direccion en memoria
{
	if (0 > (dirArch = open(pathArch, O_RDWR)))
	{
		printf("%s\n",pathArch);
		printf("Error al abrir el archivo de mapeo %d \n ",errno);
	}
	else
	{
			printf("El archivo se abrio correctamente\n");
			return dirArch;
	}
return 0;
}

//---------------Funciones Aux PPD------------------//

void * paginaMap(int sect, int dirArch)
{
	div_t res;
	res = div(sect, 8);
	offset = (res.quot * TAM_PAG);
	dirMap = mmap(NULL,TAM_PAG, PROT_READ | PROT_WRITE, MAP_SHARED, dirArch , offset);
	printf("%d %X\n", errno, dirMap[0]);

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
	//encolar
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
