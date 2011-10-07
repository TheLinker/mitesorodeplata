//#include<sys/types.h>
//#include<sys/ipc.h>
//#include<string.h>
//#include<stdint.h>
#include<stdio.h>
#include<unistd.h>
#include<pthread.h>
#include<sys/msg.h>
#include<stdlib.h>
#include<signal.h>
#include "nipc.h"

#define SIZEBUF 1024

struct mensaje {
	uint32_t type_msg;
	uint32_t sector_msg;
};

typedef struct pedido{
	uint8_t type_pedido;
	uint32_t sector;
	struct pedido *sgte;
} __attribute__ ((packed)) pedido;

typedef struct disco{
	uint32_t id_disco;
	uint32_t cantidad_pedidos;
	pedido *pedidos;
	struct disco *sgte;
} __attribute__ ((packed)) disco;

struct disco *discos;

void agregarPedidoLectura();
void agregarPedidoEscritura();
void agregarDisco();
void listarPedidosDiscos();
uint32_t menorCantidadPedidos();
void distribuirPedidoLectura();
void distribuirPedidoEscritura();
uint32_t hayPedidosLectura();
uint32_t hayPedidosEscritura();
void estado();
void eliminarCola();
void *distribuirPedidos();

int main(int argc, char *argv[])
{
	char opcion;
	discos = NULL;
	pthread_t hilo1;
	
	while(1){
	printf("\n\n\n-------------------------------------");
	printf("\n-------------------------------------");
	printf("\n1. Agregar pedido lectura");
	printf("\n2. Agregar pedido escritura");
	printf("\n3. Agregar disco");
	printf("\n4. Listar pedidos en discos");
	printf("\n5. Distribuir Pedido Lectura");
	printf("\n6. Distribuir Pedido Escritura");
	printf("\n7. Estado de colas");
	printf("\n8. AUTO: distribuirPedidos");
	printf("\n9. Salir");
	
	printf("\nIngrese opcion: ");
	opcion = getchar();
	getchar();
	if(opcion == '1')
	{
		printf("\n	Agregar pedido de lectura");
		if (discos != NULL)
			agregarPedidoLectura();
		else
			printf("\n\nNo hay discos");		
	}
	if(opcion == '2')
	{
		printf("\n	Agregar pedido de Escritura");
		if (discos != NULL)
			agregarPedidoEscritura();
		else
			printf("\n\nNo hay discos");		
	}
	if(opcion == '3')
	{
		printf("\n	Agregar disco");
		agregarDisco(discos);
	}
	if(opcion == '4')
	{
		printf("\n	Listar pedidos en discos");
		listarPedidosDiscos(&discos);
	}
	if(opcion == '5')
	{
		printf("\n	Distribuir Pedido Lectura");
		if (discos != NULL && hayPedidosLectura(&discos)!=0)
			distribuirPedidoLectura(&discos);
		else
			printf("\n\nNo hay discos o pedidos");
	}
	if(opcion == '6')
	{
		printf("\n	Distribuir Pedido Escritura");
		if (discos != NULL && hayPedidosEscritura(&discos)!=0)
			distribuirPedidoEscritura(&discos);
		else
			printf("\n\nNo hay discos o pedidos");
	}
	if(opcion == '7')
	{
		printf("\n	Estado de cola");
		estado();
		
	}
	if(opcion == '8')
	{
		printf("\n	AUTO: distribuirPedidos");
		
		int ret1;
		ret1=pthread_create(&hilo1,NULL,distribuirPedidos,&discos);
		//pthread_join(hilo1,NULL);
		
	}
	if(opcion == '9')
	{
		printf("\n	Salir\n\n");
		eliminarCola(&discos);
		pthread_kill(hilo1,SIGKILL);
		
		exit(EXIT_SUCCESS);
	}

}
}

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
	
	if(((&buf_msg)->sector_msg)<0){
		perror("No hay sector");
		exit(EXIT_FAILURE);
	}

	buf_msg.type_msg=1; //getpid();
	size_msg=sizeof((&buf_msg)->sector_msg);

	if((msgsnd(id_cola,&buf_msg,size_msg,0))<0){
		perror("msgsnd"); 
		exit(EXIT_FAILURE);
	}else
		printf("\nMensaje publicado");
}

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
	
	if(((&buf_msg)->sector_msg)<0){
		puts("No hay sector");
		exit(EXIT_FAILURE);
	}

	buf_msg.type_msg=2; //getpid();
	size_msg=sizeof((&buf_msg)->sector_msg);

	if((msgsnd(id_cola,&buf_msg,size_msg,0))<0){
		perror("msgsnd"); 
		exit(EXIT_FAILURE);
	}else
		printf("\nMensaje publicado");
}

void agregarDisco()
{
	disco *nuevoDisco;
	
	nuevoDisco = (disco *)malloc(sizeof(disco));
	
	nuevoDisco->id_disco=12;
	nuevoDisco->cantidad_pedidos = 0;
	nuevoDisco->pedidos = NULL;
	nuevoDisco->sgte = discos;
	discos = nuevoDisco;
	
}

void listarPedidosDiscos()
{
	estado();

	disco *aux_disco;
	aux_disco=discos;
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

uint32_t menorCantidadPedidos()
{
	disco *aux;
	uint32_t menor_pedido=99999;
	aux = discos;

	while(aux != NULL)
	{
		if (menor_pedido > aux->cantidad_pedidos)
			menor_pedido = aux->cantidad_pedidos;
		aux=aux->sgte;
	}
	return menor_pedido;
}

void distribuirPedidoLectura()
{
	uint16_t encontrado = 0;
	uint32_t id_cola, size_msg, menorPedido;
	key_t clave = 111;
	struct mensaje buf_msg;
	menorPedido =  menorCantidadPedidos();
	
	if ((id_cola = msgget(clave, IPC_CREAT | 0666))<0){
			perror("msgget:create");
			exit(EXIT_FAILURE);
		}
	
	size_msg = msgrcv(id_cola, &buf_msg, SIZEBUF,0,0);
	
	if (size_msg>0)
	{
		//printf("\n\nLeyendo cola: %d",id_cola);
		//printf("\ntype_pedido mensaje: %d", (&buf_msg)->type_msg);
		//printf("\nMensaje: %d", (&buf_msg)->sector_msg);

		disco *aux;
		aux=discos;
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
			aux=discos;			
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


void distribuirPedidoEscritura()
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
		//printf("\n\nLeyendo cola: %d",id_cola);
		//printf("\ntype_pedido mensaje: %d", (&buf_msg)->type_msg);
		//printf("\nMensaje: %d", (&buf_msg)->sector_msg);
		
		disco *aux;
		aux=discos;
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

void estado()
{
	
	printf("\n\nMensajes de lectura = %d", hayPedidosLectura());
	printf("\nMensajes de Escritura = %d", hayPedidosEscritura());

}


void eliminarCola()
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
	aux = discos;
	while(aux != NULL)
	{
		discos=aux;
		aux=aux->sgte;
		free(discos);
	}
	free(aux);
}

void *distribuirPedidos()
{	while(1)
	{
		while(hayPedidosEscritura()!=0 || hayPedidosLectura()!=0)
		{
			if (hayPedidosEscritura()!=0)
				distribuirPedidoEscritura();
			else		
				if (hayPedidosLectura()!=0)
					distribuirPedidoLectura();
		}
		sleep(5);
	}
}
