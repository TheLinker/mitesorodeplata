#include<sys/types.h>
#include<sys/ipc.h>
#include<sys/msg.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<stdint.h>


#define SIZEBUF 512

struct mensaje {
	uint32_t tipo_msg;
	uint32_t sector_msg;
};

struct pedid{
	uint8_t tipo;
	uint32_t sector;
	struct pedido *sgte;
};
typedef struct pedid pedido;

struct disc{
	uint32_t id_disco;
	uint32_t cantidad_pedidos;
	pedido *pedidos;
	struct disco *sgte;
};
typedef struct disc disco;
struct disco *discos;

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
	
	if(((&buf_msg)->sector_msg)==NULL){
		puts("No hay sector");
		exit(EXIT_FAILURE);
	}

	buf_msg.tipo_msg=1; //getpid();
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
	
	if(((&buf_msg)->sector_msg)==NULL){
		puts("No hay sector");
		exit(EXIT_FAILURE);
	}

	buf_msg.tipo_msg=2; //getpid();
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
			printf("\n\tTipo %d, Sector %d", aux_pedido->tipo, aux_pedido->sector);
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
		printf("\n\nLeyendo cola: %d",id_cola);
		printf("\nTipo mensaje: %d", (&buf_msg)->tipo_msg);
		printf("\nMensaje: %d", (&buf_msg)->sector_msg);

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
		nuevoPedido->tipo=(&buf_msg)->tipo_msg;
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
		printf("\n\nLeyendo cola: %d",id_cola);
		printf("\nTipo mensaje: %d", (&buf_msg)->tipo_msg);
		printf("\nMensaje: %d", (&buf_msg)->sector_msg);
		
		disco *aux;
		aux=discos;
		while(aux != NULL)	
		{
			pedido *nuevoPedido;
			nuevoPedido = (pedido *)malloc(sizeof(pedido));
			nuevoPedido->tipo = (&buf_msg)->tipo_msg;
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

msgqnum_t hayPedidosLectura()
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
msgqnum_t hayPedidosEscritura()
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




int main(int argc, char *argv[])
{
	char opcion;
	discos = NULL;
	
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
	printf("\n8. ");
	printf("\n9. Salir");
	
	printf("\nIngrese opcion: ");
	opcion = getchar();
	getchar();
	if(opcion == '1')
	{
		printf("\n	Agregar pedido de lectura");
		agregarPedidoLectura();
	}
	if(opcion == '2')
	{
		printf("\n	Agregar pedido de Escritura");
		agregarPedidoEscritura();
	}
	if(opcion == '3')
	{
		printf("\n	Agregar disco");
		agregarDisco();
	}
	if(opcion == '4')
	{
		printf("\n	Listar pedidos en discos");
		listarPedidosDiscos();
	}
	if(opcion == '5')
	{
		printf("\n	Distribuir Pedido Lectura");
		if (discos != NULL && hayPedidosLectura()!=0)
			distribuirPedidoLectura();
		else
			printf("\n\nNo hay discos o pedidos");
	}
	if(opcion == '6')
	{
		printf("\n	Distribuir Pedido Escritura");
		if (discos != NULL && hayPedidosEscritura()!=0)
			distribuirPedidoEscritura();
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
		printf("\n	");
		//listarPedidosDiscos();
	}
	if(opcion == '9')
	{
		printf("\n	Salir\n\n");
		eliminarCola();
		exit(EXIT_SUCCESS);
	}

}
}
