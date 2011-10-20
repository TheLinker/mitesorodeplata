#include "ppd.h"


config_t vecConfig;
char * bufferConsola;

int main()
{
	vecConfig = getconfig("config.txt");
	dirMap = abrirArchivoV(vecConfig.rutadisco);
	//conectar()
	//CREAR TRES HILOS, UNO PARA ESCUCHAR PEDIDOS Y ENCOLARLOS, OTRO PARA ATENDER LOS PEDIDOS Y OTRO PARA LA CONSOLA
	escucharPedidos();
	//escucharConsola()
	return 1;
}


void escucharPedidos(void)
{
	//HACER UN WHILE Q ESCUCHE PEDIDOS
	nipc_packet msj;
	msjprueba(&msj);

	if(0 == strcmp(vecConfig.algplan, "cscan"))
		insertCscan(msj, vecConfig.posactual);
	else
		insertFifo(msj);


	//atenderPedido(/*dirmap, payloadDescriptor, sect, param*/);

return;
}

void atenderPedido(void)
{

	//switch(payloadDescriptor)   TODO
	//{
		//case LEER:
			//leerPedido(sect);
			//break;
		//case ESCRIBIR:
			//escribirPedido(sect, payload);
			//break;
		//default:
			//printf("Error comando PPD";
			//break;
	//}

	//    ### HAY Q DEFINIR LOS VALORES PARA LEER Y ESCRIBIR Y DEJAR EL SWITCH DE ARRIBA ###

	//recv()   ###Queda esperando a q lleguen pedidos###
	if(0 == strcmp(comando,"leer"))
	{
		//pasar lo q me mandan como parametro a un int
		leerSect(sect, dirArch);
	}
	else
	{
		if(0 == strcmp(comando,"escribir"))
		{
			escribirSect(param, sect, dirArch);
		}
		else
		{
			printf("Ha ingresado un comando invalido\n");
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

	strcpy ( direccionServidor.sun_path, "fichero" );   /* nombre */
	unlink( "fichero" );
	bind ( servidor, punteroServidor, lengthServidor );   /* crea el fichero */

	puts ("\n estoy a la esperaaaaa \n");
	listen ( servidor, 5 );

	cliente = accept ( servidor, punteroServidor, &lengthServidor );
	puts ("\n acepto la conexion \n");

	if(recv(cliente,comando,sizeof(comando),0)==-1) // recivimos lo que nos envia el cliente
	{
		printf("error recibiendo");
		exit(0);
	}

	atenderConsola(comando);

	close( cliente );

}

void atenderConsola(char comando[15])
{
	if(0 == strcmp(comando,"info"))
		{
			funcInfo();
		}
		else
		{
			if(0 == strcmp(comando,"clean"))
			{
				funcClean();
			}
			else
			{
			if(0 == strcmp(comando,"trace"))
			{
				funcTrace();
			}
			else
			{
				printf("Ha ingresado un comando invalido desde la consola\n");
			}
			}

		}
}


//---------------Funciones PPD------------------//


void leerSect(int sect, FILE * dirArch)
{

	if( (0 >= sect) && (cantSect <= sect))
	{
		dirMap = (int *) paginaMap(sect, dirArch);
		res = div(sect, 8);
		dirSect = dirMap + ((res.rem *8 *512 ) - 1);  //NO SE SI VA O NO EL -1    TODO
		memcpy(buffer, dirSect, TAM_SECT);
		if(0 != munmap(dirMap,TAM_PAG))
			printf("Fallo la eliminacion del mapeo\n");
		//mando el buffer por el protocolo al raid
	}
	else
	{
		printf("El sector no es valido\n");
	}


}


void escribirSect(char param[15], int sect, FILE * dirArch)
{
	//validar numero de sector
	//buscar en el puntero del archivo la direccion donde arrancaria el sector
	//buffer = (char *) malloc (TAM_SEC);
	//inicializo un buffer (menset) y copio lo que hay q escribir
	//copio el buffer a la memoria mapeada
	//doy aviso de OK

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
	res = div(sect, 8);
	offset = (res.quot * 512);
	dirMap = mmap(NULL,TAM_PAG, PROT_WRITE, MAP_SHARED, (int) dirArch , offset);

	return dirArch;
}

//---------------Funciones Consola------------------//


void funcInfo()
{
	sprintf(bufferConsola, "%d", vecConfig.posactual);
	//send(socket,bufferConsola,strlen(bufferConsola),0);
	return;
}

void funcClean(/*char * parametros*/)
{
	//char * strprimSec, * strultSec;
	//int primSec, ultSec;

	//strprimSec = strtok(parametros, ",");
	//strultSec = strtok(NULL, "\0");
	//primSec = atoi(strprimSec);
	//ultSec = atoi(strultSec);
	//memset(bufferConsola, '\0', TAM_SECT);
	//while(primsec <= ultsec)
	//{
	//	escribirSect(bufferConsola, primsec, dirArch);
	//	primsec++;
	//}

	strcpy(bufferConsola, "Se han limpiado correctamente los sectores");
	//send(socket,bufferConsola,strlen(bufferConsola),0);
	return;
}

void funcTrace(/*char * parametros*/)
{
	//int cantparam = 0;
	//simular el disco
	return;
}

//------------Prueba-----------------//

void msjprueba(nipc_packet * msj)
{
	msj->len = 1024;
	strcpy(msj->payload, "1234,hola como estas vos...");
	msj->type = 2;

}
