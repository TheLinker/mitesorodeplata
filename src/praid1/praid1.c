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
#include "nipc.h"
#include "log.h"
#include "praid1.h"
#include "praid_func.h"

int main(int argc, char *argv[])
{
  config_t *config;
  config = calloc(sizeof(config_t), 1);
  config_read(config);
  log_t* log = log_new(config->log_path, "RAID", config->log_mode);
  // log_info(log, "Principal", "Message info: %s", "se conecto el cliente xxx");
  // log_warning(log, "Principal", "Message warning: %s", "not load configuration file");
  // log_error(log, "Principal", "Message error: %s", "Crash!!!!");
  
  datos               *info_ppal = (datos *)malloc(sizeof(datos));
  pfs                 *aux_pfs;
  uint32_t             max_sock;
  nipc_packet          mensaje;
  nipc_socket          sock_new;
  struct sockaddr_in  *addr_ppd = malloc(sizeof(struct sockaddr_in));
  uint32_t             clilen;
  fd_set               set_socket;
  
  info_ppal->sock_raid = -1;
  info_ppal->lista_pfs = NULL;
  info_ppal->discos    = NULL;
  
  info_ppal->sock_raid = create_socket();
  nipc_bind_socket(info_ppal->sock_raid,(char *)config->server_host,config->server_port);
  nipc_listen(info_ppal->sock_raid);
  
  
  
  printf("------------------------------\n");
  printf("--- Socket escucha RAID: %d ---\n",info_ppal->sock_raid);
  printf("------------------------------\n");
  log_info(log, "Principal", "Message info: Socket escucha %d", info_ppal->sock_raid);
  
  sem_init(&(info_ppal->semaforo),1,1);
  
    
  while(1)
  {
    FD_ZERO(&set_socket);
    
    FD_SET(info_ppal->sock_raid, &set_socket);
    
    max_sock = info_ppal->sock_raid;
    
    aux_pfs = info_ppal->lista_pfs;
    while(aux_pfs != NULL)
    {
      if(aux_pfs->sock > max_sock)
	max_sock = aux_pfs->sock;
      FD_SET (aux_pfs->sock, &set_socket);
      aux_pfs=aux_pfs->sgte;
    }
    
    //listar_pedidos_discos(info_ppal->discos);
    //if (info_ppal->discos!=NULL)printf("------------------------------\n");
    
    select(max_sock+1, &set_socket, NULL, NULL, NULL);  
    
    /*
     * Control discos consistentes
     */
    if((info_ppal->max_sector == 0) && (info_ppal->discos != NULL))
    {
      printf("No hay discos consistentes!!!\n");
      break;
    }
        
    /*
     * Lista de socket PFS
     */
    aux_pfs=info_ppal->lista_pfs;
    while(aux_pfs != NULL)
    {
      if(FD_ISSET(aux_pfs->sock, &set_socket)>0)
      {
	if(recv_socket(&mensaje,aux_pfs->sock)>0)
	{ //sleep(2);
	 mensaje.payload.contenido[mensaje.len-4]='\0';
	 //printf("El mensaje es: %d - %d - %d - %s\n",mensaje.type,mensaje.len,mensaje.payload.sector,mensaje.payload.contenido);
	 if(mensaje.type == nipc_handshake)
	 {
	   printf("BASTA DE HANDSHAKE!!!!\n");
	   log_warning(log, "Principal", "Message warning: %s", "Ya se realizo el HANDSHAKE");
	 }
	  if(mensaje.type == nipc_req_read)
	  {
	    if(mensaje.len == 4)
	    {
	      uint8_t *id_disco;
	      printf("Pedido de lectura del FS: %d\n",mensaje.payload.sector);
	      id_disco = distribuir_pedido_lectura(&info_ppal,mensaje,aux_pfs->sock);
	      log_info(log, "Principal", "Message info: Pedido lectura sector %d en disco %s", mensaje.payload.sector,id_disco);
	      printf("------------------------------\n");
	    }
	    else
	    {
	      printf("MANDASTE CUALQUIER COSA\n");
	      log_warning(log, "Principal", "Message warning: %s", "Formato de mensaje no reconocido");
	    }
	  }
	  if(mensaje.type == nipc_req_write)
	  {
	    if(mensaje.len != 4)
	    {
	      printf("Pedido de escritura del FS: %d - %s\n",mensaje.payload.sector,mensaje.payload.contenido);
	      distribuir_pedido_escritura(&info_ppal,mensaje,aux_pfs->sock);
	      log_info(log, "Principal", "Message info: Pedido escritura sector %d", mensaje.payload.sector);
	      printf("------------------------------\n");
	    }
	    else
	    {
	      printf("MANDASTE CUALQUIER COSA\n");
	      log_warning(log, "Principal", "Message warning: %s", "Formato de mensaje no reconocido");
	    }
	  }
	  if(mensaje.type == nipc_error)
	  {
	    printf("ERROR: %d - %s\n",mensaje.payload.sector,mensaje.payload.contenido);
	    log_error(log, "Principal", "Message error: Sector:%d Error: %s", mensaje.payload.sector,mensaje.payload.contenido);
	  }
	}
	else
	{
	  printf("Se cayo la conexion con el PFS: %d\n",aux_pfs->sock);
	  log_warning(log, "Principal", "Message warning: Se cayo la conexion con el PFS:%d",aux_pfs->sock);
	  liberar_pfs_caido(&info_ppal->lista_pfs,aux_pfs->sock);
	  printf("------------------------------\n");
	}
      }
      aux_pfs = aux_pfs->sgte;
    }
    
    /*
     * SOCKECT PRINCIPAL DE ESCUCHA
     */
    
    if(FD_ISSET(info_ppal->sock_raid, &set_socket)>0)
    {
      if( (sock_new=accept(info_ppal->sock_raid,(struct sockaddr *)addr_ppd,(void *)&clilen)) <0)
      {
	printf("ERROR en la nueva conexion\n");
	log_error(log, "Principal", "Message error: %s", "No se pudo establecer la conexion");
      }
      else
      {
	if(recv_socket(&mensaje,sock_new)>=0)
	{ //sleep(2);
	  mensaje.payload.contenido[mensaje.len-4]='\0';
	  //printf("El mensaje es: %d - %d - %d - %s\n",mensaje.type,mensaje.len,mensaje.payload.sector,mensaje.payload.contenido);
	  if(mensaje.type == nipc_handshake)
	  {
	    if(mensaje.len != 0)
	    {
	        if(info_ppal->discos == NULL) 
		  log_info(log, "Principal", "Message info: %s", "Entra en funcionamiento el PRAID");
		printf("Nueva conexion PPD: %s - %d \n",mensaje.payload.contenido, sock_new);
		//char id_disco[20];
		//memcpy(&id_disco,mensaje.payload.contenido,20);
		agregar_disco(&info_ppal,(uint8_t *)mensaje.payload.contenido,sock_new);//crea hilo
		FD_SET (sock_new, &set_socket);
		log_info(log, "Principal", "Message info: Nueva conexion PPD: %s", mensaje.payload.contenido);
		mensaje.type = 0;
		mensaje.len = 0;
		if(send_socket(&mensaje,sock_new)<0)
		  printf("Error ennvio de HANDSHAKE OK");
		printf("------------------------------\n");
	    }
	    else
	    {
	      if(info_ppal->discos!=NULL)
	      {
		  printf("Nueva conexion PFS: %d\n",sock_new);
		  pfs *nuevo_pfs;
		  nuevo_pfs = (pfs *)malloc(sizeof(pfs));
		  nuevo_pfs->sock=sock_new;
		  nuevo_pfs->sgte = info_ppal->lista_pfs;
		  info_ppal->lista_pfs = nuevo_pfs;
		  FD_SET (nuevo_pfs->sock, &set_socket);
		  log_info(log, "Principal", "Message info: Nueva conexion PFS: %d", sock_new);
		  mensaje.type = 0;
		  mensaje.len = 0;
		  if(send_socket(&mensaje,sock_new)<0)
		    printf("Error ennvio de HANDSHAKE OK");
		  printf("------------------------------\n");
	      }
	      else
	      {
		  //ENVIAR ERROR DE CONEXION
		  printf("No hay discos conectados!\n");
		  log_error(log, "Principal", "Message error: %s", "No hay discos conectados!");
		  printf("cerrar conexion: %d\n",sock_new);
		  mensaje.type = 0;
		  memcpy(mensaje.payload.contenido,"No hay discos conectados!",strlen("No hay discos conectados")+1);
		  mensaje.len = 4+strlen("No hay discos conectados")+1;
		  if(send_socket(&mensaje,sock_new)<0)
		    printf("Error ennvio de HANDSHAKE OK");
		  nipc_close(sock_new);
	      }
	    }
	  }
	  if(mensaje.type == nipc_req_read)
	  {
	    printf("ATENCION!!! Formato de mensaje no reconocido: %d - %d - %d - %s\n",mensaje.type, mensaje.len, mensaje.payload.sector, mensaje.payload.contenido);
	    log_warning(log, "Principal", "Message warning: Formato de mensaje no reconocido: %d - %d - %d - %s",
		mensaje.type, mensaje.len, mensaje.payload.sector, mensaje.payload.contenido);
	    printf("------------------------------\n");
	  }
	  if(mensaje.type == nipc_req_write)
	  {
	    printf("ATENCION!!! Formato de mensaje no reconocido: %d - %d - %d - %s\n",mensaje.type, mensaje.len, mensaje.payload.sector, mensaje.payload.contenido);
	    log_warning(log, "Principal", "Message warning: Formato de mensaje no reconocido: %d - %d - %d - %s",
			mensaje.type, mensaje.len, mensaje.payload.sector, mensaje.payload.contenido);
	    printf("------------------------------\n");
	  }
	  if(mensaje.type == nipc_error)
	  {
	    printf("ERROR: %d - %s\n",mensaje.payload.sector,mensaje.payload.contenido);
	    log_error(log, "Principal", "Message error: Sector:%d Error: %s",
		      mensaje.payload.sector,mensaje.payload.contenido);
	  }
	}
      }
    }
  }
  printf("\n\n Se termino el programa \n\n");
  log_warning(log, "Principal", "Message warning: Sali del sistema");
  log_delete(log);
  exit(EXIT_SUCCESS);
}