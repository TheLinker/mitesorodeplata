#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<stdlib.h>
#include<stdio.h>
#include<string.h>

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


uint32_t sendSocket(nipc_packet *packet, uint32_t sock)
{
	uint32_t Escrito = 0;
	uint32_t Aux = 0;

	/*
	* Comprobacion de los parametros de entrada
	*/
	if ((sock == -1) || (packet == NULL) || (519 < 1))
		return -1;
	
	/*
	* Bucle hasta que hayamos escrito todos los caracteres que nos han
	* indicado.
	*/
	while (Escrito < 519)
	{
		Aux = send(sock, packet->buffer + Escrito, 519 - Escrito,0);
		if (Aux > 0)
		{
			/*
			* Si hemos conseguido escribir caracteres, se actualiza la
			* variable Escrito
			*/
			Escrito = Escrito + Aux;
		}
		else
		{
			/*
			* Si se ha cerrado el socket, devolvemos el numero de caracteres
			* leidos.
			* Si ha habido error, devolvemos -1
			*/
			if (Aux == 0)
				return Escrito;
			else
				return -1;
		}
	}
	/*
	* Devolvemos el total de caracteres leidos
	*/
	return Escrito;
}



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
	mensaje.type=0;
	strcpy(mensaje.payload.contenido,ppd_name);
	mensaje.payload.sector=0;
	mensaje.len=strlen(mensaje.payload.contenido);
	
	//if(send(sock_ppd,mensaje.buffer, sizeof(mensaje.buffer)+1,0)<0)
	if(sendSocket(&mensaje,sock_ppd)<0)
	{
		printf("\nError send");
		exit(EXIT_FAILURE);
	}
	else
	printf("\n\nEl mensaje enviado es: %d - %d - %d - %s",mensaje.type,mensaje.len,mensaje.payload.sector,mensaje.payload.contenido);
	
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
			if(sendSocket(&mensaje,sock_ppd)<0)
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
			strcpy(mensaje.payload.contenido,"Escritura OK");
			mensaje.len=516;
			if(sendSocket(&mensaje,sock_ppd)<0)
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

