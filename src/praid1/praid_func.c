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
	disco *nuevo_disco;
	nuevo_disco = (disco *)malloc(sizeof(disco));
	memcpy(nuevo_disco->id,id_disco,strlen((char *)id_disco)+1);
	nuevo_disco->sock=sock_new;
	nuevo_disco->pedido_sincro = 0;
	nuevo_disco->sector_sincro = (int32_t) -1;
	nuevo_disco->ya_sincro_start = NULL;
	nuevo_disco->ya_sincro_end = NULL;
	nuevo_disco->cantidad_pedidos = 0;
	nuevo_disco->pedidos_start = NULL;
	nuevo_disco->pedidos_end = NULL;
	nuevo_disco->sgte = (*info_ppal)->discos;
	sem_wait(&((*info_ppal)->semaforo));
	(*info_ppal)->discos = nuevo_disco;
	sem_post(&((*info_ppal)->semaforo));
	if(pthread_create(&nuevo_disco->hilo,NULL,(void *)espera_respuestas,(void *)info_ppal)!=0)
		printf("Error en la creacion del hilo\n");
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
		aux_disco=aux_disco->sgte;
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
		printf("\tDisco %s, Cantidad %d, Sincronizado %d\n", aux_disco->id, aux_disco->cantidad_pedidos, aux_disco->sector_sincro);
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
	//printf("%d",sock_disco);
	//getchar();
	return sock_disco;
}

/**
 * Distribulle un pedido de lectura de la cola de mensajes
 *
 */
uint8_t* distribuir_pedido_lectura(datos **info_ppal, nipc_packet mensaje,nipc_socket sock_pfs)
{
	nipc_socket sock_disco;
	sem_wait(&((*info_ppal)->semaforo));
	sock_disco =  menor_cantidad_pedidos((*info_ppal)->discos,mensaje.payload.sector);
	//sock_disco =  uno_y_uno(info_ppal,mensaje.payload.sector);
	
	if (sock_disco != 0)
	{
		pedido *nuevo_pedido;
		nuevo_pedido = (pedido *)malloc(sizeof(pedido));
		nuevo_pedido->sock = sock_pfs;
		nuevo_pedido->type = mensaje.type;
		nuevo_pedido->sector = mensaje.payload.sector;
		strcpy((char *)nuevo_pedido->contenido,"");
		nuevo_pedido->sgte = NULL;
		
		disco *aux;
		aux = (*info_ppal)->discos;
		while((aux != NULL) && (aux->sock != sock_disco))
			aux = aux->sgte;
		
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
		aux->cantidad_pedidos++;
		
		sem_post(&((*info_ppal)->semaforo));
		
		if(send_socket(&mensaje,aux->sock)<=0)
		{
			//limpio_discos_caidos(info_ppal,aux->sock);
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
	while(aux_pfs != NULL  && aux_pfs->sock != sock_pfs)
	  aux_pfs = aux_pfs->sgte;
	sectores_t *nueva_escritura;
	nueva_escritura = (sectores_t *)malloc(sizeof(sectores_t));
	nueva_escritura->sector = mensaje.payload.sector;
	sem_wait(&(aux_pfs->semaforo));
	nueva_escritura->sgte = aux_pfs->escrituras;
	aux_pfs->escrituras = nueva_escritura;
	sem_post(&(aux_pfs->semaforo));
	
	disco *aux;
	aux = (*info_ppal)->discos;
	while(aux != NULL)	
	{
		pedido *nuevo_pedido;
		nuevo_pedido = (pedido *)malloc(sizeof(pedido));
		nuevo_pedido->sock = sock_pfs;
		nuevo_pedido->type = mensaje.type;
		nuevo_pedido->sector = mensaje.payload.sector;
		strcpy((char *)nuevo_pedido->contenido,(char *)mensaje.payload.contenido);
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
		uint32_t i;
		uint8_t *id_disco;
		
		mensaje.type=1;
		mensaje.len=4;
		printf("COMIENZA Pedido Sincronizacion %s\n", el_disco->id);
		for(i=0 ; i < (*info_ppal)->max_sector ; i++)
		{   
			if (el_disco->pedido_sincro == 2)
				break;
			mensaje.payload.sector = i;
			id_disco = distribuir_pedido_lectura(info_ppal , mensaje , el_disco->sock);
			
			if((i%10000)==0)
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
	
	//FILE * control = fopen("control.txt", "w");
	
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
	
	//recv_socket(&mensaje,el_disco->sock);
	uint32_t cant_sectores = 1024*1024;
	
	if ((*info_ppal)->discos->sgte != NULL)
	{  /** SINCRONIZAR **/
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
	else
	{    /** UNICO DISCO - MAESTRO **/
		el_disco->sector_sincro = cant_sectores;
		(*info_ppal)->max_sector = cant_sectores;
		printf("Discos de %d sectores\n",cant_sectores);
	}
	printf("------------------------------\n");
	
	int32_t recibido=0;
	while(1)
	{
		recibido = 0;
		recibido = recv_socket(&mensaje,el_disco->sock);
		if(recibido > 0)
		{
			//printf("----sock: %s------\n",el_disco->id);
			mensaje.payload.contenido[mensaje.len-4]='\0';
			//printf("El mensaje es: %d - %d - %d - %s\n",mensaje.type,mensaje.len,mensaje.payload.sector,mensaje.payload.contenido);
			//listar_pedidos_discos((*info_ppal)->discos);
			//printf("----------\n");
			
			if(mensaje.type != nipc_handshake)
			{
				if(mensaje.type == nipc_CHS)
				{/** nunca va a llegar aca **/
					printf("Disco: %s - CHS: %s\n",el_disco->id,mensaje.payload.contenido);
					log_info(log, (char *)el_disco->id, "Message info: Disco: %s - CHS: %s",el_disco->id,mensaje.payload.contenido);
				}
				else
				{      
					//sem_wait(&((*info_ppal)->semaforo));
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
						sem_wait(&((*info_ppal)->semaforo));
						disco *aux_disco;
						aux_disco = (*info_ppal)->discos;
						while(aux_disco != NULL  &&  aux_disco->sock != aux_pedidos->sock)
							aux_disco = aux_disco->sgte;
						
						if (aux_disco != NULL)//(aux_disco->sock == aux_pedidos->sock)
						{  /** es una sincronizacion de disco **/
							//printf("Envio de sector %d al PPD %s-%d\n",mensaje.payload.sector,aux_disco->id,aux_pedidos->sock);
							mensaje.type = 2;
							pedido *nuevo_pedido;
							nuevo_pedido = (pedido *)malloc(sizeof(pedido));
							nuevo_pedido->sock = -1;
							nuevo_pedido->type = mensaje.type;
							nuevo_pedido->sector = mensaje.payload.sector;
							strcpy((char *)nuevo_pedido->contenido,(char *)mensaje.payload.contenido);
							nuevo_pedido->sgte = NULL;
							if (aux_disco->pedidos_start == NULL)
							{
								aux_disco->pedidos_start = nuevo_pedido;
								aux_disco->pedidos_end   = nuevo_pedido;
							}
							else
							{
								aux_disco->pedidos_end->sgte = nuevo_pedido;
								aux_disco->pedidos_end       = nuevo_pedido;
							}
							aux_disco->cantidad_pedidos++;
							/** ENVIAR ESCRITURA A DISCO **/
							if(send_socket(&mensaje,aux_pedidos->sock)<0)
							{ // Aca viene el problema
								//se soluciona con SIGPIPE ignorar
							}
						}
						else
						{
							if(aux_pedidos->sock != -1)
							{   /** es una respuesta a una escritura de un PFS o sincronizacion **/
								if(mensaje.type == nipc_req_read)
								{
									//printf("Recibida lectura del sector: %d - %s\n",mensaje.payload.sector,mensaje.payload.contenido);
									log_info(log, (char *)el_disco->id, "Message info: Recibida lectura del sector: %d", mensaje.payload.sector);
									if(send_socket(&mensaje,aux_pedidos->sock)<=0)
									{
										//printf("\nError envio a PFS");
									}
								}
								if(mensaje.type == nipc_req_write)
								{
									//printf("Recibida escritura del sector: %d - %s\n",mensaje.payload.sector,mensaje.payload.contenido);
									log_info(log, (char *)el_disco->id, "Message info: Recibida escritura del sector: %d", mensaje.payload.sector);
									
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
											//printf("\nError envio a PFS");
										}
									}
									sem_post(&(aux_pfs->semaforo));	
								}
							}
							else
							{
								//char logbuff[10];
								//sprintf( logbuff, "%d\n",mensaje.payload.sector);
								//fprintf( control , "%s", logbuff );
								
								//log_info(log, (char *)el_disco->id, "Message info: control de llegada: %d ",mensaje.payload.sector);
								/** es una respuesta de sincronizacion **/
								if((el_disco->sector_sincro%20000)==0)
								{
									printf("Sincronizacion parcial: %d\n",el_disco->sector_sincro);
									listar_discos((*info_ppal)->discos);
								}
								if((el_disco->sector_sincro + 1) > mensaje.payload.sector)
								{  /** ya estaba sincronizado este sector **/
									printf("Sector YA ESTABA SINCRONIZADO en disco: %s - %d\n",el_disco->id,aux_pedidos->sock);
								}
								if((el_disco->sector_sincro + 1) == mensaje.payload.sector)
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
											if(el_disco->ya_sincro_start == NULL)
												break;
										}
									}
								}
								if((el_disco->sector_sincro + 1) < mensaje.payload.sector)
								{  /** se encola ordenadamente para llevar el control de los sectores sincronizados **/
									//printf("Sector encolado en disco: %s - %d\n",aux_disco->id,aux_pedidos->sock);
									insertar(&(el_disco->ya_sincro_start),mensaje.payload.sector);
									
									/*sectores_t *new_sector = (sectores_t *)malloc(sizeof(sectores_t));
									new_sector->sector = mensaje.payload.sector;
									new_sector->sgte = NULL;
									
									if (el_disco->ya_sincro_start == NULL)
									{
										el_disco->ya_sincro_start = new_sector;
										el_disco->ya_sincro_end   = new_sector;
									}
									else
									{
										el_disco->ya_sincro_end->sgte = new_sector;
										el_disco->ya_sincro_end       = new_sector;
									}*/
								}
								if((el_disco->sector_sincro + 1) == (*info_ppal)->max_sector)
								{
									log_time = time(NULL);
									log_tm = localtime( &log_time );
									strftime( str_time , 127 , "%H:%M:%S" , log_tm ) ;
									if (ftime (&tmili))
									{
										perror("[[CRITICAL]] :: No se han podido obtener los milisegundos\n");
										return 0;
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
						}
						else
						{
							anterior->sgte = aux_pedidos->sgte;
						}
						free(aux_pedidos);
						el_disco->cantidad_pedidos--;
						sem_post(&((*info_ppal)->semaforo));
					}
					else
					{ 
						printf("disco: %s - sector no solicitado: %d - %d - %d \n",el_disco->id, mensaje.type, mensaje.len, mensaje.payload.sector);
					}
					//printf("------------------------------\n");
					//listar_pedidos_discos((*info_ppal)->discos);
					//sem_post(&((*info_ppal)->semaforo));
				}
			}
		}
		else
		{
			el_disco->pedido_sincro = 2;
			usleep(10);
			limpio_discos_caidos(info_ppal,el_disco->sock);
			break;
		}
		//printf("------------------------------\n");
	}
	//fclose(control);
	printf("------------------------------\n");
	pthread_exit(NULL);
	return 0;
}

/**
 * Inserta sector sincronizado en la lista
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
void liberar_pfs_caido(lista_pfs **pedidos_pfs, nipc_socket sock)
{
	lista_pfs *aux_pfs;
	lista_pfs *anterior;
	aux_pfs = *pedidos_pfs;
	anterior=NULL;
	while(aux_pfs != NULL &&  aux_pfs->sock != sock)
	{
		anterior=aux_pfs;
		aux_pfs=aux_pfs->sgte;
	}
	if(anterior == NULL)
	{
		*pedidos_pfs = (*pedidos_pfs)->sgte;
	}
	else
	{
		anterior = anterior->sgte;
	}
	nipc_close(aux_pfs->sock);
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
	sem_wait(&((*info_ppal)->semaforo));
	aux_discos = (*info_ppal)->discos;
	while(aux_discos != NULL && aux_discos->sock != sock_ppd)
	{
		anterior = aux_discos;
		aux_discos = aux_discos->sgte;
	}
	//if(aux_discos->sock == sock_ppd) //Saco disco de la lista
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
				mensaje.type = aux_pedidos->type;
				mensaje.len = 4;
				mensaje.payload.sector = aux_pedidos->sector;
				strcpy((char *)mensaje.payload.contenido,(char *)aux_pedidos->contenido);
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
				aux_discos->cantidad_pedidos--;
			}
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
	disco *liberar_disco;
	liberar_disco = aux_discos;
	free(liberar_disco);
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
	/**   Sockets a liberar cuando termine el programa
	aux_sock = (*info_ppal)->sock_x_liberar;
	while(aux_sock != NULL)
	{
		printf("Eliminar: %d\n",aux_sock->sock);
		aux_sock = aux_sock->sgte;
	} **/
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