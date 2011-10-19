#include<string.h>
#include<stdio.h>
//nose que pasa
#include<stdlib.h>
#include<unistd.h>
#include<pthread.h>
#include<sys/msg.h>
#include<signal.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>

#include "sock.h"
#include "praid1.h"
#include "praid_func.h"

#define SOCKET_MAX_BUFFER 100

int main(int argc, char *argv[])
{
  disco *discos=NULL;
  fd_set set_socket;
  nipc_socket *sock_raid;
  nipc_socket sock_ppd;
  
    
  sock_raid = createSocket("127.0.0.1",50003);
  
  
  printf("\n\n pasooo   %d\n\n",*sock_raid);
  
  getchar();
  int max_sock = *sock_raid;
  
  while(sock_raid >0)
  {
    FD_ZERO(&set_socket);
    nipc_packet mensaje;
    struct sockaddr_in *addr_ppd = malloc(sizeof(struct sockaddr_in));
    int clilen;
    
    FD_SET(*sock_raid, &set_socket);
    select(max_sock+1, &set_socket, NULL, NULL, NULL);
    
    if(FD_ISSET(*sock_raid, &set_socket)>0)
    {
      if( (sock_ppd=accept(*sock_raid,(struct sockaddr *)addr_ppd,(void *)&clilen)) <0)
	printf("\nERROR en la conexion  %d\n",sock_ppd);
      else
      {
	uint32_t fin;
	fin=recv(sock_ppd,mensaje.buffer,sizeof(mensaje.buffer),0);
	//printf("\nNueva conexion TYPE: %c",mensaje.type);
	//printf("\nNueva conexion LEN: %d",mensaje.len);
	//printf("\nNueva conexion PAYLOAD: %s",mensaje.payload);
	
	if(mensaje.type == nipc_handshake)
	{
	  if(mensaje.len != 0)
	  {
	    printf("\nNueva conexion PPD: %s",mensaje.payload);
	    agregarDisco(&discos);
	    listarPedidosDiscos(&discos);
	    printf("\n-------------------------------");
	    printf("\n-------------------------------");
	    printf("\n-------------------------------");
	    FD_SET(sock_ppd, &set_socket);
	  }
	  else
	  {
	    printf("\nNueva conexion PFS: %d",sock_ppd);
	  }
	}
	if(mensaje.type == nipc_req_read)
	{
	  if(mensaje.payload != '\0')
	  {
	    printf("\nRespues lectura PPD: %d",sock_ppd);
	    agregarDisco(&discos);
	  }
	  else
	  {
	    printf("\nRespues lectura PPD: %d",sock_ppd);
	  }
	}
      }
      
    }
  }
   exit(EXIT_SUCCESS);
}



/*
int main(int argc, char *argv[])
{
	disco *discos;
	char opcion;
	discos = NULL;
	//pthread_t hilo1;
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
			if (discos != NULL)
			{
			  agregarPedidoLectura();
			  distribuirPedidoLectura(&discos);
			}else
				printf("\n\nNo hay discos");		
		}
		if(opcion == '2')
		{
			printf("\n	Agregar pedido de Escritura");
			if (discos != NULL)
			{
			  agregarPedidoEscritura();
			  distribuirPedidoEscritura(&discos);
			}else
				printf("\n\nNo hay discos");		
		}
		if(opcion == '3')
		{
			printf("\n	Agregar disco");
			agregarDisco(&discos);
		}
		if(opcion == '4')
		{
			printf("\n	Listar pedidos en discos");
			listarPedidosDiscos(&discos);
		}
		if(opcion == '5')
		{
			printf("\n	Distribuir Pedido Lectura");
			if (discos != NULL && hayPedidosLectura()!=0)
				distribuirPedidoLectura(&discos);
			else
				printf("\n\nNo hay discos o pedidos");
		}
		if(opcion == '6')
		{
			printf("\n	Distribuir Pedido Escritura");
			if (discos != NULL && hayPedidosEscritura()!=0)
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
			printf("\n	");
			//distribuirPedidos(&discos);
			//int ret1;
			//ret1=pthread_create(&hilo1,NULL,distribuirPedidos,(void *)&discos);
			//pthread_join(hilo1,NULL);
		}
		if(opcion == '9')
		{
			printf("\n	Salir\n\n");
			eliminarCola(&discos);
			//pthread_kill(hilo1,SIGKILL);
			exit(EXIT_SUCCESS);
		}
	}
}
*/