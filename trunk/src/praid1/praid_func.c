#include<stdio.h>
#include<unistd.h>
#include<string.h>
#include<pthread.h>
#include<sys/msg.h>
#include<stdlib.h>
#include<signal.h>
#include "praid_func.h"
#include "praid1.h"


/**
 * Crea un nuevo pedido de lectura
 *
 */
void agregarPedidoLectura()
{
	uint32_t id_cola, size_msg;
	key_t clave = 111;
	struct mensaje buf_msg;
	if ((id_cola = msgget(clave, IPC_CREAT | 0666))<0){
			perror("msgget:create");
			exit(EXIT_FAILURE);
		}
	(&buf_msg)->sector_msg = 64;
	printf("\n\nIngrese Sector: %d",(&buf_msg)->sector_msg );
	//scanf("%d",&(&buf_msg)->sector_msg);
	buf_msg.type_msg=1; //getpid();
	size_msg=sizeof((&buf_msg)->sector_msg);
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
	struct mensaje buf_msg;
	if ((id_cola = msgget(clave, IPC_CREAT | 0666))<0){
			perror("msgget:create");
			exit(EXIT_FAILURE);
		}
	(&buf_msg)->sector_msg = 32;
	printf("\n\nIngrese Sector: %d",(&buf_msg)->sector_msg );
	//scanf("%d",&(&buf_msg)->sector_msg);
	buf_msg.type_msg=2; //getpid();
	size_msg=sizeof((&buf_msg)->sector_msg);

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
void agregarDisco(disco **discos)
{
	disco *nuevoDisco;
	nuevoDisco = (disco *)malloc(sizeof(disco));
	nuevoDisco->id_disco=12;
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
		printf("\n\nDisco %d, Cantidad %d", aux_disco->id_disco, aux_disco->cantidad_pedidos);
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
void distribuirPedidoLectura(disco **discos)
{
	uint16_t encontrado = 0;
	uint32_t id_cola, size_msg, menorPedido;
	key_t clave = 111;
	struct mensaje buf_msg;
	menorPedido =  menorCantidadPedidos(*discos);
	if ((id_cola = msgget(clave, IPC_CREAT | 0666))<0){
			perror("msgget:create");
			exit(EXIT_FAILURE);
		}
	size_msg = msgrcv(id_cola, &buf_msg, SIZEBUF,0,0);
	if (size_msg>0)
	{
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
		nuevoPedido->type_pedido=(&buf_msg)->type_msg;
		nuevoPedido->sector = (&buf_msg)->sector_msg;

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
	}
	else
	{
		perror("msgrcv");
		exit(EXIT_FAILURE);
	}
}

/**
 * Distribulle un pedido de Escritura de la cola de mensajes
 *
 */
void distribuirPedidoEscritura(disco **discos)
{
	uint32_t id_cola, size_msg;
	key_t clave = 222;
	struct mensaje buf_msg;
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
			nuevoPedido->type_pedido = (&buf_msg)->type_msg;
			nuevoPedido->sector = (&buf_msg)->sector_msg;
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
 *
void *distribuirPedidos(disco **discos)
{
	while(hayPedidosEscritura()!=0 || hayPedidosLectura()!=0)
	{
		if (hayPedidosEscritura()!=0)
			distribuirPedidoEscritura(&discos);
		else		
			if (hayPedidosLectura()!=0)
				distribuirPedidoLectura(&discos);
	}
}*/
