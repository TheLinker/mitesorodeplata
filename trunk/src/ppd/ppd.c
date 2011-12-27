#include "ppd.h"
#include "../common/nipc.h"
#include "../common/utils.h"
#include "../common/log.h"

config_t vecConfig;
int32_t dirArch, sectxpis, * cantPedidos=0;
void * diskMap;
cola_t *headprt = NULL, *saltoptr = NULL, *largaptr = NULL;
size_t len = 100;
sem_t semEnc, semAten /*, semCab*/;
log_t* logppd;

int main()
{
        pthread_t thConsola, thAtenderpedidos;
        char* mensaje = NULL;
        int32_t  thidConsola,thidAtenderpedidos, sectores;
        div_t res;

        sem_init(&semEnc,1,1);
        //sem_init(&semCab,1,1);
        sem_init(&semAten,1,0);

        res = div(vecConfig.sectores, 8);
        if(0 != res.rem)
                sectores = (vecConfig.sectores - res.rem) + 8;

        //pid = getpid();
        vecConfig = getconfig("config.txt");
        logppd = log_new(vecConfig.rutalog, "PPD", vecConfig.flaglog);
        sectxpis= (vecConfig.sectores/vecConfig.pistas);
        dirArch = abrirArchivoV(vecConfig.rutadisco);
        diskMap = discoMap(vecConfig.sectores, dirArch);
        cantSect = vecConfig.sectores;


        printf("%s\n", vecConfig.modoinit);


        int pid = fork();

        if (pid == 0)
        {
                        if(-1 == execle("/home/utn_so/Desarrollo/Workspace/consolappd/Debug/consolappd", "consolappd", NULL, NULL))
                        {
                                printf("Error al ejecutar la consola \n");
                                printf("NUMERO DE ERROR: %d \n", errno);
                        }
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
        sleep(1);
        while(1)
        {
                if (0 > recv_socket(&msj, *socket));
                {
                        //if(0 == (msj.payload.sector % 5000))
                        //printf("%d, %d, %d ENTRA AL INSERT \n", msj.type, msj.len, msj.payload.sector);
                        //log_info(logppd, "Escuchar Pedidos", "Message info: Pedido escritura sector %d", msj.payload.sector);
                        if(0 == strcmp(vecConfig.algplan, "cscan"))
                        {
                                sem_wait(&semEnc);
                                //sem_wait(&semCab);
                                insertCscan(msj, &headprt, &saltoptr, vecConfig.posactual, *socket);
                                sem_post(&semEnc);
                                //sem_post(&semCab);
                                sem_post(&semAten);
                        }else
                        {
                                sem_wait(&semEnc);
                                //sem_wait(&semCab);
                                insertNStepScan(msj,&cantPedidos, &headprt, &saltoptr, &largaptr, vecConfig.posactual, *socket);
                                sem_post(&semEnc);
                                //sem_post(&semCab);
                                sem_post(&semAten);

                        }
                }
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
                        obtenercola(&headprt, &saltoptr, cola);
                        tamcol = tamcola(&headprt, &saltoptr);
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
                                ped = desencolar(&headprt, &saltoptr);
                                obtenerrecorrido(ped->sect, trace, vecConfig.posactual);
                                time= timemovdisco(ped->sect, vecConfig.posactual);
                                log_info(logppd, "Atender Pedidos", "Message info: Procesamiento de pedido\nCola de Pedidos:[%s] Tamaño:%d\nPosicion actual: %d:%d\nSector Solicitado: %d:%d\nSectores Recorridos: %s\nTiempo Consumido: %gms\nPróximo Sector: %d\n", buffer,tamcol ,pista(vecConfig.posactual), sectpis(vecConfig.posactual), pista(ped->sect), sectpis(ped->sect), trace, time, pista(cola[1]), sectpis(cola[1]));

                                posCabeza = vecConfig.posactual;
                                sem_post(&semEnc);
                                //sem_post(&semCab);
                        }else
                        {
                                ped = desencolarNStepScan(&headprt, &saltoptr, &largaptr, &cantPedidos, vecConfig.posactual);
                                obtenerrecorrido(ped->sect, trace, vecConfig.posactual);
                                time= timemovdisco(ped->sect, vecConfig.posactual);
                                log_info(logppd, "Atender Pedidos", "Message info: Procesamiento de pedido\nCola de Pedidos:[%s] Tamaño:\nPosicion actual: %d:%d\nSector Solicitado: %d:%d\nSectores Recorridos: %s\nTiempo Consumido: %gms\nPróximo Sector: %d\n", buffer,pista(vecConfig.posactual), sectpis(vecConfig.posactual), pista(ped->sect), sectpis(ped->sect), trace, time, pista(cola[1]), sectpis(cola[1]));
                                *cantPedidos --;
                                posCabeza = vecConfig.posactual;
                                sem_post(&semEnc);
                                //sem_post(&semCab);
                        }
                }else
                {
                        if(0 == strcmp(vecConfig.algplan, "cscan"))
                        {
                                sem_wait(&semAten);
                                //sem_wait(&semCab);
                                sem_wait(&semEnc);
                                obtenercola(&headprt, &saltoptr, cola);
                                ped = desencolar(&headprt, &saltoptr);
                                posCabeza = vecConfig.posactual;
                                sem_post(&semEnc);
                                //sem_post(&semCab);
                        }else
                        {
                                sem_wait(&semAten);
                                //sem_wait(&semCab);
                                sem_wait(&semEnc);
                                obtenercola(&headprt, &saltoptr, cola);
                                ped = desencolarNStepScan(&headprt, &saltoptr, &largaptr, &cantPedidos, vecConfig.posactual);
                                *cantPedidos --;
                                posCabeza = vecConfig.posactual;
                                sem_post(&semEnc);
                                //sem_post(&semCab);
                        }
                }

                if (ped != NULL)
                {
                        if((vecConfig.flaglog == 3) || (vecConfig.flaglog == 4))
                        {
                        }
                        //if(0 == (ped->sect % 5000))
                                //printf("SECTOR PEDIDO %d \n", ped->sect);

                        switch(ped->oper)
                        {

                                case nipc_req_read:
                                        sleep(vecConfig.tiempolec);
                                        leerSect(ped->sect, ped->socket);
                                        moverCab(ped->sect);
                                        break;
                                case nipc_req_write:
                                        sleep(vecConfig.tiempoesc);
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
                free(trace);
                }
        }
}

void escucharConsola()
{
        //while que escuche consola
        char infodisc[30];

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

        do      //verifico que se quede esperando la conexion en caso de error

                cliente = accept ( servidor, NULL, NULL);

        while (cliente == -1);

        memset(infodisc,'\0', 30);
        sprintf(infodisc, "%d,%d", vecConfig.pistas, vecConfig.sectores);
        send(cliente, infodisc, strlen(infodisc),0);
        //puts ("\n Acepto la conexion \n");

        while(1)
        {

                if(recv(cliente,comando,sizeof(comando),0) == -1) // recibimos lo que nos envia el cliente
                {
                        printf("error recibiendo\n");
                        exit(0);
                }

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
        nipc_packet resp;

        if( (0 <= sect) && (cantSect >= sect))
        {
                //if(0 == (sect % 5000))
                        //printf("----------------Entro a Leer------------------- \n");
                //dirMap = paginaMap(sect, dirArch);
                //res = div(sect, 8);
                dirSect = diskMap + (sect *512);
                memcpy(&buffer, dirSect, TAM_SECT);
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
                printf("El sector no es valido: %d\n",sect);
        }


}


void escribirSect(int sect, char buffer[512], nipc_socket sock)
{
        nipc_packet resp;


                if((0 <= sect) && (cantSect >= sect))
                {
                        //if(0 == (sect % 5000))
                                //printf("----------------Entro a Escribir------------------- \n");
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
                        printf("El sector no es valido\n");
                }

}

int abrirArchivoV(char * pathArch)                      //Se le pasa el pathArch del config. Se mapea en esta funcion lo cual devuelve la direccion en memoria
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
        if(ret = posix_madvise(diskMap, tam, POSIX_MADV_RANDOM))
                        printf("El madvise ha fallado. Número de error: %d \n", ret);
        //printf("%d %X\n", errno, dirMap[0]);

        return diskMap;
}

//---------------Funciones Consola------------------//


void funcInfo(int cliente)
{
        char  bufferConsola[15];

        memset(bufferConsola, '\0', 15);
        //sem_wait(&semCab);
        sprintf(bufferConsola, "%d", vecConfig.posactual);
        //sem_post(&semCab);
        printf("%s\n", bufferConsola);
        send(cliente,bufferConsola, sizeof(bufferConsola),0);
        printf("%s\n", bufferConsola);
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
                        insertCscan(pedido, &headprt, &saltoptr, vecConfig.posactual,0);
                        sem_post(&semEnc);
                        //sem_post(&semCab);
                        sem_post(&semAten);
                        primSec++;
                }else
                {
                        sem_wait(&semEnc);
                        //sem_wait(&semCab);
                        insertNStepScan(pedido,&cantPedidos, &headprt, &saltoptr, &largaptr, vecConfig.posactual,0);
                        sem_post(&semEnc);
                        //sem_post(&semCab);
                        sem_post(&semAten);
                        primSec++;
                }
        }

        strcpy(bufferConsola, "Se han encolado correctamente los sectores para ser limpeados");
        send(cliente,bufferConsola,strlen(bufferConsola),0);
        free(bufferConsola);
        return;
}

void funcTrace(char * parametros, int cliente)
{
        int i, j, h, cantparam =0;
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
                pedido.payload.sector= calcularSector(lsectores[h]);
                //encolar
                log_info(logppd, "Escuchar Consola", "Message info: Pedido trace sector %d", pedido.payload.sector);
                if(0 == strcmp(vecConfig.algplan, "cscan"))
                        insertCscan(pedido, &headprt, &saltoptr, vecConfig.posactual,cliente);
                else
                {
                        insertNStepScan(pedido,&cantPedidos, &headprt, &saltoptr, &largaptr, vecConfig.posactual,cliente);
                }
        }
        //sem_post(&semCab);
        for(h=0; h<cantparam; h++)
                sem_post(&semAten);

        sem_post(&semEnc);


        return;
}
int calcularSector(char structSect[25])
{
        int pist = atoi(strtok(structSect, ":"));
        int sect = atoi(strtok(NULL, "\0"));
        return ((pist * sectxpis) + sect);
}

void traceSect(int sect, int32_t nextsect, int cliente, int32_t posCab, int32_t cola[20])
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
