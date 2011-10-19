#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>


#include "sock.h"



nipc_packet *next_packet=0;
uint32_t buffer[2000];
//static uint32_t i=0;
uint8_t handshake=0;


/**
 * Inicia un socket
 *
 * @nipc_socket estructura del socket del protocolo NIPC
 * @return el descriptor del socket creado
 */
nipc_socket *createSocket(char *host, uint16_t port)
{
    printf("\n\nCREA SOCKET\n\n");
    nipc_socket *sock_new = malloc(sizeof(nipc_socket));
    struct sockaddr_in addr_raid;
    int i = 1;
    if ((*sock_new = socket(AF_INET,SOCK_STREAM,0))<0)
    {
      printf("Error socket");
      exit(EXIT_FAILURE);
    }
    if (setsockopt(*sock_new, SOL_SOCKET, SO_REUSEADDR, &i,sizeof(sock_new)) < 0)
    {
      printf("\nError: El socket ya esta siendo utilizado...");
      exit(EXIT_FAILURE);
    }
    addr_raid.sin_family = AF_INET;
    addr_raid.sin_port=htons(50003);
    addr_raid.sin_addr.s_addr=inet_addr("127.0.0.1");
    memset(&(addr_raid.sin_zero),'\0',sizeof(addr_raid));
    if (bind(*sock_new,(struct sockaddr *)&addr_raid,sizeof(struct sockaddr_in))<0)
    {
      printf("Error bind");
      exit(EXIT_FAILURE);
    }
    if ((listen(*sock_new,10))<0)
    {
      puts("Error listen");
      exit(EXIT_FAILURE);
    }
    
    printf("\n\nSOCKET Creado bien: %d\n\n",*sock_new);
        
    return sock_new;
}


/**
 * Cierra el socket especificado
 *
 */
void closeSocket(nipc_socket *socket)
{
    if(socket)
        close(socket);
}

/**
 * Envia un paquete con socket indicado
 *
 */
void sendSocket(nipc_packet *packet, nipc_socket *socket)
{
    
}

/**
 * Recive un paquete en el socket especificado
 *
 * @nipc_packet estructura del protocolo NIPC
 * @return el paquete recivido
 */
nipc_packet * recvSocket(nipc_socket *socket)
{
    nipc_packet *algo=NULL;
    return algo;
}


