#include "ppd.h"


config_t vecConfig;
char * bufferConsola;

int main()
{
	vecConfig = getconfig("config.txt");
	dirArch = abrirArchivoV(vecConfig.rutadisco);
	conectarConPraid();
	//CREAR TRES HILOS, UNO PARA ESCUCHAR PEDIDOS Y ENCOLARLOS, OTRO PARA ATENDER LOS PEDIDOS Y OTRO PARA LA CONSOLA
	escucharPedidos();
	escucharConsola();
	//atenderPedido(/*dirmap, payloadDescriptor, sect, param*/);
	return 1;
}

void conectarConPraid();  //ver tipos de datos 
{
  nipc_socket create_socket();  // ver tipos de datos
}

nipc_socket create_socket(char *host, uint16_t port)
{
	nipc_socket ppd;
  struct sockaddr_in addr_ppd;
  uint32_t i = 1;

	if ( (ppd = socket(AF_INET,SOCK_STREAM,0)) <0 )
  {
    printf("ERROR AL CREAR EL SOCKET");
    exit(1);
  }

	addr_ppd.sin_family = AF_INET;
  addr_ppd.sin_port=htons(50003);  //cambiar puerto
  addr_ppd.sin_addr.s_addr=inet_addr("127.0.0.1");

	if( connect(ppd, (struct sockaddr *) &addr_ppd, sizeof(add_ppd)) <0 )
	{
		printf("ERROR EN LA CONEXION");
		exit(1);
	}	

	//recv();

	//send();

	return ppd;

}

void escucharPedidos(void)
{
	cola_t *headprt = NULL, *saltoptr = NULL;
	nipc_packet msj; /*PRUEBA*/
	msjprueba(&msj); /*PRUEBA*/

	if(0 == strcmp(vecConfig.algplan, "cscan"))
		//HACER UN WHILE Q ESCUCHE PEDIDOS
		insertCscan(msj, headprt, saltoptr, vecConfig.posactual);
	else
		//HACER UN WHILE Q ESCUCHE PEDIDOS
		insertFifo(msj, headprt);

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
			escribirSect(buffer, sect, dirArch);
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
   
	if (bind ( servidor, punteroServidor, lengthServidor )) <0)   /* crea el fichero */
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

	if(recv(cliente,comando,sizeof(comando),0) == -1) // recibimos lo que nos envia el cliente
	{
		printf("error recibiendo");
		exit(0);
	}

	atenderConsola(comando);

	close( cliente );

}

void atenderConsola(char comando[30])
{
	char * funcion, * parametros;

	funcion = strtok(comando," ");
	parametros = strtok (NULL, "\0");

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


void leerSect(int sect, FILE * dirArch)
{
	div_t res;

	if( (0 >= sect) && (cantSect <= sect))
	{
		dirMap = paginaMap(sect, dirArch);
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


void escribirSect(char buffer[512], int sect, FILE * dirArch)
{
	div_t res;

		if( (0 >= sect) && (cantSect <= sect))
		{
			dirMap = paginaMap(sect, dirArch);
			res = div(sect, 8);
			dirSect = dirMap + ((res.rem *8 *512 ) - 1);  //NO SE SI VA O NO EL -1    TODO
			memcpy(dirSect, buffer, TAM_SECT);
			//envio mensaje de operacion exitosa
			if(0 != munmap(dirMap,TAM_PAG))
				printf("Fallo la eliminacion del mapeo\n");

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
	sprintf(bufferConsola, "%d", vecConfig.posactual);
	//send(socket,bufferConsola,strlen(bufferConsola),0);
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
		escribirSect(bufferConsola, primSec, dirArch);
		primSec++;
	}

	strcpy(bufferConsola, "Se han limpiado correctamente los sectores");
	//send(socket,bufferConsola,strlen(bufferConsola),0);
	return;
}

void funcTrace(char * parametros)
{
	//int cantparam = 0;
	//simular el disco
	return;
}

//------------Prueba-----------------//

void msjprueba(nipc_packet * msj)
{
	msj->len = 1024;
	strcpy((char *) msj->payload.contenido, "1234,hola como estas vos...");
	msj->type = 2;

}
