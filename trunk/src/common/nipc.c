#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "nipc.h"

#define MAX_CONECTIONS 20

// Errores
#define HANDSHAKE_OK     0
#define NO_DISKS         1
#define ALREADY_CONNECT  2
#define CONNECTION_FAIL  3
#define DISKS_FAILED     4
#define NO_FOUND_SECTOR  5
#define WRITE_FAIL       6
#define READ_FAIL        7

/**
 * Inicia un socket para escuchar nuevas conexiones
 *
 * @nipc_socket descriptor del socket del protocolo NIPC
 * @return el descriptor del socket creado
 */
nipc_socket create_socket(char *host, uint16_t port)
{
    nipc_socket sock_new;
    struct sockaddr_in addr_raid;
    uint32_t i = 1;
    if ((sock_new = socket(AF_INET,SOCK_STREAM,0))<0)
    {
      printf("Error socket");
      exit(EXIT_FAILURE);
    }
    if (setsockopt(sock_new, SOL_SOCKET, SO_REUSEADDR, &i,sizeof(sock_new)) < 0)
    {
      printf("\nError: El socket ya esta siendo utilizado...");
      exit(EXIT_FAILURE);
    }
    addr_raid.sin_family = AF_INET;
    addr_raid.sin_port=htons(port);
    addr_raid.sin_addr.s_addr=inet_addr(host);
    if (bind(sock_new,(struct sockaddr *)&addr_raid,sizeof(struct sockaddr_in))<0)
    {
      printf("Error bind");
      exit(EXIT_FAILURE);
    }
    
    return sock_new;
}

/**
 * Recibe un paquete del socket indicado
 *
 * @return cantidad de caracteres recividos, en caso de ser 0 informa que el socket ha sido cerrado
 */
uint32_t recv_socket(nipc_packet *packet, nipc_socket sock)
{
    uint32_t   leido = 0;
    uint32_t   aux = 0;
    char       buffer[1024];
    
    //* Comprobacion de que los parametros de entrada son correctos
    if ((sock == -1) || (packet == NULL))
	return -1;
    //* Mientras no hayamos leido todos los datos solicitados
    aux = recv (sock, buffer , 3 ,0);
    memcpy(&packet->type, buffer     , 1 );
    memcpy(&packet->len , buffer + 1 , 2 );
    //leido = leido + aux;
    while (leido < (0 + packet->len))
    {
	aux = recv (sock, buffer + leido, (0 + packet->len) - leido,0);
	if (aux > 0)
	{
		//* Si hemos conseguido leer datos, incrementamos la variable
		//* que contiene los datos leidos hasta el momento
		leido = leido + aux;
	}
	else
	{
	    //* Si read devuelve 0, es que se ha cerrado el socket. Devolvemos
	    //* los caracteres leidos hasta ese momento
	    if (aux == 0) 
	    {
	      return leido;
	    }
	    if (aux == -1)
	    {
		/*
		* En caso de error, la variable errno nos indica el tipo
		* de error. 
		* El error EINTR se produce si ha habido alguna
		* interrupcion del sistema antes de leer ningun dato. No
		* es un error realmente.
		* El error EGAIN significa que el socket no esta disponible
		* de momento, que lo intentemos dentro de un rato.
		* Ambos errores se tratan con una espera de 100 microsegundos
		* y se vuelve a intentar.
		* El resto de los posibles errores provocan que salgamos de 
		* la funcion con error.
		*/
		switch (errno)
		{
		    case EINTR:
		    case EAGAIN:
			    usleep(100);
			    break;
		    default:
			    return -1;
		}
	    }
	}
    }
    packet->payload.sector = 0;
    //borrar contenido
    uint32_t i=0;
    for(i=0; i < 512;i++)
    {
      packet->payload.contenido[i]='\0';
      printf("%c",packet->payload.contenido[i]);
    }
    switch (packet->type)
    {
      case nipc_handshake:
	printf("HANDSHAKE\n");
	memcpy(&packet->payload.sector   , buffer    , 4 + 1);
	memcpy(&packet->payload.contenido, buffer + 4, packet->len + 1);
	packet->payload.contenido[packet->len -4]='\0';
	break;
      case nipc_req_read:
	if (packet->len == 4)
	  {
	    memcpy((uint32_t *)&packet->payload.sector, buffer, packet->len + 1);
	  }
	if (packet->len == 516)
	{
	  memcpy((uint32_t *)&packet->payload.sector   , buffer    , 4 + 1);
	  memcpy((uint8_t *) &packet->payload.contenido, buffer + 4, packet->len + 1);
	  packet->payload.contenido[packet->len]='\0';
	}
	break;
      case nipc_req_write:
	if (packet->len == 4)
	  {
	    memcpy((uint32_t *)&packet->payload.sector, buffer, packet->len + 1);
	  }
	if (packet->len == 516)
	{
	  memcpy((uint32_t *)&packet->payload.sector   , buffer    , 4 + 1);
	  memcpy((uint8_t  *)&packet->payload.contenido, buffer + 4, packet->len + 1);
	  packet->payload.contenido[packet->len]='\0';
	}
	break;
      case nipc_error:
	printf("ERROR\n");
	memcpy(&packet->payload.sector   , buffer    , 4 + 1);
	memcpy(&packet->payload.contenido, buffer + 4, packet->len + 1);
	packet->payload.contenido[packet->len -4]='\0';
	break;
      case nipc_CHS:
	break;
      default:
	return -1;
    }
    printf("%d - %d - %d - %s\n",packet->type,packet->len,packet->payload.sector,packet->payload.contenido);
    return leido;
}

/**
 * Envia un paquete en el socket especificado
 *
 * @return cantidad de caracteres enviados
 */
uint32_t send_socket(nipc_packet *packet, uint32_t sock)
{
    uint32_t Escrito = 0;
    uint32_t Aux = 0;
    char     buffer[1024];
    //* Comprobacion de los parametros de entrada
    if ((sock == -1) || (packet == NULL) || (519 < 1))
	return -1;
    
    /*
    * Preparo el paquete para ser enviado
    */
    memcpy(buffer    , &packet->type , sizeof(uint8_t));
    printf("type: %d\n",buffer[0]);
    memcpy(buffer + 1, &packet->len  , sizeof(uint16_t));
    uint16_t len_mensaje;
    switch (packet->type)
    {
      case 0: //nipc_handshake
	memcpy(buffer + 3, &packet->payload.sector   , sizeof(uint32_t) );
	switch (packet->payload.sector)
	{
	  case 0: // HANDSHAKE_OK
	    printf("HANDSHAKE %d\n",packet->len);
	    memcpy(buffer + 7, &packet->payload.contenido, packet->len);
	    break;
	  case 1:
	    len_mensaje = 4 + strlen("No hay discos conectados");
	    memcpy(buffer + 1, &len_mensaje , sizeof(uint16_t));
	    memcpy(buffer + 7, "No hay discos conectados", len_mensaje);
	    break;
	  case 2:
	    len_mensaje = 4 + strlen("La conexion ya ha sido establecida");
	    memcpy(buffer + 1, &len_mensaje , sizeof(uint16_t));
	    memcpy(buffer + 7, "La conexion ya ha sido establecida", len_mensaje);
	    break;
	  case 3:
	    len_mensaje = 4 + strlen("Se ha producido una falla al intentar conectarse");
	    memcpy(buffer + 1, &len_mensaje , sizeof(uint16_t));
	    memcpy(buffer + 7, "Se ha producido una falla al intentar conectarse", len_mensaje);
	    break;
	  default:
	    return -1;
	}
	break;
      case 1:  //nipc_req_read
	if (packet->len == 4)
	  {
	    memcpy(buffer + 3, &packet->payload.sector, packet->len + 1);
	  }
	if (packet->len == 516)
	{
	  memcpy(buffer + 3, &packet->payload.sector   , sizeof(uint32_t) );
	  memcpy(buffer + 7, &packet->payload.contenido, packet->len      );
	  //packet->payload.contenido[packet->len]='\0';
	}
	break;
      case 2: //nipc_req_write
	if (packet->len == 4)
	  {
	    memcpy(buffer + 3, &packet->payload.sector, packet->len );
	  }
	if (packet->len == 516)
	{
	  printf("Escritura\n");
	  memcpy(buffer + 3, &packet->payload.sector   , sizeof(uint32_t) );
	  memcpy(buffer + 7, &packet->payload.contenido, packet->len      );
	  //packet->payload.contenido[packet->len]='\0';
	}
	break;
      case 3: //nipc_error
	memcpy(buffer + 3, &packet->payload.sector   , sizeof(uint32_t) );
	uint32_t codigo;
	codigo = atoi((char *)packet->payload.contenido);
	printf("---%d---",codigo);
	switch (codigo)
	{
	  case 0: // todos los discos rotos
	    memcpy(buffer + 7, "Todos los discos estan dañados. Se cerrara la conexión.", strlen("Todos los discos estan dañados. Se cerrara la conexión."));
	    break;
	  case 1:
	    memcpy(buffer + 7, "El sector solicitado es inexistente.", strlen("El sector solicitado es inexistente."));
	    break;
	  case 2:
	    memcpy(buffer + 7, "No se pudo realizar la escritura del sector solicitado.", strlen("No se pudo realizar la escritura del sector solicitado."));
	    break;
	  case 3:
	    memcpy(buffer + 7, "No se puedo ralizar la lectura del sector solicitado.", strlen("No se puedo ralizar la lectura del sector solicitado."));
	    break;
	  default:
	    return -1;
	}
	break;
      case 4: //nipc_CHS
	break;
      default:
	return -1;
    }
    //* Bucle hasta que hayamos escrito todos los caracteres que nos han
    //* indicado.
    while (Escrito < (3 + packet->len))
    {
	Aux = send(sock, buffer + Escrito, (3 + packet->len) - Escrito,0);
	if (Aux > 0)
	{
	   //* Si hemos conseguido escribir caracteres, se actualiza la
	   //* variable escrito
	    Escrito = Escrito + Aux;
	}
	else
	{
	    //* Si se ha cerrado el socket, devolvemos el numero de caracteres
	    //* leidos.
	    //* Si ha habido error, devolvemos -1
	    if (Aux == 0)
		return Escrito;
	    else
		return -1;
	}
    }
    return Escrito;
}

/**
 * Pone en escucha al socket especificado
 *
 */
void nipc_listen(nipc_socket sock)
{
  if ((listen(sock,MAX_CONECTIONS))<0)
    {
      printf("Error listen");
      exit(EXIT_FAILURE);
    }
}

/**
 * Cierra el socket especificado
 *
 */
void nipc_close(nipc_socket sock)
{
  if (close(sock)<0)
  {
    printf("Error close");
    exit(EXIT_FAILURE);
  }
}

