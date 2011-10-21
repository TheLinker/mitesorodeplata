#include<string.h>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<pthread.h>
#include<sys/msg.h>
#include<signal.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include "../common/nipc.h"
#include "praid1.h"
#include "praid_func.h"

int main(int argc, char *argv[])
{
  disco *discos=NULL;
  pfs *pedidos_pfs=NULL;
  pfs *aux_pfs;
  fd_set set_socket;
  nipc_socket sock_raid,sock_new;
  sock_raid = create_socket("127.0.0.1",50003);
  
  printf("\n Socket RAID: %d\n\n",sock_raid);
  
  uint32_t max_sock;
  nipc_packet mensaje;
  struct sockaddr_in *addr_ppd = malloc(sizeof(struct sockaddr_in));
  uint32_t clilen;  
  
  while(1)
  {
    FD_ZERO(&set_socket);
        
    FD_SET(sock_raid, &set_socket);
    
    max_sock = sock_raid;
    
    aux_pfs=pedidos_pfs;
    while(aux_pfs!=NULL)
    {
      if(aux_pfs->sock > max_sock)
	max_sock = aux_pfs->sock;
      FD_SET (aux_pfs->sock, &set_socket);
      aux_pfs=aux_pfs->sgte;
    }
    
    select(max_sock+1, &set_socket, NULL, NULL, NULL);  
      
    aux_pfs=pedidos_pfs;
    while(aux_pfs != NULL)
    {
      if(FD_ISSET(aux_pfs->sock, &set_socket)>0)
      {
	//printf("\nYa esta en la conexion  %d\n",sock_new);
//--------------	
	if(recv_socket(&mensaje,sock_new)>0)
	{
	  if(mensaje.type == nipc_handshake)
	    printf("\nBASTA DE HANDSHAKE!!!!\n");
	  if(mensaje.type == nipc_req_read)
	  {
	    if(mensaje.len == 4)
	    {
	      printf("\nPedido de lectura del FS: %d",mensaje.payload.sector);
	      distribuirPedidoLectura(&discos,mensaje);
	      printf("\n-------------------------------");
	    }
	    else
	    {
	      printf("\nMANDASTE CUALQUIER COSA");
	    }
	  }
	  if(mensaje.type == nipc_req_write)
	  {
	    if(mensaje.len != 4)
	    {
	      printf("\nPedido de escritura del FS: %d - %s",mensaje.payload.sector,mensaje.payload.contenido);
	      //distribuirPedidoEscritura(&discos);
	      printf("\n-------------------------------");
	    }
	    else
	    {
	      printf("\nnMANDASTE CUALQUIER COSA");
	    }
	  }
	  if(mensaje.type == nipc_error)
	  {
	    printf("\nERROR: %d - %s",mensaje.payload.sector,mensaje.payload.contenido);
	  }
	}
	else
	{
	  //murio el socket
	  pfs *anterior=NULL;
	  aux_pfs=pedidos_pfs;
	  while(aux_pfs != NULL &&  aux_pfs->sock != sock_new)
	  {
	    anterior=aux_pfs;
	    aux_pfs=aux_pfs->sgte;
	  }
	  if(anterior==NULL)
	    pedidos_pfs = pedidos_pfs->sgte;
	  else
	    anterior->sgte=anterior->sgte->sgte;
	  free(aux_pfs);
	  
	}
	
//	----------------
      }
      aux_pfs = aux_pfs->sgte;
    }
    
    if(FD_ISSET(sock_raid, &set_socket)>0)
    {
      if( (sock_new=accept(sock_raid,(struct sockaddr *)addr_ppd,(void *)&clilen)) <0)
	printf("\nERROR en la conexion  %d\n",sock_new);
      else
      if(recv_socket(&mensaje,sock_new)>0)
      {
	if(mensaje.type == nipc_handshake)
	{
	  if(mensaje.len != 0)
	  {
	    printf("\nNueva conexion PPD: %s",mensaje.payload.contenido);
	    char id_disco[20];
	    memcpy(&id_disco,mensaje.payload.contenido,20);
	    agregarDisco(&discos,id_disco,sock_new);//crea hilo
	    //listarPedidosDiscos(&discos);
	    
	    printf("\n-------------------------------");
	  }
	  else
	  {
	    if(discos!=NULL)
	    {
	      printf("\nNueva conexion PFS: %d",sock_new);
	      pfs *nuevo_pfs;
	      nuevo_pfs = (pfs *)malloc(sizeof(pfs));
	      nuevo_pfs->sock=sock_new;
	      nuevo_pfs->sgte = pedidos_pfs;
	      pedidos_pfs = nuevo_pfs;
	      FD_SET (nuevo_pfs->sock, &set_socket);
	    }
	    else
	    {
	      //ENVIAR ERROR DE CONEXION
	      printf("No hay discos conectados!");
	    }
	  }
	}
	if(mensaje.type == nipc_req_read)
	{
	  if(mensaje.len == 4)
	  {
	    printf("\nPedido de lectura del FS: %d",mensaje.payload.sector);
	    //agregarPedidoLectura();
	    //distribuirPedidoLectura(&discos);
	    
	    printf("\n-------------------------------");
	  }
	  else
	  {
	    printf("\nRespuesta lectura: %s",mensaje.payload.contenido);
	  }
	}
	if(mensaje.type == nipc_req_write)
	{
	  if(mensaje.len != 4)
	  {
	    printf("\nPedido de escritura del FS: %d - %s",mensaje.payload.sector,mensaje.payload.contenido);
	    //agregarPedidoEscritura();
	    //distribuirPedidoEscritura(&discos);
	    
	    printf("\n-------------------------------");
	  }
	  else
	  {
	    printf("\nRespuesta lectura: %d",mensaje.payload.sector);
	  }
	}
	if(mensaje.type == nipc_error)
	{
	  printf("\nERROR: %d - %s",mensaje.payload.sector,mensaje.payload.contenido);
	}
      }
    }
  }
  printf("\n\n POR ALGO SALIII \n\n");
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