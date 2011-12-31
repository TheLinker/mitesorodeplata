#include<stdio.h>
#include <time.h>
#include<unistd.h>
#include<string.h>
#include <sys/timeb.h>
#include<pthread.h>
#include<sys/msg.h>
#include<sys/sem.h>
#include<stdlib.h>
#include<signal.h>
#include "praid_func.h"
#include "praid1.h"
#include "nipc.h"
#include "log.h"

#define ESTADO_OK   1
#define ESTADO_FAIL 0

/**
 * Agrega un nuevo disco al RAID
 *
 */
void agregar_disco(datos **info_ppal,uint8_t *id_disco, nipc_socket sock_new)
{
	sem_wait(&((*info_ppal)->semaforo));
	disco *nuevo_disco;
	nuevo_disco = (disco *)malloc(sizeof(disco));
	memcpy(nuevo_disco->id,id_disco,strlen((char *)id_disco)+1);
	nuevo_disco->sock=sock_new;
	sem_init(&(nuevo_disco->sem_disco),0,1);
	nuevo_disco->pedido_sincro = 0;
	nuevo_disco->sector_sincro = (int32_t) -1;
	nuevo_disco->ya_sincro_start = NULL;
	nuevo_disco->cantidad_pedidos = 0;
	nuevo_disco->pedidos_start = NULL;
	nuevo_disco->pedidos_end = NULL;
	nuevo_disco->encolados = 0;
	nuevo_disco->sgte = (*info_ppal)->discos;
	//sem_wait(&((*info_ppal)->semaforo));
	(*info_ppal)->discos = nuevo_disco;
	//sem_post(&((*info_ppal)->semaforo));
	if(pthread_create(&nuevo_disco->hilo,NULL,(void *)espera_respuestas,(void *)info_ppal)!=0)
		printf("Error en la creacion del hilo\n");
	sem_post(&((*info_ppal)->semaforo));
}

/**
 * Lista todos los discos y sus pedidos asignados
 *
 */
void listar_pedidos_discos(disco *discos)
{
	disco *aux_disco;
	aux_disco = discos;
	while(aux_disco != NULL)	
	{
		printf("\tDisco %s, Cantidad %d, Sincronizado %d\n", aux_disco->id, aux_disco->cantidad_pedidos, aux_disco->sector_sincro);
		pedido *aux_pedido;
		aux_pedido = aux_disco->pedidos_start;
		while(aux_pedido!=NULL)
		{
			printf("\t\ttype_pedido %d, Sector %d\n", aux_pedido->type, aux_pedido->sector);
			aux_pedido=aux_pedido->sgte;
		}
		aux_disco = aux_disco->sgte;
	}
}
/**
 * Lista todos los discos
 *
 */
void listar_discos(disco *discos)
{
	disco *aux_disco;
	aux_disco = discos;
	while(aux_disco != NULL)	
	{
		printf("\tDisco %s, Cantidad %d, Encolados %d, Sincronizado %d\n", aux_disco->id, aux_disco->cantidad_pedidos, aux_disco->encolados, aux_disco->sector_sincro);
		aux_disco=aux_disco->sgte;
	}
}

/**
 * Devuelve el socket del disco con la menor cantidad de pedidos
 *
 * @return Socket de disco elegido
 */
nipc_socket menor_cantidad_pedidos(disco *discos, int32_t sector)
{
	disco *aux_disco;
	int32_t menor_pedido=8192*512;
	nipc_socket sock_disco = 0;
	aux_disco = discos;
	while(aux_disco != NULL)
	{
		if (menor_pedido > aux_disco->cantidad_pedidos  &&  sector <= aux_disco->sector_sincro)
		{
			sock_disco   = aux_disco->sock;
			menor_pedido = aux_disco->cantidad_pedidos;
		}
		aux_disco=aux_disco->sgte;
	}
	return sock_disco;
}

/**
 * Devuelve el socket del disco siguiente a recibir un pedido
 *
 * @return Socket de disco elegido
 */
nipc_socket uno_y_uno(datos **info_ppal, int32_t sector)
{
	disco *aux_disco;
	nipc_socket sock_disco = 0;
	aux_disco = (*info_ppal)->discos;
	while(aux_disco != NULL)
	{
		if ((*info_ppal)->sock_control != aux_disco->sock  &&  sector <= aux_disco->sector_sincro)
		{
			sock_disco = aux_disco->sock;
		}
		aux_disco=aux_disco->sgte;
	}
	if (sock_disco == 0)
	{
		aux_disco = (*info_ppal)->discos;
		while(aux_disco != NULL)
		{
			if (sector <= aux_disco->sector_sincro)
			{
				sock_disco = aux_disco->sock;
			}
			aux_disco = aux_disco->sgte;
		}
	}
	(*info_ppal)->sock_control = sock_disco;
	return sock_disco;
}

/**
 * Encola pedido en la lista
 *
 */
void * encolar(disco ** disco, pedido * elemento)
{
	if ((*disco)->pedidos_start == NULL)
	{
		(*disco)->pedidos_start = elemento;
		(*disco)->pedidos_end   = elemento;
	}
	else
	{
		(*disco)->pedidos_end->sgte = elemento;
		(*disco)->pedidos_end       = elemento;
	}
	return 0;
}

/**
 * Crear pedido
 *
 */
pedido * crear_pedido(nipc_packet mensaje, nipc_socket socket)
{
	pedido *nuevo_pedido;
	nuevo_pedido = (pedido *) malloc(sizeof(pedido));
	if(nuevo_pedido == NULL)
	{
		printf("fallo el malloc \n");
		getchar();
	}
	nuevo_pedido->sock = socket;
	nuevo_pedido->type = mensaje.type;
	nuevo_pedido->sector = mensaje.payload.sector;
	nuevo_pedido->sgte = NULL;
	return nuevo_pedido;
}

/**
 * Distribulle un pedido de lectura de la cola de mensajes
 *
 */
uint8_t* distribuir_pedido_lectura(datos **info_ppal, nipc_packet mensaje,nipc_socket sock_pfs)
{
	lista_pfs *aux_pfs = (*info_ppal)->pfs_activos;
	while(aux_pfs != NULL && aux_pfs->sock != sock_pfs)
		aux_pfs = aux_pfs->sgte;
	if (aux_pfs != NULL)
	{
		sem_wait(&(aux_pfs->semaforo));
		sectores_t *nueva_lectura;
		nueva_lectura = (sectores_t *)malloc(sizeof(sectores_t));
		nueva_lectura->sector = mensaje.payload.sector;
		nueva_lectura->sgte = aux_pfs->lecturas;
		aux_pfs->lecturas = nueva_lectura;
		sem_post(&(aux_pfs->semaforo));
	}
  
	nipc_socket sock_disco;
	//sem_wait(&((*info_ppal)->semaforo));
	sock_disco =  menor_cantidad_pedidos((*info_ppal)->discos,mensaje.payload.sector);
	//sock_disco =  uno_y_uno(info_ppal,mensaje.payload.sector);
	
	if (sock_disco != 0)
	{
		pedido *nuevo_pedido;
		nuevo_pedido = crear_pedido(mensaje,sock_pfs);
		disco *aux;
		aux = (*info_ppal)->discos;
		while((aux != NULL) && (aux->sock != sock_disco))
			aux = aux->sgte;
		sem_wait(&((*info_ppal)->semaforo));
		encolar(&aux,nuevo_pedido);
		aux->cantidad_pedidos++;
		sem_post(&((*info_ppal)->semaforo));
		if(send_socket(&mensaje,aux->sock)<=0)
		{
			//printf("Error envio pedido lectura %d ",mensaje.payload.sector);
		}
		return aux->id;
	}
	else
	{
		sem_post(&((*info_ppal)->semaforo));
		return (uint8_t*)"NODISK";
	}
}

/**
 * Distribulle un pedido de Escritura de la cola de mensajes
 *
 */
void distribuir_pedido_escritura(datos **info_ppal, nipc_packet mensaje,nipc_socket sock_pfs)
{
	lista_pfs *aux_pfs = (*info_ppal)->pfs_activos;
	while(aux_pfs != NULL && aux_pfs->sock != sock_pfs)
		aux_pfs = aux_pfs->sgte;
	if (aux_pfs != NULL)
	{
		sem_wait(&(aux_pfs->semaforo));
		sectores_t *nueva_escritura;
		nueva_escritura = (sectores_t *)malloc(sizeof(sectores_t));
		nueva_escritura->sector = mensaje.payload.sector;
		nueva_escritura->sgte = aux_pfs->escrituras;
		aux_pfs->escrituras = nueva_escritura;
		sem_post(&(aux_pfs->semaforo));
	}
	disco *aux;
	aux = (*info_ppal)->discos;
	while(aux != NULL)	
	{
		pedido *nuevo_pedido;
		nuevo_pedido = (pedido *)malloc(sizeof(pedido));
		nuevo_pedido->sock = sock_pfs;
		nuevo_pedido->type = mensaje.type;
		nuevo_pedido->sector = mensaje.payload.sector;
		nuevo_pedido->sgte = NULL;
		sem_wait(&((*info_ppal)->semaforo));
		if (aux->pedidos_start == NULL)
		{
			aux->pedidos_start = nuevo_pedido;
			aux->pedidos_end   = nuevo_pedido;
		}
		else
		{
			aux->pedidos_end->sgte = nuevo_pedido;
			aux->pedidos_end       = nuevo_pedido;
		}
		//encolar(&aux,&nuevo_pedido);
		aux->cantidad_pedidos++;
		sem_post(&((*info_ppal)->semaforo));
		if(send_socket(&mensaje,aux->sock)<0)
		{
			printf("\nError send escritura");
		}
		aux=aux->sgte;
	}
}

/**
 * Hilo para hacer los pedidos de sincronizacion 
 *
 */
void *pedido_sincronizacion(datos **info_ppal)
{
	nipc_packet  mensaje;
	disco       *el_disco;
	el_disco = (*info_ppal)->discos;
	while(el_disco != NULL && el_disco->pedido_sincro != 0)
		el_disco = el_disco->sgte;
	if (el_disco != NULL)
	{
		uint32_t i, control;
		//uint8_t *id_disco;
		mensaje.type=1;
		mensaje.len=4;
		//pedido *nuevo_pedido;
		//nipc_socket ppd_master = menor_cantidad_pedidos((*info_ppal)->discos,(*info_ppal)->max_sector);
		printf("COMIENZA Pedido Sincronizacion %s\n", el_disco->id);
		for(i=0 ; i < (*info_ppal)->max_sector ; i++)
		{
			mensaje.payload.sector = i;
			control = 0;
			usleep(50);
			while(el_disco->encolados > 50 )
			{
				usleep(500);
				control++;
				if (control == 2000) // 1000:medio segundo  2make000:un segundo
				{
					//printf("------------------------------\n");
					//printf("Re-pedido de sincronizacion %s - %d\n",el_disco->id,el_disco->sector_sincro+1);
					mensaje.payload.sector = (el_disco->sector_sincro+1);
					distribuir_pedido_lectura(info_ppal , mensaje , el_disco->sock);
					mensaje.payload.sector = i;
					control=0;
					//break;
				}
			}
			
			if (el_disco->pedido_sincro != 2)
				/*id_disco = */distribuir_pedido_lectura(info_ppal , mensaje , el_disco->sock);
			else
				break;
			
			if((i%50000)==0)
			{
				printf("------------------------------\n");
				printf("Pedido Sincronizacion parcial: %d\n",i);
				listar_discos((*info_ppal)->discos);
			}
		}
		if (el_disco->pedido_sincro == 0)
		{
			el_disco->pedido_sincro = 1;
			printf("FIN Pedido Sincronizacion %s\n", el_disco->id);
		}
		else
			printf("Se corto pedido sincronizacion %s\n", el_disco->id);
	}
	pthread_exit(NULL);
}

/**
 * Registrar sincronizacion
 *
 */
void *registrar_sincro(disco *el_disco, nipc_packet *mensaje)
{
	if((el_disco->sector_sincro + 1) > mensaje->payload.sector)
	{  /** ya estaba sincronizado este sector **/
		//printf("Sector YA ESTABA SINCRONIZADO en disco: %s - %d\n",el_disco->id,el_disco->sock);
	}
	if((el_disco->sector_sincro + 1) == mensaje->payload.sector)
	{  /** se incrementa en 1 sector_sincro y se revisa la cola para ver si hay otros **/
		el_disco->sector_sincro++;
		//printf("Sector preferido en disco: %s - %d\n",aux_disco->id,aux_pedidos->sock);
		sectores_t *aux;
		
		if (el_disco->ya_sincro_start != NULL)
		{ 
			while((el_disco->sector_sincro + 1) == (el_disco->ya_sincro_start)->sector)
			{
				el_disco->sector_sincro++;
				aux = el_disco->ya_sincro_start;
				el_disco->ya_sincro_start = (el_disco->ya_sincro_start)->sgte;
				free(aux);
				el_disco->encolados--;
				if(el_disco->ya_sincro_start == NULL)
					break;
			}
		}
	}
	if((el_disco->sector_sincro + 1) < mensaje->payload.sector)
	{  /** se encola ordenadamente para llevar el control de los sectores sincronizados **/
		insertar(&(el_disco->ya_sincro_start),mensaje->payload.sector);
		el_disco->encolados++;
	}
	return 0;
}

/**
 * Recibe constantemente los pedidos que alla en la cola
 *
 */
void *espera_respuestas(datos **info_ppal)
{
	config_t *config;
	config = calloc(sizeof(config_t), 1);
	config_read(config);
	log_t* log = log_new(config->log_path, "RAID", config->log_mode);
	pthread_t  hilo_sincro;
	time_t log_time ;
	struct tm *log_tm ;
	struct timeb tmili;
	char str_time[128] ;
	nipc_packet  mensaje;
	pedido      *aux_pedidos;
	pedido      *anterior;
	disco       *el_disco;
	el_disco = (*info_ppal)->discos;
	while(el_disco != NULL && el_disco->hilo != pthread_self())
		el_disco = el_disco->sgte;
	recv_socket(&mensaje,el_disco->sock);
	//char chs[20];
	//strcpy(chs,"(1024,1,1024)");
	printf("%s = ",mensaje.payload.contenido);
	uint32_t pistas = atoi(1 + strtok((char *)mensaje.payload.contenido,","));
	printf("%d x ",pistas);
	uint32_t cabezas = atoi(strtok(NULL,","));
	printf("%d x ",cabezas);
	uint32_t sectores_por_pista = atoi(strtok(NULL, ")"));
	printf("%d = ",sectores_por_pista);
	uint32_t cant_sectores = pistas * cabezas * sectores_por_pista;
	
	if ((*info_ppal)->discos->sgte != NULL)
	{
		if (cant_sectores != (*info_ppal)->max_sector)
		{
			mensaje.type = nipc_CHS;
			strcpy((char *)mensaje.payload.contenido,"CHS invalido");
			mensaje.len = 4+strlen("CHS invalido");
			if(send_socket(&mensaje,el_disco->sock)<0)
				printf("El CHS es invalido para el RAID actual");
			limpio_discos_caidos(info_ppal,el_disco->sock);
			pthread_exit(NULL);
		}
		else
		{
			/** SINCRONIZAR **/
			log_time = time(NULL);
			log_tm = localtime( &log_time );
			strftime( str_time , 127 , "%H:%M:%S" , log_tm ) ;
			if (ftime (&tmili))
			{
				perror("[[CRITICAL]] :: No se han podido obtener los milisegundos\n");
				return 0;
			}
			printf("COMIENZA Sincronizacion %s\n",el_disco->id);
			log_info(log, (char *)el_disco->id, "Message info: Comienza sincronizacion: %s - %s.%.3hu ",el_disco->id,str_time, tmili.millitm);
			
			if(pthread_create(&hilo_sincro,NULL,(void *)pedido_sincronizacion,(void *)info_ppal)!=0)
				printf("Error en la creacion del hilo\n");
		}
	}
	else
	{    /** UNICO DISCO - MAESTRO **/
		el_disco->sector_sincro = cant_sectores;
		(*info_ppal)->max_sector = cant_sectores;
		printf("Discos de %d sectores\n",cant_sectores);
	}
	printf("------------------------------\n");
	
	while(1)
	{
		if(recv_socket(&mensaje,el_disco->sock)>0)
		{
			mensaje.payload.contenido[mensaje.len-4]='\0';
			//printf("El mensaje es: %d - %d - %d - %s\n",mensaje.type,mensaje.len,mensaje.payload.sector,mensaje.payload.contenido);
			if(mensaje.type != nipc_handshake)
			{
				sem_wait(&((*info_ppal)->semaforo));
				//sem_wait(&(el_disco->sem_disco));
				aux_pedidos = el_disco->pedidos_start;
				anterior=NULL;
				while((aux_pedidos != NULL) && (aux_pedidos->type != mensaje.type || aux_pedidos->sector != mensaje.payload.sector))
				{
					anterior=aux_pedidos;
					aux_pedidos=aux_pedidos->sgte;
				}
				///**/////////////////////////////////////////**//
				if(aux_pedidos != NULL)
				{
					if(mensaje.type == nipc_error)
					{
						printf("ERROR: %d - %s\n",mensaje.payload.sector,mensaje.payload.contenido);
						log_error(log, (char *)el_disco->id, "Message error: Sector:%d Error: %s", mensaje.payload.sector,mensaje.payload.contenido);
					}
					/**	Enviar a quien hizo el pedido
							caso PFS se envia tal cual esta!
							caso PPD se modifica el tipo ya que pasa a ser escritura de un disco nuevo
					*/
					disco *aux_disco;
					aux_disco = (*info_ppal)->discos;
					while(aux_disco != NULL  &&  aux_disco->sock != aux_pedidos->sock)
						aux_disco = aux_disco->sgte;
					
					if (aux_disco != NULL)
					{  /** es una sincronizacion de disco **/
						mensaje.type = 2;
						pedido *nueva_sincro;
						nueva_sincro = (pedido *)malloc(sizeof(pedido));
						if(nueva_sincro == NULL)
						{
							printf("fallo el malloc \n");
							getchar();
						}
						nueva_sincro->sock = -1;
						nueva_sincro->type = mensaje.type;
						nueva_sincro->sector = mensaje.payload.sector;
						nueva_sincro->sgte = NULL;
						//sem_wait(&(aux_disco->sem_disco));
						encolar(&aux_disco,nueva_sincro);
						aux_disco->cantidad_pedidos++;
						//sem_post(&(aux_disco->sem_disco));
						/** ENVIAR ESCRITURA A DISCO **/
						if(send_socket(&mensaje,aux_pedidos->sock)<0)
						{ // Aca viene el problema
							//se soluciona con SIGPIPE ignorar
							//printf("error envio pedido sincronizacion escritura\n");
						}
					}
					else
					{
						if(aux_pedidos->sock != -1)
						{   /** es una respuesta a una escritura de un PFS o sincronizacion **/
							if(mensaje.type == nipc_req_read)
							{
								//printf("Recibida lectura del sector: %d\n",mensaje.payload.sector);
								//log_info(log, (char *)el_disco->id, "Message info: Recibida lectura del sector: %d", mensaje.payload.sector);
								lista_pfs *aux_pfs = (*info_ppal)->pfs_activos;
								while(aux_pfs != NULL  && aux_pfs->sock != aux_pedidos->sock)
									aux_pfs = aux_pfs->sgte;
								
								if (aux_pfs != NULL)
								{
									sem_wait(&(aux_pfs->semaforo));
									sectores_t *anterior = NULL;
									sectores_t *pedido = aux_pfs->lecturas;
									while(pedido != NULL  && pedido->sector != mensaje.payload.sector)
									{
										anterior = pedido;
										pedido = pedido->sgte;
									}
									if(pedido != NULL)
									{
										if(anterior == NULL)
										{
											aux_pfs->lecturas = (aux_pfs->lecturas)->sgte;
										}
										else
										{
											anterior->sgte = pedido->sgte;
										}
										free(pedido);
										if(send_socket(&mensaje,aux_pedidos->sock)<=0)
										{
											printf("error envio a PFS escritura\n");
										}
									}
									sem_post(&(aux_pfs->semaforo));
								}
							}
							if(mensaje.type == nipc_req_write)
							{
								//printf("Recibida escritura del sector: %d \n",mensaje.payload.sector);
								//log_info(log, (char *)el_disco->id, "Message info: Recibida escritura del sector: %d", mensaje.payload.sector);
								
								lista_pfs *aux_pfs = (*info_ppal)->pfs_activos;
								while(aux_pfs != NULL  && aux_pfs->sock != aux_pedidos->sock)
									aux_pfs = aux_pfs->sgte;
								
								sem_wait(&(aux_pfs->semaforo));
								sectores_t *anterior = NULL;
								sectores_t *pedido = aux_pfs->escrituras;
								while(pedido != NULL  && pedido->sector != mensaje.payload.sector)
								{
									anterior = pedido;
									pedido = pedido->sgte;
								}
								if(pedido != NULL)
								{
									if(anterior == NULL)
									{
										aux_pfs->escrituras = (aux_pfs->escrituras)->sgte;
									}
									else
									{
										anterior->sgte = pedido->sgte;
									}
									free(pedido);
									if(send_socket(&mensaje,aux_pedidos->sock)<=0)
									{
										printf("error envio a PFS escritura\n");
									}
								}
								sem_post(&(aux_pfs->semaforo));	
							}
						}
						else
						{
							//log_info(log, (char *)el_disco->id, "Message info: control de llegada: %d ",mensaje.payload.sector);
							/** es una respuesta de sincronizacion **/
							if((el_disco->sector_sincro%50000)==0)
							{
								printf("------------------------------\n");
								printf("Sincronizacion parcial: %d\n",el_disco->sector_sincro);
								listar_discos((*info_ppal)->discos);
							}
							
							registrar_sincro(el_disco,&mensaje);
							
							if((el_disco->sector_sincro + 1) == (*info_ppal)->max_sector)
							{
								log_time = time(NULL);
								log_tm = localtime( &log_time );
								strftime( str_time , 127 , "%H:%M:%S" , log_tm ) ;
								if (ftime (&tmili))
								{
									perror("[[CRITICAL]] :: No se han podido obtener los milisegundos\n");
								}
								printf("Sincronizacion COMPLETA %s\n",el_disco->id);
								log_info(log, (char *)el_disco->id, "Message info: Finaliza sincronizacion: %s - %s.%.3hu ",el_disco->id,str_time, tmili.millitm);
								el_disco->sector_sincro++;
								listar_discos((*info_ppal)->discos);
							}
						}
					}
					if(anterior == NULL)
					{
						el_disco->pedidos_start = (el_disco->pedidos_start)->sgte;
						if (aux_pedidos->sgte == NULL)
						{
							el_disco->pedidos_end = NULL;
						}
					}
					else
					{
						anterior->sgte = aux_pedidos->sgte;
						if (aux_pedidos->sgte == NULL)
						{
							el_disco->pedidos_end = anterior;
						}
					}
					free(aux_pedidos);
					el_disco->cantidad_pedidos--;
				}
				else
				{
					/*
					pedido      *aux_ped;
					disco       *aux_disk;
					int i;
					aux_disk = (*info_ppal)->discos;
					while(aux_disk != NULL)
					{
						i=0;
						printf("Disco %s - %d - cantidad: %d\n",aux_disk->id,aux_disk->sock,aux_disk->cantidad_pedidos );
						aux_ped = aux_disk->pedidos_start;
						while(aux_ped != NULL)
						{
							i++;
							//printf("\t Type %d - Sector %d - PFS %d\n",aux_ped->type,aux_ped->sector,aux_ped->sock );
							aux_ped = aux_ped->sgte;
						}
						printf("TOTAL: %d\n\n",i);
						aux_disk = aux_disk->sgte;
					}
					printf("------------------------------\n");
					*/
					lista_pfs   *aux_pfs;
					sectores_t  *aux_cola;
					uint16_t    banderin=0;
					aux_pfs = (*info_ppal)->pfs_activos;
					while(aux_pfs != NULL && banderin == 0)
					{
						sem_wait(&(aux_pfs->semaforo));
						//i=0;
						//printf("PFS %d \n",aux_pfs->sock );
						aux_cola = aux_pfs->lecturas;
						while(aux_cola != NULL  && banderin == 0)
						{
							//i++;
							//printf("\t Sector %d \n",aux_cola->sector );
							if (aux_cola->sector == mensaje.payload.sector)
							{
								el_disco->cantidad_pedidos--;
								if(send_socket(&mensaje,aux_pfs->sock)<=0)
								{
									printf("Error envio sector %d \n",mensaje.payload.sector);
									//getchar();
								}
								banderin = 1;
							}
							aux_cola = aux_cola->sgte;
						}
						//printf("TOTAL: %d\n\n",i);
						sem_post(&(aux_pfs->semaforo));
						aux_pfs = aux_pfs->sgte;
					}
					if (banderin == 0)
					{
						/*if (mensaje.payload.sector > el_disco->sector_sincro)
						{
							registrar_sincro(el_disco,&mensaje);
							//el_disco->cantidad_pedidos--;
						}
						else*/
						//{
							//printf("disco: %s - sector no solicitado: %d - %d - %d \n",el_disco->id, mensaje.type, mensaje.len, mensaje.payload.sector);
							//getchar();
						//}
					}
				}
				sem_post(&((*info_ppal)->semaforo));
				//sem_post(&(el_disco->sem_disco));
			}
		}
		else
		{
			el_disco->pedido_sincro = 2;
			pthread_join(hilo_sincro,NULL);
			limpio_discos_caidos(info_ppal,el_disco->sock);
			break;
		}
	}
	sleep(5);
	printf("------------------------------\n");
	pthread_exit(NULL);
	return 0;
}

/**
 * Inserta sector sincronizado en la listaregistrar_sincro(el_disco,&mensaje);
 *
 */
void insertar (sectores_t **lista, uint32_t sector)
{
	sectores_t *aux;
	sectores_t *new_sector = (sectores_t *)malloc(sizeof(sectores_t));
	new_sector->sector = sector;
	new_sector->sgte = NULL;
	if (*lista == NULL )
	{
		*lista = new_sector;
	}
	else
	{
		if((*lista)->sector > sector)
		{
			new_sector->sgte = *lista;
			*lista = new_sector;
		}
		else
		{
			aux=*lista;
			while(aux->sgte != NULL && (aux->sgte)->sector < sector)
				aux = aux->sgte;
			new_sector->sgte = aux->sgte;
			aux->sgte = new_sector;
		}
	}
}

/**
 * Limpia los PFS caidos
 *
 */
void liberar_pfs_caido(datos **info_ppal, nipc_socket sock)
{
	lista_pfs *aux_pfs;
	lista_pfs *anterior;
	aux_pfs = (*info_ppal)->pfs_activos;
	anterior=NULL;
	while(aux_pfs != NULL &&  aux_pfs->sock != sock)
	{
		anterior=aux_pfs;
		aux_pfs=aux_pfs->sgte;
	}
	nipc_close(aux_pfs->sock);
	if(anterior == NULL)
	{
		(*info_ppal)->pfs_activos = ((*info_ppal)->pfs_activos)->sgte;
	}
	else
	{
		anterior = anterior->sgte;
	}
	free(aux_pfs);
}

/**
 * Limpia los discos caidos
 *
 */
uint16_t limpio_discos_caidos(datos **info_ppal,nipc_socket sock_ppd)
{
	config_t *config;
	config = calloc(sizeof(config_t), 1);
	config_read(config);
	log_t* log = log_new(config->log_path, "PRAID", config->log_mode);
	uint32_t estado = ESTADO_FAIL; // inicializa como si no hubiera discos sincronizados
	nipc_packet mensaje;
	disco *aux_discos;
	disco *anterior = NULL;
	/** los dejo a mano para liberar al termino del programa **/
	sectores_t *aux_sock=(sectores_t *)malloc(sizeof(sectores_t));
	aux_sock->sector = sock_ppd;
	aux_sock->sgte = (*info_ppal)->sock_x_liberar;
	(*info_ppal)->sock_x_liberar = aux_sock;
	/**
	 *Saco el disco de la lista 
	 **/
	sem_wait(&((*info_ppal)->semaforo));
	aux_discos = (*info_ppal)->discos;
	while(aux_discos != NULL && aux_discos->sock != sock_ppd)
	{
		anterior = aux_discos;
		aux_discos = aux_discos->sgte;
	}
	if(anterior == NULL)
		(*info_ppal)->discos = ((*info_ppal)->discos)->sgte;
	else
		anterior->sgte = aux_discos->sgte;
	sem_post(&((*info_ppal)->semaforo));
	
	printf("Se perdio la conexion con el disco %s\n",aux_discos->id);
	log_error(log, (char *)aux_discos->id, "Message error: Se perdio la conexion con el disco %s", aux_discos->id);
	  
	if ((*info_ppal)->discos != NULL)
	{   //Re-Direcciono pedidos de discos caidos
		pedido *aux_pedidos;
		pedido *liberar_pedido;
		aux_pedidos = aux_discos->pedidos_start;
		while(aux_pedidos != NULL)
		{
			//saco el pedido del disco
			if (aux_pedidos->type == nipc_req_read)
			{
				mensaje.type = nipc_req_read;
				mensaje.len = 4;
				mensaje.payload.sector = aux_pedidos->sector;
				
				aux_sock = (*info_ppal)->sock_x_liberar;
				while(aux_sock != NULL && aux_sock->sector != aux_pedidos->sock)
					aux_sock = aux_sock->sgte;
				
				if (aux_sock == NULL)
				{
					uint8_t *id_disco;
					id_disco = distribuir_pedido_lectura(info_ppal,mensaje,aux_pedidos->sock);
					if(id_disco == (uint8_t*)"NODISK")
					{
						printf("No hay discos SINCRONIZADOS\n");
						break;//return (uint16_t) 1;
					}
					else
					{
						printf("Re-Distribuir pedido: %d en disco: %s\n",mensaje.payload.sector, id_disco);
						log_info(log, (char *)aux_discos->id , "Message info: Re-Distribuir pedido: %d en disco: %s", mensaje.payload.sector,id_disco);
					}
				}
			}
			aux_discos->cantidad_pedidos--;
			aux_discos->pedidos_start = aux_pedidos->sgte;
			liberar_pedido = aux_pedidos;
			free(liberar_pedido);
			aux_pedidos = aux_pedidos->sgte;
		}
	}
	else
	{  /** Enviar aviso a los PFS que se cerrara el sistema **/
		printf("No hay discos para redireccionar\n");
		(*info_ppal)->max_sector = 0;
		return 1;
	}
	free(aux_discos);
	aux_discos = (*info_ppal)->discos;
	while((aux_discos != NULL) && (estado == ESTADO_FAIL))
	{
		if(aux_discos->sector_sincro == (*info_ppal)->max_sector)
			estado = ESTADO_OK;
		aux_discos = aux_discos->sgte;
	}
	if (estado == ESTADO_FAIL)
	{
		(*info_ppal)->max_sector = 0;   // informa que no hay discos consistentes 
		printf("paso por aca\n");
	}
	return 0;
}

/**
 * Limpia pedidos de discos caidos
 *
 */
uint16_t limpio_pedidos(datos **info_ppal,nipc_socket sock_caido)
{
	disco *aux_discos;
	//sem_wait(&((*info_ppal)->semaforo));
	//sem_post(&((*info_ppal)->semaforo));
	aux_discos = (*info_ppal)->discos;
	while(aux_discos != NULL)
	{
		pedido *aux_pedidos;
		pedido *liberar_pedido;
		aux_pedidos = aux_discos->pedidos_start;
		while(aux_pedidos != NULL)
		{
			//saco el pedido del disco
			if (aux_pedidos->sock == sock_caido)
			{
				aux_discos->cantidad_pedidos--;
			}
			aux_discos->pedidos_start = aux_pedidos->sgte;
			liberar_pedido = aux_pedidos;
			free(liberar_pedido);
			aux_pedidos = aux_pedidos->sgte;
		}
		aux_discos = aux_discos->sgte;
	}
	return 0;
}

/**
 * Lee la configuracion
 *
 */
int config_read(config_t *config)
{
	char line[1024], w1[1024], w2[1024];
	FILE *fp;
	fp = fopen("./bin/praid.conf","r");
    if( fp == NULL )
	{
		printf("No se encuentra el archivo de configuración\n");
		return 1;
    }
    while( fgets(line, sizeof(line), fp) )
	{
		char* ptr;
		if( line[0] == '/' && line[1] == '/' )
			continue;
		if( (ptr = strstr(line, "//")) != NULL )
			*ptr = '\n'; //Strip comments
		if( sscanf(line, "%[^:]: %[^\t\r\n]", w1, w2) < 2 )
			continue;
		//Strip trailing spaces
		ptr = w2 + strlen(w2);
		while (--ptr >= w2 && *ptr == ' ');
			ptr++;
		*ptr = '\0';

		if(strcmp(w1,"host")==0)
			strcpy((char *)(config->server_host), w2);
		else 
			if(strcmp(w1,"puerto")==0)
			config->server_port = atoi(w2);
			else
				if(strcmp(w1,"flag_consola")==0)
				{
					if(strcmp(w1,"true") == 0)
						config->console = 1;
					else
						if(strcmp(w1,"false") == 0)
							config->console = 0;
				}
				else
				if(strcmp(w1,"log_path")==0)
					strcpy(config->log_path, w2);
				else
				if(strcmp(w1,"log_mode")==0)
				{
					if(strcmp(w2,"none") == 0)
						config->log_mode = LOG_OUTPUT_NONE;
					else
						if(strcmp(w2,"console") == 0)
							config->log_mode = LOG_OUTPUT_CONSOLE;
						else
							if(strcmp(w2,"file") == 0)
								config->log_mode = LOG_OUTPUT_FILE;
							else
							if(strcmp(w2,"file+console") == 0)
								config->log_mode = LOG_OUTPUT_CONSOLEANDFILE;
				}
				else
					printf("Configuracion desconocida:'%s'\n", w1);
	}
	fclose(fp);
	return 0;
}


/**
 * Valida que no alla dos discos con el mismo nombre
 *
 */
uint16_t validar_disco(datos *info_ppal, char * nombre_disco)
{
	sem_wait(&(info_ppal->semaforo));
	disco *aux = info_ppal->discos;
	while (aux != NULL && strcmp((char *)aux->id,nombre_disco)!=0)
		aux = aux->sgte;
	sem_post(&(info_ppal->semaforo));
	if (aux == NULL)
		return 0;
	else
		return 1;
}

