// cd Desarrollo/Workspace/mitesorodeplata/src/praid1/
// gcc soc_ppd.c -o ppd

#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include <errno.h>
#include<pthread.h>


#include<stdio.h>
#include<unistd.h>
#include<string.h>
#include<pthread.h>
#include<sys/msg.h>
#include<sys/sem.h>
#include<stdlib.h>
#include<signal.h>

typedef struct payload_t{
        uint32_t  sector;
	char   contenido[512];
    }  __attribute__ ((packed)) payload_t;


typedef union nipc_packet {
    char buffer[519];
    struct {
        char    type;
	uint16_t   len;
	payload_t  payload;
    } __attribute__ ((packed));
} nipc_packet;


int32_t send_socket(nipc_packet *packet, uint32_t sock)
{
    int32_t Escrito = 0;
    int32_t Aux = 0;
    if ((sock == -1) || (packet == NULL) || (519 < 1))
	return -1;
    while (Escrito < (3 + packet->len))
    {
	Aux = send(sock, packet + Escrito, (3 + packet->len) - Escrito,0);
	if (Aux > 0)
	{
	  Escrito = Escrito + Aux;
	}
	else
	{
	    if (Aux == 0)
		return Escrito;
	    else
		return -1;
	}
    }
    return Escrito;
}

/**
 * Recibe un paquete del socket indicado
 *
 * @return cantidad de caracteres recividos, en caso de ser 0 informa que el socket ha sido cerrado
 */
int32_t recv_socket(nipc_packet *packet, uint32_t sock)
{
    int32_t   leido = 0;
    int32_t   aux = 0;
    if ((sock == -1) || (packet == NULL))
	return -1;
    aux = recv (sock, packet, 3, 0);
    leido = leido + aux;
    while (leido < (3 + packet->len))
    {
	aux = recv(sock, packet->buffer + leido, (3 + packet->len) - leido,0);
        if (aux > 0)
	{
		leido = leido + aux;
	}
	else
	{
	    if (aux == 0) 
	    {
	      return leido;
	    }
	    if (aux == -1)
	    {
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

/*void *espera_respuestas(uint32_t *sock)
{
  nipc_packet *packet;
  int32_t recibido =0;
  while(1)
  {
    recibido = 0;
    recibido = recv_socket(packet,*sock);
    printf("Recibido: %d - %d\n",recibido,*sock);
    if(recibido >= 0)
    {
      printf("Control de mensaje recibido: %d - %d - %d - %s\n",packet->type,packet->len,packet->payload.sector,packet->payload.contenido);
    }
    else
    {
      printf("se callo el sistema RAID\n");
      exit(EXIT_FAILURE);
    }
  }
}*/

int main(int argc, char **argv){
	uint32_t sock_ppd;
	struct sockaddr_in addr_ppd;
	struct sockaddr_in *addr_raid=malloc(sizeof(struct sockaddr_in));
	int i = 1;
	
	if ((sock_ppd = socket(AF_INET,SOCK_STREAM,0))<0)
	{
		printf("Error socket");
		exit(EXIT_FAILURE);
	}
	if (setsockopt(sock_ppd, SOL_SOCKET, SO_REUSEADDR, &i,sizeof(sock_ppd)) < 0)
	{
		printf("\nError: El socket ya esta siendo utilizado...");
		exit(EXIT_FAILURE);
	}
	
	//datos del PPD
	addr_ppd.sin_family = AF_INET;
	addr_ppd.sin_port=htons(atoi(argv[1]));
	addr_ppd.sin_addr.s_addr=inet_addr("127.0.0.1");
	
	memset(&(addr_ppd.sin_zero),'\0',sizeof(addr_ppd));
	
	if (bind(sock_ppd,(struct sockaddr *)&addr_ppd,sizeof(struct sockaddr_in))<0)
	{
		printf("\nError bind");
		exit(EXIT_FAILURE);
	}
	
	//datos del PRAID
	addr_raid->sin_family = AF_INET;
	addr_raid->sin_port=htons(50000);
	addr_raid->sin_addr.s_addr=inet_addr("127.0.0.1");
	
	if(connect(sock_ppd,(struct sockaddr *)addr_raid,sizeof(struct sockaddr_in))<0)
	{
		printf("\nError connect");
		exit(EXIT_FAILURE);
	}
	char ppd_name[512];
	printf("Ingrese nombre disco: ");
	scanf("%s",ppd_name);
	
	nipc_packet mensaje;
	
	
	//Envio HANDSHAKE
	mensaje.type=0;
	strcpy(mensaje.payload.contenido,ppd_name);
	mensaje.payload.sector=0;
	mensaje.len = 4+strlen(mensaje.payload.contenido);
	if(send_socket(&mensaje,sock_ppd)<0)
	{
		printf("\nError send");
		exit(EXIT_FAILURE);
	}
	else
	printf("El mensaje enviado es: %d - %d - %d - %s\n\n",mensaje.type,mensaje.len,mensaje.payload.sector,mensaje.payload.contenido);
	
	//Recibo respuesta HANDSHAKE
	if(recv_socket(&mensaje,sock_ppd)<0)
	{
		printf("\nError recv");
		exit(EXIT_FAILURE);
	}
	else
	printf("El mensaje recibido es: %d - %d - %d - %s\n\n",mensaje.type,mensaje.len,mensaje.payload.sector,mensaje.payload.contenido);
	
	//Envio CHS o cantidad de sectores
	mensaje.type=0;
	strcpy(mensaje.payload.contenido,"");  //CHS(8192, 1, 512) o CHS(1024, 1, 1024)
	mensaje.payload.sector = 10;  
	mensaje.len = 4;
	if(send_socket(&mensaje,sock_ppd)<0)
	{
		printf("\nError send");
		exit(EXIT_FAILURE);
	}
	else
	printf("El mensaje enviado es: %d - %d - %d - %s\n\n",mensaje.type,mensaje.len,mensaje.payload.sector,mensaje.payload.contenido);
	
	/*
	pthread_t respuestas;
	if(pthread_create(&respuestas,NULL,(void *)espera_respuestas,(void *)&sock_ppd) < 0)
	  printf("Error en la creacion del hilo\n");
	*/
	
	uint32_t unSector;
	char opcion;
	while(1){
		printf("\n\n\n-------------------------------------");
		printf("\n-------------------------------------");
		printf("\n1. Devolver Lectura");
		printf("\n2. Devolver Escritura");
		printf("\n3. ");
		printf("\n4. ");
		printf("\n5. ");
		printf("\n6. ");
		printf("\n7. ");
		printf("\n8. ");
		printf("\n9. Salir");
		printf("\nIngrese opcion: ");
		opcion = getchar();
		getchar();
		if(opcion == '1')
		{
			printf("\n	Devolver Lectura");
			printf("\nIngrse numero de sector: ");
			scanf("%d",&unSector);
			mensaje.type=1;
			mensaje.payload.sector = unSector;
			strcpy(mensaje.payload.contenido,"esto es el contenido!!!");
			mensaje.len=516;
			if(send_socket(&mensaje,sock_ppd)<0)
			{
				printf("\nError send");
				exit(EXIT_FAILURE);
			}
			else
			  printf("\n\nEl mensaje enviado es: %d - %d - %d - %s",mensaje.type,mensaje.len,mensaje.payload.sector,mensaje.payload.contenido);
		}
		if(opcion == '2')
		{
			printf("\n	Devolver Escritura");
			printf("\nIngrse numero de sector: ");
			scanf("%d",&unSector);
			mensaje.type=2;
			mensaje.payload.sector = unSector;
			strcpy(mensaje.payload.contenido,"");
			mensaje.len=4;
			if(send_socket(&mensaje,sock_ppd)<0)
			{
				printf("\nError send");
				exit(EXIT_FAILURE);
			}
			else
			  printf("\n\nEl mensaje enviado es: %d - %d - %d - %s",mensaje.type,mensaje.len,mensaje.payload.sector,mensaje.payload.contenido);
		}
		if(opcion == '3')
		{
			printf("\n	");
			
		}
		if(opcion == '4')
		{
			printf("\n	");
			
		}
		if(opcion == '5')
		{
			printf("\n	");
			
		}
		if(opcion == '6')
		{
			printf("\n	");
			
		}
		if(opcion == '7')
		{
			printf("\n	");
			
		}
		if(opcion == '8')
		{
			printf("\n	");
			
		}
		if(opcion == '9')
		{
			printf("\n	Salir\n\n");
			close(sock_ppd);
			exit(EXIT_SUCCESS);
		}
	}
	
	return 0;
}

