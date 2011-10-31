#include<stdio.h>
#include<unistd.h>
#include<string.h>
#include<pthread.h>
#include<sys/msg.h>
#include<sys/sem.h>
#include<stdlib.h>
#include<signal.h>
#include "praid_func.h"
#include "praid1.h"
#include "nipc.h"
#include "log.h"


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
  nuevo_disco->cantidad_pedidos = 0;
  nuevo_disco->pedidos = NULL;
  nuevo_disco->sgte = (*info_ppal)->discos;
  (*info_ppal)->discos = nuevo_disco;
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
		printf("Disco %s, Cantidad %d\n", aux_disco->id, aux_disco->cantidad_pedidos);
		pedido *aux_pedido;
		aux_pedido = aux_disco->pedidos;
		while(aux_pedido!=NULL)
		{
			printf("\ttype_pedido %d, Sector %d\n", aux_pedido->type, aux_pedido->sector);
			aux_pedido=aux_pedido->sgte;
		}
		aux_disco=aux_disco->sgte;
	}
}

/**
 * Obtiene la menor cantidad de pedidos de todos los discos
 *
 * @return menor cantidad de pedidos en discos
 */
uint32_t menor_cantidad_pedidos(disco *discos)
{
	disco *aux_disco;
	uint32_t menor_pedido=99999;
	aux_disco = discos;
	while(aux_disco != NULL)
	{
		if (menor_pedido > aux_disco->cantidad_pedidos)
			menor_pedido = aux_disco->cantidad_pedidos;
		aux_disco=aux_disco->sgte;
	}
	return menor_pedido;
}

/**
 * Distribulle un pedido de lectura de la cola de mensajes
 *
 */
uint8_t* distribuir_pedido_lectura(disco **discos, nipc_packet mensaje,nipc_socket sock_pfs)
{
	uint16_t encontrado = 0;
	uint32_t menor_pedido;
	menor_pedido =  menor_cantidad_pedidos(*discos);
	disco *aux;
	aux = *discos;
	while((aux != NULL) && encontrado == 0)
	{
		if (aux->cantidad_pedidos == menor_pedido)
			encontrado = 1;
		else
			aux = aux->sgte;
	}
	pedido *nuevo_pedido;
	nuevo_pedido = (pedido *)malloc(sizeof(pedido));
	nuevo_pedido->sock = sock_pfs;
	nuevo_pedido->type = mensaje.type;
	nuevo_pedido->sector = mensaje.payload.sector;
	strcpy((char *)nuevo_pedido->contenido,"");
	if (encontrado == 0)
	{
	  printf("QUE PASO!!!!");
	  aux=*discos;
	}
	nuevo_pedido->sgte = aux->pedidos;
	aux->pedidos = nuevo_pedido;
	aux->cantidad_pedidos++;
	
	
	if(send_socket(&mensaje,aux->sock)<=0)
	{
		printf("\nError send");
		exit(EXIT_FAILURE);
	}
	else
	printf("Lectura en disco: %s\n",aux->id);
	return aux->id;
}

/**
 * Distribulle un pedido de Escritura de la cola de mensajes
 *
 */
void distribuir_pedido_escritura(disco **discos, nipc_packet mensaje,nipc_socket sock_pfs)
{

	disco *aux;
	aux = *discos;
	while(aux != NULL)	
	{
		pedido *nuevo_pedido;
		nuevo_pedido = (pedido *)malloc(sizeof(pedido));
		nuevo_pedido->sock = sock_pfs;
		nuevo_pedido->type = mensaje.type;
		nuevo_pedido->sector = mensaje.payload.sector;
		strcpy((char *)nuevo_pedido->contenido,(char *)mensaje.payload.contenido);
		nuevo_pedido->sgte = aux->pedidos;
		aux->pedidos = nuevo_pedido;
		
		if(send_socket(&mensaje,aux->sock)<0)
		{
		  printf("\nError send");
		  exit(EXIT_FAILURE);
		}
		else
		  printf("Escritura en disco: %s\n",aux->id);
		
		
		aux=aux->sgte;
	}
}

/**
 * Recibe constantemente los pedidos que alla en la cola
 *
 */
void *espera_respuestas(datos **info_ppal)
{
  log_t*       log = log_new("./src/praid1/log.txt", "Runner", LOG_OUTPUT_FILE );
  nipc_packet  mensaje;
  pedido      *aux_pedidos;
  pedido      *anterior;
  disco       *el_disco;
  
  el_disco = (*info_ppal)->discos;
  while(el_disco != NULL && el_disco->hilo != pthread_self())
    el_disco = el_disco->sgte;
  
  while(1)
  {
    strcpy((char *)mensaje.buffer, "");
    if(recv_socket(&mensaje,el_disco->sock)>0)
    {
      //printf("El mensaje es: %d - %d - %d - %s\n",mensaje.type,mensaje.len,mensaje.payload.sector,mensaje.payload.contenido);
      if(mensaje.type != nipc_handshake)
      {
	if(mensaje.type == nipc_CHS)
	{
	  printf("Disco: %s - CHS: %s\n",el_disco->id,mensaje.payload.contenido);
	  log_info(log, (char *)el_disco->id, "Message info: Disco: %s - CHS: %s",el_disco->id,mensaje.payload.contenido);
	}
	else
	{      
	  if(mensaje.type == nipc_error)
	  {
	    printf("ERROR: %d - %s\n",mensaje.payload.sector,mensaje.payload.contenido);
	    log_error(log, (char *)el_disco->id, "Message error: Sector:%d Error: %s", mensaje.payload.sector,mensaje.payload.contenido);
	  }
	  if(mensaje.type == nipc_req_read)
	  {
	    printf("Recibida lectura del sector: %d - %s\n",mensaje.payload.sector,mensaje.payload.contenido);
	    log_info(log, (char *)el_disco->id, "Message info: Recibida lectura del sector: %d - %s", mensaje.payload.sector,mensaje.payload.contenido);
	  }
	  if(mensaje.type == nipc_req_write)
	  {
	    printf("Recibida escritura del sector: %d - %s\n",mensaje.payload.sector,mensaje.payload.contenido);
	    log_info(log, (char *)el_disco->id, "Message info: Recibida escritura del sector: %d - %s", mensaje.payload.sector,mensaje.payload.contenido);
	  }
	  
	  aux_pedidos=el_disco->pedidos;
	  anterior=NULL;
	  
	  while(aux_pedidos != NULL && (aux_pedidos->type != mensaje.type || aux_pedidos->sector != mensaje.payload.sector))
	  {
	    anterior=aux_pedidos;
	    aux_pedidos=aux_pedidos->sgte;
	  }
	  if(aux_pedidos == NULL)
	  {
	    printf("Sector no solicitado!!!\n");
	  }
	  else
	  {
	    printf("Envio de sector %d al PFS %d\n",mensaje.payload.sector,aux_pedidos->sock);
	    //enviar a quien hizo el pedido
	    //if(send_socket(mensaje,el_disco->sock)>0)
	  
	    if(anterior == NULL)
	    {
	      el_disco->pedidos = (el_disco->pedidos)->sgte;
	    }
	    else
	    {
	      anterior->sgte = aux_pedidos->sgte;
	    }
	    free(aux_pedidos);
	    el_disco->cantidad_pedidos--;
	  }
	}
      }
    }
    else
    {
      printf("Se perdio la conexion con el disco %s\n",(char *)el_disco->id);
      log_error(log, (char *)el_disco->id, "Message error: Se perdio la conexion con el disco %s", (char *)el_disco->id);
      nipc_close(el_disco->sock);
      el_disco->sock = -1;
      //mensaje trucho para avisar evento
      //mensaje.type = 9;
      //mensaje.len = 0;
      //mensaje.payload.sector = 0;
      //strcpy((char *)mensaje.payload.contenido,"");
      //if(send_socket(&mensaje,(*info_ppal)->sock_raid)<0)
	//printf("ERROR AVISO");
      
      //strcpy((char *)el_disco->id,"");
      break;
    }
    printf("------------------------------\n");
  }
  printf("------------------------------\n");
  pthread_exit(NULL);
}

/**
 * Limpia los PFS caidos
 *
 */
void liberar_pfs_caido(pfs **pedidos_pfs, nipc_socket sock)
{
  pfs *aux_pfs;
  pfs *anterior;
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
void limpio_discos_caidos(datos **info_ppal)
{
  log_t* log = log_new("./src/praid1/log.txt", "Runner", LOG_OUTPUT_FILE );
  
  //bloquear hasta una señal o semaforo
  
  nipc_packet mensaje;
  disco *aux_discos;
  disco *anterior = NULL;
  aux_discos = (*info_ppal)->discos;
  while(aux_discos != NULL)
  {
    if(aux_discos->sock == -1)
    {
      printf("Limpieza de un disco\n");
      //Saco disco de la lista
      if(anterior == NULL)
	(*info_ppal)->discos = ((*info_ppal)->discos)->sgte;
      else
	anterior->sgte = aux_discos->sgte;
      listar_pedidos_discos((*info_ppal)->discos);
      //if ((*info_ppal)->discos != NULL)
      printf("-------------------Limpieza disco---------------\n");
      //Re-Direcciono pedidos de discos caidos
      pedido *aux_pedidos;
      pedido *liberar_pedido;
      disco  *liberar_disco;
      aux_pedidos = aux_discos->pedidos;
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
	  id_disco = distribuir_pedido_lectura(&(*info_ppal)->discos,mensaje,aux_pedidos->sock);
	  printf("Re-Distribuir pedido: %d en disco: %s\n",mensaje.payload.sector, id_disco);
	  log_info(log, "Principal", "Message info: Re-Distribuir pedido: %d en disco: %s", mensaje.payload.sector,id_disco);
	  aux_discos->cantidad_pedidos--;
	}
	
	aux_discos->pedidos = aux_pedidos->sgte;
	liberar_pedido = aux_pedidos;
	free(liberar_pedido);
	aux_pedidos = aux_pedidos->sgte;
      }
      liberar_disco = aux_discos;
      free(liberar_disco);
    }
    anterior = aux_discos;
    aux_discos = aux_discos->sgte;
    
  }
}
