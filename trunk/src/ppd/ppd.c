#include "ppd.h"
#include "../common/nipc.h"
#include "../common/utils.h"


config_t vecConfig;
int posCabAct, dirArch, sectxpis;
cola_t *headprt = NULL, *saltoptr = NULL;
size_t len = 100;
sem_t semEnc;

int main()
{
	//int	pid;

	pthread_t thConsola, thAtenderpedidos;
	char* mensaje = NULL;
	int  thidConsola,thidAtenderpedidos;

	sem_init(&semEnc,1,1);

	//pid = getpid();
	vecConfig = getconfig("config.txt");
	sectxpis = (vecConfig.sectores/vecConfig.pistas);
	dirArch = abrirArchivoV(vecConfig.rutadisco);
	posCabAct = vecConfig.posactual;
	cantSect = vecConfig.sectores * vecConfig.pistas;

	printf("%s\n", vecConfig.modoinit);


	int pid = fork();

	if (pid == 0)
	{
		/*if(-1 == execle("../../consolappd/Debug/consolappd", "consolappd", NULL, NULL))
			printf("Error al ejecutar la consola \n");
			printf("NUMERO DE ERROR: %d \n", errno);*/
	}else
	{
		thidConsola = pthread_create( &thConsola, NULL, (void *) escucharConsola, (void*) mensaje);

		thidAtenderpedidos = pthread_create( &thAtenderpedidos, NULL, (void *) atenderPedido, NULL);

		if(!(strncmp(vecConfig.modoinit, "CONNECT",7)))
		{
			printf("Conexion con praid\n");
			conectarConPraid();
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
	}

		return 1;
}

 void conectarConPraid()  //ver tipos de datos
{
	nipc_packet buffer;
	nipc_socket *socket;
	nipc_socket sock_new;
	pthread_t thEscucharPedidos;
	int  thidEscucharPedidos;

	sock_new = create_socket();
	printf("socket: %d\n", sock_new);
	nipc_connect_socket(sock_new, vecConfig.ippraid, vecConfig.puertopraid);

	buffer.type = 0;
	buffer.len = 4 + 2;
	buffer.payload.sector = 0;
	strcpy(buffer.payload.contenido, "l");
	send_socket(&buffer ,sock_new);
	recv_socket(&buffer, sock_new);
	printf("%d\n", buffer.len);
	socket = malloc(sizeof(nipc_socket));
	*socket = sock_new;

	thidEscucharPedidos = pthread_create( &thEscucharPedidos, NULL,(void *)  escucharPedidos,  socket);


	pthread_join(thEscucharPedidos, NULL);


}


void escucharPedidos(nipc_socket *socket)
{
	nipc_packet msj;

	while(1)
	{
		//sleep(1);
		if (0 > recv_socket(&msj, *socket));
		{	if(0 == (msj.payload.sector % 5000))
				printf("%d, %d, %d ENTRA AL INSERT \n", msj.type, msj.len, msj.payload.sector);

			sem_wait(&semEnc);
			insertCscan(msj, &headprt, &saltoptr, vecConfig.posactual, *socket);
			sem_post(&semEnc);
		}
	}

return;
}

void atenderPedido()
{

	ped_t * ped;

	while(1)
	{

		sem_wait(&semEnc);
		ped = desencolar(&headprt, &saltoptr);
		sem_post(&semEnc);

		if (ped != NULL)
		{
			if(0 == (ped->sect % 5000))
				printf("SECTOR PEDIDO %d \n", ped->sect);

			switch(ped->oper)
			{

				case nipc_req_read:
					leerSect(ped->sect, ped->socket);
					break;
				case nipc_req_write:
					escribirSect(ped->sect, ped->buffer, ped->socket);
					break;
				case nipc_req_trace:
					traceSect(ped->sect, ped->nextsect,(int) ped->socket);
					break;

				default:
					printf("Error comando PPD %d \n", ped->oper);
					break;

			}


		free(ped);

		}

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

	unlink("/tmp/archsocketconsola");

	strcpy ( direccionServidor.sun_path, "/tmp/archsocketconsola" );   /* nombre */
	//unlink( "fichero" );
   
	if (bind ( servidor, punteroServidor, lengthServidor ) <0)   /* crea el fichero */
	{
       printf("ERROR BIND\n");
       exit(1);
	}

	puts ("\n Estoy a la espera \n");
   
	if (listen ( servidor, 1 ) <0 )
	{
       printf("ERROR LISTEN\n");
       exit(1);
	}

	do	//verifico que se quede esperando la conexion en caso de error

		cliente = accept ( servidor, NULL, NULL);

	while (cliente == -1);
   
	//puts ("\n Acepto la conexion \n");

	while(1)
	{

		if(recv(cliente,comando,sizeof(comando),0) == -1) // recibimos lo que nos envia el cliente
		{
			printf("error recibiendo\n");
			exit(0);
		}

		// SEND DATOS NECESARIOS TODO

		atenderConsola(comando, cliente);
	}

	close( cliente );

}

void atenderConsola(char comando[100], int cliente)
{
	char * funcion, * parametros;

	funcion = strtok(comando,"(");
	parametros = strtok (NULL, ")");

	if(0 == strcmp(funcion,"info"))
		{
			funcInfo(cliente);
		}
		else
		{
			if(0 == strcmp(funcion,"clean"))
			{
				funcClean(parametros, cliente);
			}
			else
			{
			if(0 == strcmp(funcion,"trace"))
			{
				funcTrace(parametros, cliente);
			}
			else
			{
				printf("Ha ingresado un comando invalido desde la consola\n");
			}
			}

		}
}


//---------------Funciones PPD------------------//


void leerSect(int sect, nipc_socket sock)
{
	div_t res;
	nipc_packet resp;

	if( (0 <= sect) && (cantSect >= sect))
	{
		if(0 == (sect % 5000))
			printf("----------------Entro a Leer------------------- \n");
		dirMap = paginaMap(sect, dirArch);
		res = div(sect, 8);
		dirSect = dirMap + ((res.rem  *512));
		memcpy(&buffer, dirSect, TAM_SECT);
		if(0 != munmap(dirMap,TAM_PAG))
			printf("Fallo la eliminacion del mapeo\n");

		resp.type = 1;
		resp.payload.sector = sect;
		memcpy((char *) resp.payload.contenido, &buffer, TAM_SECT);
		resp.len = sizeof(resp.payload);
		send_socket(&resp, sock);    //mando el buffer por el protocolo al raid
	}
	else
	{
		printf("El sector no es valido: %d\n",sect);
	}


}


void escribirSect(int sect, char buffer[512], nipc_socket sock)
{
	div_t res;
	nipc_packet resp;


		if((0 <= sect) && (cantSect >= sect))
		{
			if(0 == (sect % 5000))
				printf("----------------Entro a Escribir------------------- \n");
			dirMap = paginaMap(sect, dirArch);
			res = div(sect, 8);
			dirSect = dirMap + ((res.rem *512 ));
			memcpy(dirSect, buffer, TAM_SECT);
			msync(dirMap, TAM_PAG, MS_ASYNC);
			if(0 != munmap(dirMap,TAM_PAG))
				printf("Fallo la eliminacion del mapeo\n");

			resp.type = 2;
			resp.payload.sector = sect;
			memset(resp.payload.contenido,'\0', TAM_SECT);
			resp.len = 4;

			send_socket(&resp, sock);
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
	int ret;
	res = div(sect, 8);
	offset = (res.quot * TAM_PAG);
	dirMap = mmap(NULL,TAM_PAG, PROT_READ | PROT_WRITE, MAP_SHARED, dirArch , offset);
	if(ret = posix_madvise(dirMap, TAM_PAG, POSIX_MADV_SEQUENTIAL))
			printf("El madvise ha fallado. Número de error: %d \n", ret);
	//printf("%d %X\n", errno, dirMap[0]);

	return dirMap;
}

//---------------Funciones Consola------------------//


void funcInfo(int cliente)
{
	char * bufferConsola = NULL;
	bufferConsola = (char *) malloc(TAM_SECT);
	memset(bufferConsola, '\0', TAM_SECT);
	sprintf(bufferConsola, "%d", vecConfig.posactual);
	send(cliente,bufferConsola, strlen(bufferConsola) +1 ,0);
	return;
}

void funcClean(char * parametros, int cliente)
{
	char * strprimSec, * strultSec, * bufferConsola;
	int primSec, ultSec;
	nipc_packet pedido;

	bufferConsola = (char *) malloc(TAM_SECT);
	memset(bufferConsola, '\0', TAM_SECT);
	strprimSec = strtok(parametros, ":");
	strultSec = strtok(NULL, "\0");
	primSec = atoi(strprimSec);
	ultSec = atoi(strultSec);
	pedido.type = 2;
	pedido.len = (sizeof(nipc_packet));
	memset(pedido.payload.contenido, '\0', TAM_SECT);
	//encolar
	while(primSec <= ultSec)
	{
		//setea el sector
		pedido.payload.sector= primSec;
		//encolar
		sem_wait(&semEnc);
		insertCscan(pedido, &headprt, &saltoptr, vecConfig.posactual,0);
		sem_post(&semEnc);
		primSec++;
	}

	strcpy(bufferConsola, "Se han limpiado correctamente los sectores");
	send(cliente,bufferConsola,strlen(bufferConsola) + 1,0);
	free(bufferConsola);
	return;
}

void funcTrace(char * parametros, int cliente)
{
	int i, j, cantparam =0;
	char cantparametros[100],lsectores[5][25];
	memset(cantparametros, '\0', 100);
	strncpy(cantparametros, parametros, 100);

	for(i=0; i<5; i++)
	{
		int k;
		for (k = 0 ; k < 25 ; k++)
			lsectores[i][k] = '\0';
	}

	strtok(cantparametros , " ");
	do
	{
		cantparam ++;
	}
	while( NULL != strtok(NULL , " "));

	strcpy(lsectores[0], strtok(parametros, " "));

	for(j=1; j<cantparam; j++ )
	{
		strcpy(lsectores[0], strtok(NULL, " "));
	}

	nipc_packet pedido;

	pedido.type = nipc_req_trace;
	pedido.len = (sizeof(nipc_packet));
	memset(pedido.payload.contenido, '\0', TAM_SECT);

	//encolar
	for(i=0; i<cantparam; i++)
	{
		//setea el sector
		pedido.payload.sector= calcularSector(lsectores[i]);
		//encolar
		sem_wait(&semEnc);
		insertCscan(pedido, &headprt, &saltoptr, vecConfig.posactual,cliente);
		sem_post(&semEnc);

	}

	return;
}
int calcularSector(char structSect[25])
{
	int pist = atoi(strtok(structSect, ":"));
	int sect = atoi(strtok(NULL, "\0"));
	return ((pist * 1024) + sect); //TODO
}

void traceSect(int sect, int32_t nextsect, int cliente)
{
	char * bufferConsola;
	double tiempo;
	int32_t ssect, psect, snextsect, pnextsect, sposactual, pposactual;
	bufferConsola = (char *) malloc(TAM_SECT);
	memset(bufferConsola, '\0', TAM_SECT);

	psect = pista(sect);
	ssect = sectpis(sect);
	pnextsect = pista(nextsect);
	snextsect = sectpis(nextsect);
	pposactual = pista(vecConfig.posactual);
	sposactual = sectpis (vecConfig.posactual);
	tiempo = timemovdisco(sect);

	sprintf(bufferConsola, "%d,%d,%d,%d,%d,%d,%g", psect, ssect, pnextsect, snextsect, pposactual, sposactual, tiempo);
	sleep(1);
	send(cliente, bufferConsola, strlen(bufferConsola),0);
	free(bufferConsola);
}
//------------Prueba-----------------//

void msjprueba(nipc_packet * msj)
{
	msj->len = 1024;
	strcpy((char *) msj->payload.contenido, "1234,hola como estas vos...");
	msj->type = 2;

}
