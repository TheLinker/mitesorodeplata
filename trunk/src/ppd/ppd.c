#include "ppd.h"
#include "../common/nipc.h"
#include "../common/utils.h"
#include "../common/log.h"

config_t vecConfig;
int32_t dirArch, sectxpis,cantPedidos=0;
void * diskMap;
cola_t *headptr = NULL, *saltoptr = NULL, *largaptr = NULL;
size_t len = 100;
sem_t semEnc, semAten /*, semCab*/;
log_t* logppd;

int32_t main()
{
	pthread_t thConsola, thAtenderpedidos;
	char* mensaje = NULL;

	sem_init(&semEnc,1,1);
	//sem_init(&semCab,1,1);
	sem_init(&semAten,1,0);

	//pid = getpid();
	vecConfig = getconfig("config.txt");
	logppd = log_new(vecConfig.rutalog, "PPD", vecConfig.flaglog);
	sectxpis= (vecConfig.sectores/vecConfig.pistas);
	dirArch = abrirArchivoV(vecConfig.rutadisco);
	diskMap = discoMap(vecConfig.sectores, dirArch);
	cantSect = vecConfig.sectores;

	printf("%s\n", vecConfig.modoinit);
	int32_t pid = fork();

	if (pid == 0)
	{
		if(-1 == execle("consolappd", "consolappd", NULL, NULL))
		{
				printf("Error al ejecutar la consola \n");
				printf("NUMERO DE ERROR: %d \n", errno);
		}
	}else
	{
		pthread_create( &thConsola, NULL, (void *) escucharConsola, (void*) mensaje);

		pthread_create( &thAtenderpedidos, NULL, (void *) atenderPedido, NULL);

		if(!(strncmp(vecConfig.modoinit, "CONNECT",7)))
		{
			printf("Conexion con praid\n");
			conectarConPraid();
		}
		else
			if(!(strncmp(vecConfig.modoinit, "LISTEN",6)))
			{
				printf("Conexion con Pfs\n");
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

	sock_new = create_socket();
	printf("socket: %d\n", sock_new);
	nipc_connect_socket(sock_new, vecConfig.ippraid, vecConfig.puertopraid);

	srand(time(NULL));
	buffer.type = 0;
	//strcpy((char *)buffer.payload.contenido, "l");
	sprintf((char *)buffer.payload.contenido,"Disco %d", rand()%100+1);
	buffer.len = 4 + strlen((char *)buffer.payload.contenido);
	buffer.payload.sector = 0;
	send_socket(&buffer ,sock_new);
	recv_socket(&buffer, sock_new);
	printf("%d\n", buffer.len);
	
	buffer.type = 4;
	strcpy((char *)buffer.payload.contenido, vecConfig.chs);
	buffer.len = 4 + strlen((char *)buffer.payload.contenido);
	buffer.payload.sector = 0;
	send_socket(&buffer ,sock_new);
	
	socket = malloc(sizeof(nipc_socket));
	*socket = sock_new;

	pthread_create( &thEscucharPedidos, NULL,(void *)  escucharPedidos,  socket);

	pthread_join(thEscucharPedidos, NULL);
}


void escucharPedidos(nipc_socket *socket)
{
	nipc_packet msj;
	sleep(1);
	
	while(1)
	{
		if (0 < recv_socket(&msj, *socket))
		{
			//log_info(logppd, "Escuchar Pedidos", "Message info: Pedido escritura sector %d", msj.payload.sector);
			if(0 == strcmp(vecConfig.algplan, "cscan"))
			{
				sem_wait(&semEnc);
				//sem_wait(&semCab);
				insertCscan(msj, &headptr, &saltoptr, vecConfig.posactual, *socket);
				sem_post(&semEnc);
				//sem_post(&semCab);
				sem_post(&semAten);
			}else
			{
				sem_wait(&semEnc);
				//sem_wait(&semCab);
				insertNStepScan(msj,&cantPedidos, &headptr, &saltoptr, &largaptr, vecConfig.posactual, *socket);
				sem_post(&semEnc);
				//sem_post(&semCab);
				sem_post(&semAten);
			}
		}
		else
		  return;
	}
	return;
}

void atenderPedido()
{
    ped_t * ped;
    char buffer[1024], bufferaux[11], * trace;
    int32_t l, posCabeza, cola[20], tamcol;
    double time;

    while(1)
    {
        if((vecConfig.flaglog == 3) || (vecConfig.flaglog == 4))
        {
            sem_wait(&semAten);
            //sem_wait(&semCab);
            sem_wait(&semEnc);
            if(0 != strcmp(vecConfig.algplan, "cscan"))
                llenarColas(&headptr, &saltoptr, &largaptr, &cantPedidos, vecConfig.posactual);
            obtenercola(&headptr, &saltoptr, cola);
            tamcol = tamcola(&headptr, &saltoptr);
            memset(buffer, '\0', sizeof(buffer));
            for(l=0;(l<sizeof(cola)) && (cola[l] != -1); l++)
            {
                memset(bufferaux, '\0', sizeof(bufferaux));
                sprintf(bufferaux, "%d:%d,", pista(cola[l]), sectpis(cola[l]));
                strcat(buffer, bufferaux);
            }

            trace =(char *) malloc(2000);

            if(0 == strcmp(vecConfig.algplan, "cscan"))
            {
                ped = desencolar(&headptr, &saltoptr);
                obtenerrecorrido(ped->sect, trace, vecConfig.posactual);
                time= timemovdisco(ped->sect, vecConfig.posactual);
                log_info(logppd, "Atender Pedidos", "Message info: Procesamiento de pedido\n" "Cola de Pedidos:[%s] Tamaño:%d\n"
                                                    "Posicion actual: %d:%d\n" "Sector Solicitado: %d:%d\n"
                                                    "Sectores Recorridos: %s\n" "Tiempo Consumido: %gms\n"
                                                    "Próximo Sector: %d\n", buffer, tamcol, pista(vecConfig.posactual), 
                                                    sectpis(vecConfig.posactual), pista(ped->sect), sectpis(ped->sect),
                                                    trace, time, pista(cola[1]), sectpis(cola[1]));

                posCabeza = vecConfig.posactual;
                sem_post(&semEnc);
                //sem_post(&semCab);
            }else
            {
                ped = desencolar(&headptr, &saltoptr);
                time= timemovdisco(ped->sect, vecConfig.posactual);
                log_info(logppd, "Atender Pedidos", "Message info: Procesamiento de pedido\n" "Cola de Pedidos:[%s] Tamaño:\n"
                                                    "Posicion actual: %d:%d\n" "Sector Solicitado: %d:%d\n"
                                                    "Sectores Recorridos: %s\n" "Tiempo Consumido: %gms\n"
                                                    "Próximo Sector: %d\n", buffer,pista(vecConfig.posactual), 
                                                    sectpis(vecConfig.posactual), pista(ped->sect), 
                                                    sectpis(ped->sect), trace, time, pista(cola[1]), 
                                                    sectpis(cola[1]));
                cantPedidos --;
                posCabeza = vecConfig.posactual;
                sem_post(&semEnc);
                //sem_post(&semCab);
            }

            free(trace);
        } else {
            if(0 == strcmp(vecConfig.algplan, "cscan"))
            {
                sem_wait(&semAten);
                //sem_wait(&semCab);
                sem_wait(&semEnc);
                obtenercola(&headptr, &saltoptr, cola);
                ped = desencolar(&headptr, &saltoptr);
                posCabeza = vecConfig.posactual;
                sem_post(&semEnc);
                //sem_post(&semCab);
            }else
            {
                sem_wait(&semAten);
                //sem_wait(&semCab);
                sem_wait(&semEnc);
                llenarColas(&headptr, &saltoptr, &largaptr, &cantPedidos, vecConfig.posactual);
                obtenercola(&headptr, &saltoptr, cola);
                ped = desencolar(&headptr, &saltoptr);
                cantPedidos --;
                posCabeza = vecConfig.posactual;
                sem_post(&semEnc);
                //sem_post(&semCab);
            }
        }

        if (ped != NULL)
        {
            switch(ped->oper)
            {
                case nipc_req_read:
                    usleep(vecConfig.tiempolec);
                    leerSect(ped->sect, ped->socket);
                    moverCab(ped->sect);
                    break;
                case nipc_req_write:
                    usleep(vecConfig.tiempoesc);
                    escribirSect(ped->sect, ped->buffer, ped->socket);
                    moverCab(ped->sect);
                    break;
                case nipc_req_trace:
                    traceSect(ped->sect, ped->nextsect,(int) ped->socket, posCabeza, cola);
                    moverCab(ped->sect);
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
	char infodisc[30];

	int32_t servidor, cliente, lengthServidor;
	struct sockaddr_un direccionServidor;
	struct sockaddr* punteroServidor;

	signal ( SIGCHLD, SIG_IGN );

	punteroServidor = ( struct sockaddr* ) &direccionServidor;
	lengthServidor = sizeof ( direccionServidor );
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

	do      //verifico que se quede esperando la conexion en caso de error
		cliente = accept ( servidor, NULL, NULL);
	while (cliente == -1);

	memset(infodisc,'\0', 30);
	sprintf(infodisc, "%d,%d", vecConfig.pistas, vecConfig.sectores);
	send(cliente, infodisc, strlen(infodisc),0);
	//puts ("\n Acepto la conexion \n");

	int32_t bandera=0;
	while(1)
	{
		bandera = recv(cliente,comando,sizeof(comando),0);
		if(bandera < 0) // recibimos lo que nos envia el cliente
		{
			printf("error recibiendo\n");
			exit(0);
		}
		else
			atenderConsola(comando, cliente);
	}
	close( cliente );
}

void atenderConsola(char comando[100], int32_t cliente)
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
    nipc_packet resp;
	void * dirSect;
    char buffer[TAM_SECT];
    if( (0 <= sect) && (cantSect >= sect))
    {
        //dirMap = paginaMap(sect, dirArch);
        //res = div(sect, 8);
        dirSect = diskMap + (sect * 512);
        memcpy(&buffer, dirSect,512);// TAM_SECT);
        //if(0 != munmap(dirMap,TAM_PAG))
            //printf("Fallo la eliminacion del mapeo\n");

        resp.type = 1;
        resp.payload.sector = sect;
        memcpy((char *) resp.payload.contenido, &buffer, TAM_SECT);
        resp.len = sizeof(resp.payload);
        send_socket(&resp, sock);    //mando el buffer por el protocolo al raid
    }
    else
    {
        resp.type = nipc_error;
        resp.payload.sector = sect;
        memset(buffer, '\0', TAM_SECT);
        sprintf(buffer, "El sector %d no es válido.", sect);
        strcpy((char *) resp.payload.contenido, buffer);
        resp.len = strlen((char *)resp.payload.contenido) + sizeof(resp.payload.sector) + 1;
        send_socket(&resp, sock);    //mando el buffer por el protocolo al raid
    }


}


void escribirSect(int32_t sect, char buffer[512], nipc_socket sock)
{
	nipc_packet resp;
	if((0 <= sect) && (cantSect >= sect))
	{
		//dirMap = paginaMap(sect, dirArch);
		//res = div(sect, 8);
		dirSect = diskMap + (sect *512 );
		memcpy(dirSect, buffer, TAM_SECT);
		msync(dirSect, 512, MS_ASYNC);
		//if(0 != munmap(dirMap,TAM_PAG))
				//printf("Fallo la eliminacion del mapeo\n");

		resp.type = 2;
		resp.payload.sector = sect;
		memset(resp.payload.contenido,'\0', TAM_SECT);
		resp.len = 4;

		send_socket(&resp, sock);
	}
	else
	{
        resp.type = nipc_error;
        resp.payload.sector = sect;
        sprintf(buffer, "El sector %d no es válido.", sect);
        strcpy((char *) resp.payload.contenido, buffer);
        resp.len = strlen((char *)resp.payload.contenido) + sizeof(resp.payload.sector) + 1;
        send_socket(&resp, sock);    //mando el buffer por el protocolo al raid
	}
}

int32_t abrirArchivoV(char * pathArch)   //Se le pasa el pathArch del config. Se mapea en esta funcion lo cual devuelve la direccion en memoria
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

void * discoMap(int32_t sectores, int32_t dirArch)
{
        int32_t ret=0, tam;
        tam = (sectores * 512);
        diskMap = mmap(NULL, tam, PROT_READ | PROT_WRITE, MAP_SHARED, dirArch , 0);
        ret = posix_madvise(diskMap, tam, POSIX_MADV_RANDOM);
		if(ret)
			printf("El madvise ha fallado. Número de error: %d \n", ret);
        //printf("%d %X\n", errno, dirMap[0]);

        return diskMap;
}

//---------------Funciones Consola------------------//


void funcInfo(int32_t cliente)
{
        char  bufferConsola[15];

        memset(bufferConsola, '\0', 15);
        //sem_wait(&semCab);
        sprintf(bufferConsola, "%d", vecConfig.posactual);
        //sem_post(&semCab);
        send(cliente,bufferConsola, sizeof(bufferConsola),0);
        return;
}

void funcClean(char * parametros, int32_t cliente)
{
    char * strprimSec, * strultSec, * bufferConsola;
    int32_t primSec, ultSec=0;
    nipc_packet pedido;

    bufferConsola = (char *) malloc(TAM_SECT);
    memset(bufferConsola, '\0', TAM_SECT);
    strprimSec = strtok(parametros, ":");
    strultSec = strtok(NULL, "\0");
    primSec = atoi(strprimSec);
    if (strultSec != NULL)
		ultSec = atoi(strultSec);
	else
	{
		strcpy(bufferConsola, "Los parametros son incorrectos\n");
		send(cliente,bufferConsola,strlen(bufferConsola),0);
		free(bufferConsola);
		return;
	}
    pedido.type = nipc_req_write;
    memset(pedido.payload.contenido, '\0', TAM_SECT);
    pedido.len = (sizeof(nipc_packet));
    //encolar
    while(primSec <= ultSec)
    {
        if (0 == (primSec % 30))
            //usleep(6000);
        //setea el sector
        pedido.payload.sector= primSec;
        //encolar
        if(0 == strcmp(vecConfig.algplan, "cscan"))
        {
            sem_wait(&semEnc);
            //sem_wait(&semCab);
            insertCscan(pedido, &headptr, &saltoptr, vecConfig.posactual,0);
            sem_post(&semEnc);
            //sem_post(&semCab);
            sem_post(&semAten);
            primSec++;
        }else
        {
            sem_wait(&semEnc);
            //sem_wait(&semCab);
            insertNStepScan(pedido,&cantPedidos, &headptr, &saltoptr, &largaptr, vecConfig.posactual,0);
            sem_post(&semEnc);
            //sem_post(&semCab);
            sem_post(&semAten);
            primSec++;
        }
    }

    strcpy(bufferConsola, "Se han encolado correctamente los sectores para ser limpeados\n");
    send(cliente,bufferConsola,strlen(bufferConsola),0);
    free(bufferConsola);
    return;
}

void funcTrace(char * parametros, int32_t cliente)
{
	int32_t i, j, h, cantparam =0;
	char cantparametros[100],lsectores[5][25];
	memset(cantparametros, '\0', 100);
	strncpy(cantparametros, parametros, 100);

	for(i=0; i<5; i++)
	{
		int32_t k;
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
		strcpy(lsectores[j], strtok(NULL, " "));
	}

	nipc_packet pedido;

	pedido.type = nipc_req_trace;
	pedido.len = (sizeof(nipc_packet));
	memset(pedido.payload.contenido, '\0', TAM_SECT);

	//encolar

	sem_wait(&semEnc);
	for(h=0; h<cantparam; h++)
	{
		//setea el sector
		int32_t control = calcularSector(lsectores[h]);
		if (control < 0)
		{
			send(cliente,"ERROR",strlen("ERROR"),0);
			sem_post(&semEnc);
			return;
		}
		pedido.payload.sector = control;
		//encolar
		log_info(logppd, "Escuchar Consola", "Message info: Pedido trace sector %d", pedido.payload.sector);
		if(0 == strcmp(vecConfig.algplan, "cscan"))
			insertCscan(pedido, &headptr, &saltoptr, vecConfig.posactual,cliente);
		else
		{
			insertNStepScan(pedido,&cantPedidos, &headptr, &saltoptr, &largaptr, vecConfig.posactual,cliente);
		}
	}
	//sem_post(&semCab);
	for(h=0; h<cantparam; h++)
		sem_post(&semAten);
	sem_post(&semEnc);
	return;
}

int32_t calcularSector(char structSect[25])
{
	int32_t pist = 0;
	int32_t sect = 0;
	pist = atoi(strtok(structSect, ":"));
	char * sect_str=strtok(NULL, "\0");
	if(sect_str != NULL)
	{
		sect = atoi(sect_str);
		return ((pist * sectxpis) + sect);
	}
	else
		return -1;
	
}

void traceSect(int32_t sect, int32_t nextsect, int32_t cliente, int32_t posCab, int32_t cola[20])
{
    char * bufferConsola, *aux;
    float tiempo = 0;
    int32_t ssect, psect, snextsect, pnextsect, sposactual, pposactual,p;
    bufferConsola = (char *) malloc(TAM_SECT);
    memset(bufferConsola, '\0', TAM_SECT);

    aux = (char *) malloc(TAM_SECT);

    psect = pista(sect);
    ssect = sectpis(sect);
    pnextsect = pista(nextsect);
    snextsect = sectpis(nextsect);
    pposactual = pista(posCab);
    sposactual = sectpis (posCab);
    tiempo = timemovdisco(sect, posCab);

    for(p=0; (p<20) && (cola[p] != -1); p++)
    {
        memset(aux, '\0', TAM_SECT);
        sprintf(aux, "%d:%d, ", pista(cola[p]),sectpis(cola[p]));
        strcat(bufferConsola, aux);
    }

    strcat(bufferConsola, ")");
    memset(aux, '\0', TAM_SECT);
    sprintf(aux, "%d,%d,%d,%d,%d,%d,%g\n", psect, ssect, pnextsect, snextsect, pposactual, sposactual, tiempo);
    strcat(bufferConsola, aux);

    sleep(1);
    send(cliente, bufferConsola, strlen(bufferConsola),0);
    free(aux);
    free(bufferConsola);
}
