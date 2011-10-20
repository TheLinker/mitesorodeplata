#include<stdio.h>
#include<unistd.h>
#include<string.h>
#include<pthread.h>
#include<sys/msg.h>
#include<stdlib.h>
#include<signal.h>
#include "praid_func.h"
#include "praid1.h"
#include "../common/nipc.h"

/**
 * Crea un nuevo pedido de lectura
 *
 */
void agregarPedidoLectura()
{
	uint32_t id_cola, size_msg;
	key_t clave = 111;
	struct mensajeCola buf_msg;
	if ((id_cola = msgget(clave, IPC_CREAT | 0666))<0){
			perror("msgget:create");
			exit(EXIT_FAILURE);
		}
	(&buf_msg)->sector = 64;
	printf("\n\nIngrese Sector: %d",(&buf_msg)->sector );
	//scanf("%d",&(&buf_msg)->sector_msg);
	buf_msg.type=1; //getpid();
	size_msg=sizeof((&buf_msg)->sector);
	if((msgsnd(id_cola,&buf_msg,size_msg,0))<0){
		perror("msgsnd"); 
		exit(EXIT_FAILURE);
	}else
		printf("\nMensaje publicado");
}

/**
 * Crea un nuevo pedido de Escritura
 *
 */
void agregarPedidoEscritura()
{
	uint32_t id_cola, size_msg;
	key_t clave = 222;
	struct mensajeCola buf_msg;
	if ((id_cola = msgget(clave, IPC_CREAT | 0666))<0){
			perror("msgget:create");
			exit(EXIT_FAILURE);
		}
	(&buf_msg)->sector = 32;
	printf("\n\nIngrese Sector: %d",(&buf_msg)->sector );
	//scanf("%d",&(&buf_msg)->sector_msg);
	buf_msg.type=2; //getpid();
	size_msg=sizeof((&buf_msg)->sector);

	if((msgsnd(id_cola,&buf_msg,size_msg,0))<0){
		perror("msgsnd"); 
		exit(EXIT_FAILURE);
	}else
		printf("\nMensaje publicado");
}

/**
 * Agrega un nuevo disco al RAID
 *
 */
void agregarDisco(disco **discos, char id_disco[20], uint32_t sock_new)
{
	disco *nuevoDisco;
	nuevoDisco = (disco *)malloc(sizeof(disco));
	memcpy(nuevoDisco->id,id_disco,strlen((char *)id_disco));
	nuevoDisco->sock=sock_new;
	if(pthread_create(&nuevoDisco->hilo,NULL,(void *)esperaRespuestas,(void *)nuevoDisco->sock)!=0)
	     printf("Error en la creacion del hilo");
	nuevoDisco->cantidad_pedidos = 0;
	nuevoDisco->pedidos = NULL;
	nuevoDisco->sgte = *discos;
	*discos = nuevoDisco;

}

/**
 * Lista todos los discos y sus pedidos asignados
 *
 */
void listarPedidosDiscos(disco **discos)
{
	estado();
	disco *aux_disco;
	aux_disco=*discos;
	while(aux_disco != NULL)	
	{
		printf("\n\nDisco %s, Cantidad %d", aux_disco->id, aux_disco->cantidad_pedidos);
		pedido *aux_pedido;
		aux_pedido = aux_disco->pedidos;	
		while(aux_pedido!=NULL)
		{
			printf("\n\ttype_pedido %d, Sector %d", aux_pedido->type_pedido, aux_pedido->sector);
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
uint32_t menorCantidadPedidos(disco *discos)
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
void distribuirPedidoLectura(disco **discos,nipc_packet mensaje)
{
	uint16_t encontrado = 0;
	uint32_t menorPedido;
	menorPedido =  menorCantidadPedidos(*discos);
	disco *aux;
	aux=*discos;
	while((aux != NULL) && encontrado == 0)
	{
		if (aux->cantidad_pedidos == menorPedido)
			encontrado = 1;
		else
			aux = aux->sgte;
	}
	pedido *nuevoPedido;
	nuevoPedido = (pedido *)malloc(sizeof(pedido));
	nuevoPedido->type_pedido=mensaje.type;
	nuevoPedido->sector = mensaje.payload.sector;
	if (encontrado==1)
	{
		nuevoPedido->sgte = aux->pedidos;
		aux->pedidos = nuevoPedido;
		aux->cantidad_pedidos++;
	}
	else
	{
		aux=*discos;
		nuevoPedido->sgte = aux->pedidos;
		aux->pedidos = nuevoPedido;
		aux->cantidad_pedidos++;
	}
	
	if(send_socket(&mensaje,aux->sock)<0)
	{
		printf("\nError send");
		exit(EXIT_FAILURE);
	}
	else
	printf("\nLectura en disco: %s",aux->id);
	
}

/**
 * Distribulle un pedido de Escritura de la cola de mensajes
 *
 */
void distribuirPedidoEscritura(disco **discos)
{
	uint32_t id_cola, size_msg;
	key_t clave = 222;
	struct mensajeCola buf_msg;
	if ((id_cola = msgget(clave, IPC_CREAT | 0666))<0){
			perror("msgget:create");
			exit(EXIT_FAILURE);
		}
	size_msg = msgrcv(id_cola, &buf_msg, sizeof(uint32_t),0,0);
	if (size_msg>0)
	{
		disco *aux;
		aux=*discos;
		while(aux != NULL)	
		{
			pedido *nuevoPedido;
			nuevoPedido = (pedido *)malloc(sizeof(pedido));
			nuevoPedido->type_pedido = (&buf_msg)->type;
			nuevoPedido->sector = (&buf_msg)->sector;
			nuevoPedido->sgte = aux->pedidos;
			aux->pedidos = nuevoPedido;
			aux=aux->sgte;
		}
	}
	else
	{
		perror("msgrcv");
		exit(EXIT_FAILURE);
	}
}

/**
 * Obtiene la cantidad de mensajes de lectura en la cola
 *
 * @return cantidad de mensajes de Lectura
 */
uint32_t hayPedidosLectura()
{
	uint32_t id_cola;
	key_t clave =111;
	struct msqid_ds cola;
	if ((id_cola = msgget(clave,IPC_CREAT |  0666))<0){
			perror("msgget:create");
			exit(EXIT_FAILURE);
		}
	if((msgctl(id_cola,IPC_STAT,&cola))<0){
		perror("msgctl");
		exit(EXIT_FAILURE);
	}
	return cola.msg_qnum;
}

/**
 * Obtiene la cantidad de mensajes de Escritura en la cola
 *
 * @return cantidad de mensajes de Escritura
 */
uint32_t hayPedidosEscritura()
{
	uint32_t id_cola;
	key_t clave =222;
	struct msqid_ds cola;
	if ((id_cola = msgget(clave,IPC_CREAT |  0666))<0){
			perror("msgget:create");
			exit(EXIT_FAILURE);
		}
	if((msgctl(id_cola,IPC_STAT,&cola))<0){
		perror("msgctl");
		exit(EXIT_FAILURE);
	}
	return cola.msg_qnum;
}

/**
 * Imprime cantidad de mensaje de las colas
 *
 */
void estado()
{
	printf("\n\nMensajes de lectura = %d", hayPedidosLectura());
	printf("\nMensajes de Escritura = %d", hayPedidosEscritura());
}

/**
 * Elimina las colas de mensajes y libera la memoria de los discos
 *
 */
void eliminarCola(disco **discos)
{
	uint32_t id_cola;
	key_t clave =111;
	if ((id_cola = msgget(clave,IPC_CREAT |  0666))<0){
			perror("msgget:create");
			exit(EXIT_FAILURE);
		}
	if((msgctl(id_cola,IPC_RMID,NULL))<0){
		perror("msgctl");
		exit(EXIT_FAILURE);
	}
	clave =222;
	if ((id_cola = msgget(clave,IPC_CREAT |  0666))<0){
			perror("msgget:create");
			exit(EXIT_FAILURE);
		}
	if((msgctl(id_cola,IPC_RMID,NULL))<0){
		perror("msgctl");
		exit(EXIT_FAILURE);
	}
	disco *aux;
	aux = *discos;
	while(aux != NULL)
	{
		*discos=aux;
		aux=aux->sgte;
		free(*discos);
	}
	free(aux);
}

/**
 * Distriubuye constantemente los pedidos que alla en la cola
 *
 */
void *esperaRespuestas(uint32_t sock)
{
  nipc_packet mensaje;
  while(1)
  {
    if(recv_socket(&mensaje,sock)>0)
    {
      printf("\n\nEl mensaje recivido es: %c - %d - %d - %s",mensaje.type,mensaje.len,mensaje.payload.sector,mensaje.payload.contenido);
      uint32_t id_cola, size_msg;
      key_t clave = 111;
      struct mensajeCola buf_msg;
      if ((id_cola = msgget(clave, IPC_CREAT | 0666))<0){
	      perror("msgget:create");
	      exit(EXIT_FAILURE);
      }
      buf_msg.type=mensaje.type;
      (&buf_msg)->sector = mensaje.payload.sector;
      memcpy((&buf_msg)->contenido, mensaje.payload.contenido,strlen(mensaje.payload.contenido));
      
      size_msg=516;
      if((msgsnd(id_cola,&buf_msg,size_msg,0))<0){
	      perror("msgsnd"); 
	      exit(EXIT_FAILURE);
      }else
	      printf("\nMensaje publicado");
    }
  }
  
  return (void *)1;
}
