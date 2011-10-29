#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "nipc.h"

#define MAX_CONECTIONS 10

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
	uint32_t leido = 0;
	uint32_t aux = 0;
	//* Comprobacion de que los parametros de entrada son correctos
	if ((sock == -1) || (packet == NULL))
		return -1;
	//* Mientras no hayamos leido todos los datos solicitados
	while (leido < 519)
	{
		aux = recv (sock, packet->buffer + leido, 519 - leido,0);
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
				return leido;
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
	return leido;
}

/**
 * Envia un paquete en el socket especificado
 *
 * @return cantidad de caracteres enviados
 */
uint32_t send_socket(nipc_packet *packet, nipc_socket sock)
{
	uint32_t escrito = 0;
	uint32_t aux = 0;
	//* Comprobacion de los parametros de entrada
	if ((sock == -1) || (packet == NULL))
		return -1;
	//* Bucle hasta que hayamos escrito todos los caracteres que nos han
	//* indicado.
	while (escrito < 519)
	{
		aux = send(sock, packet->buffer + escrito, 519 - escrito,0);
		if (aux > 0)
		{
			//* Si hemos conseguido escribir caracteres, se actualiza la
			//* variable escrito
			escrito = escrito + aux;
		}
		else
		{
			//* Si se ha cerrado el socket, devolvemos el numero de caracteres leidos
			//* Si ha habido error, devolvemos -1
			if (aux == 0)
				return escrito;
			else
				return -1;
		}
	}
	return escrito;
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

