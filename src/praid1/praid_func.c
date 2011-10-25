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
		printf("Mensaje publicado\n");
}

/**
 * Agrega un nuevo disco al RAID
 *
 */
void agregarDisco(disco **discos, uint8_t id_disco[20], nipc_socket sock_new)
{
	disco *nuevoDisco;
	nuevoDisco = (disco *)malloc(sizeof(disco));
	memcpy(nuevoDisco->id,id_disco,strlen((char *)id_disco));
	nuevoDisco->sock=sock_new;
	if(pthread_create(&nuevoDisco->hilo,NULL,(void *)esperaRespuestas,(void *)nuevoDisco->sock)!=0)
	     printf("Error en la creacion del hilo\n");
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
	nuevoPedido->type=mensaje.type;
	nuevoPedido->sector = mensaje.payload.sector;
	strcpy((char *)nuevoPedido->contenido,"");
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
	printf("Lectura en disco: %s\n",aux->id);
	
}

/**
 * Distribulle un pedido de Escritura de la cola de mensajes
 *
 */
void distribuirPedidoEscritura(disco **discos,nipc_packet mensaje)
{

	disco *aux;
	aux=*discos;
	while(aux != NULL)	
	{
		pedido *nuevoPedido;
		nuevoPedido = (pedido *)malloc(sizeof(pedido));
		nuevoPedido->type = mensaje.type;
		nuevoPedido->sector = mensaje.payload.sector;
		strcpy((char *)nuevoPedido->contenido,(char *)mensaje.payload.contenido);
		nuevoPedido->sgte = aux->pedidos;
		aux->pedidos = nuevoPedido;
		
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
	printf("Mensajes de lectura = %d\n", hayPedidosLectura());
	printf("Mensajes de Escritura = %d\n", hayPedidosEscritura());
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
void *esperaRespuestas(nipc_socket sock)
{
  nipc_packet mensaje;
  while(1)
  {
    if(recv_socket(&mensaje,sock)>0)
    {
      if(mensaje.len == 0)
      {
	printf("Se perdio la conexion con el disco\n");
	nipc_close(sock);
	break;
      }
      else
      {
	printf("El mensaje recivido es: %c - %d - %d - %s\n",mensaje.type,mensaje.len,mensaje.payload.sector,mensaje.payload.contenido);
	uint32_t id_cola, size_msg;
	key_t clave = 111;
	struct mensajeCola buf_msg;
	if ((id_cola = msgget(clave, IPC_CREAT | 0666))<0){
		perror("msgget:create");
		exit(EXIT_FAILURE);
	}
	buf_msg.type=mensaje.type;
	(&buf_msg)->sector = mensaje.payload.sector;
	memcpy(((&buf_msg)->contenido), mensaje.payload.contenido,strlen((char *)mensaje.payload.contenido));
	
	size_msg=516;
	if((msgsnd(id_cola,&buf_msg,size_msg,0))<0){
		perror("msgsnd"); 
		exit(EXIT_FAILURE);
	}else
		printf("Mensaje publicado\n");
      }
    }
    else
    {
      printf("Se perdio la conexion con el disco\n");
      nipc_close(sock);
      break;
    }
    printf("------------------------------\n");
  }
  printf("------------------------------\n");
  pthread_exit(NULL);
}

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


pthread_t crear_hilo_respuestas()
{
  pthread_t hilo;
  if(pthread_create(&hilo,NULL,(void *)enviar_respuetas,NULL)!=0)
    printf("Error en la creacion del hilo Respuetas!\n");
  return hilo;
}

void *enviar_respuetas(void *nada)
{
  
  return NULL;
  /*
   * Suspendemos en busqueda de <semaphore.h>
   * 
  uint32_t id_semaforo;
  struct sembuf buffer;
  union semun arg;
  
  id_semaforo = semget(IPC_PRIVATE,1,0666);
  if (id_semaforo<0)
  {
    perror("semget");
    exit(EXIT_FAILURE);
  }
  printf("Semaforo Respuestas: %d", id_semaforo);
  buffer.sem_num = 0;
  buffer.sem_op = -1;
  buffer.sem_flg = IPC_NOWAIT;
  if( semop(id_semaforo,&buffer,1) < 0)
  {
    perror("semop");
    exit(EXIT_FAILURE);
  }
  
  while(1)
  {
    printf("se desbloqueoooooo");
    
    //envio las respuestas!!!
    
    //lo bloqueo de nuevo
    buffer.sem_num = 0;
    buffer.sem_op = -1;
    buffer.sem_flg = IPC_NOWAIT;
    if( semop(id_semaforo,&buffer,1) < 0)
    {
      perror("semop");
      exit(EXIT_FAILURE);
    }
  }
  */
}
