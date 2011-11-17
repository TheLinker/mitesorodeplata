#include<stdio.h>
#include <time.h>
#include<unistd.h>
#include<string.h>
#include <sys/timeb.h>
#include<pthread.h>
#include<sys/msg.h>
#include<sys/sem.h>
#include<stdlib.h>
#include<signal.h>
#include "praid_func.h"
#include "praid1.h"
#include "nipc.h"
#include "log.h"


#define ESTADO_OK   1
#define ESTADO_FAIL 0

/**
 * Agrega un nuevo disco al RAID
 *
 */
void agregar_disco(datos **info_ppal,uint8_t *id_disco, nipc_socket sock_new)
{
  disco *nuevo_disco;
  nuevo_disco = (disco *)malloc(sizeof(disco));
  memcpy(nuevo_disco->id,id_disco,strlen((char *)id_disco)+1);
  nuevo_disco->sock=sock_new;
  nuevo_disco->sector_sincro = (int32_t) -1;
  nuevo_disco->ya_sincro = NULL;
  nuevo_disco->cantidad_pedidos = 0;
  nuevo_disco->pedidos = NULL;
  nuevo_disco->sgte = (*info_ppal)->discos;
  (*info_ppal)->discos = nuevo_disco;
  if(pthread_create(&nuevo_disco->hilo,NULL,(void *)espera_respuestas,(void *)info_ppal)!=0)
    printf("Error en la creacion del hilo\n");
}

/**
 * Lista todos los discos y sus pedidos asignados
 *
 */
void listar_pedidos_discos(disco *discos)
{
	disco *aux_disco;
	aux_disco = discos;
	while(aux_disco != NULL)	
	{
		printf("Disco %s, Cantidad %d, Sincronizado %d\n", aux_disco->id, aux_disco->cantidad_pedidos, aux_disco->sector_sincro);
		pedido *aux_pedido;
		aux_pedido = aux_disco->pedidos;
		while(aux_pedido!=NULL)
		{
			printf("\ttype_pedido %d, Sector %d\n", aux_pedido->type, aux_pedido->sector);
			aux_pedido=aux_pedido->sgte;
		}
		aux_disco=aux_disco->sgte;
	}
}

/**
 * Obtiene la menor cantidad de pedidos de todos los discos
 *
 * @return menor cantidad de pedidos en discos
 */
nipc_socket menor_cantidad_pedidos(disco *discos, int32_t sector)
{
	disco *aux_disco;
	int32_t menor_pedido=99999;
	nipc_socket sock_disco;
	aux_disco = discos;
	while(aux_disco != NULL)
	{
		if (menor_pedido > aux_disco->cantidad_pedidos  &&  sector <= aux_disco->sector_sincro)
			{
				sock_disco   = aux_disco->sock;
				menor_pedido = aux_disco->cantidad_pedidos;
				//printf("Disco:%s - %d - %d \n",aux_disco->id,aux_disco->cantidad_pedidos,aux_disco->sector_sincro);
			}
		aux_disco=aux_disco->sgte;
	}
	return sock_disco;
}

/**
 * Distribulle un pedido de lectura de la cola de mensajes
 *
 */
uint8_t* distribuir_pedido_lectura(disco **discos, nipc_packet mensaje,nipc_socket sock_pfs)
{
	uint16_t encontrado = 0;
	nipc_socket sock_disco;
	sock_disco =  menor_cantidad_pedidos(*discos,mensaje.payload.sector);
	disco *aux;
	aux = *discos;
	while((aux != NULL) && encontrado == 0)
	{
		if (aux->sock == sock_disco)
			encontrado = 1;
		else
			aux = aux->sgte;
	}
	pedido *nuevo_pedido;
	nuevo_pedido = (pedido *)malloc(sizeof(pedido));
	nuevo_pedido->sock = sock_pfs;
	nuevo_pedido->type = mensaje.type;
	nuevo_pedido->sector = mensaje.payload.sector;
	strcpy((char *)nuevo_pedido->contenido,"");
	if (encontrado == 0)
	  return (uint8_t*)"NODISK";
	nuevo_pedido->sgte = aux->pedidos;
	aux->pedidos = nuevo_pedido;
	aux->cantidad_pedidos++;
	
	
	if(send_socket(&mensaje,aux->sock)<=0)
	{
		printf("\nError send");
		exit(EXIT_FAILURE);
	}
	
	//printf("Lectura en disco: %s\n",aux->id);
	return aux->id;
}

/**
 * Distribulle un pedido de Escritura de la cola de mensajes
 *
 */
void distribuir_pedido_escritura(disco **discos, nipc_packet mensaje,nipc_socket sock_pfs)
{

	disco *aux;
	aux = *discos;
	while(aux != NULL)	
	{
		pedido *nuevo_pedido;
		nuevo_pedido = (pedido *)malloc(sizeof(pedido));
		nuevo_pedido->sock = sock_pfs;
		nuevo_pedido->type = mensaje.type;
		nuevo_pedido->sector = mensaje.payload.sector;
		strcpy((char *)nuevo_pedido->contenido,(char *)mensaje.payload.contenido);
		nuevo_pedido->sgte = aux->pedidos;
		aux->pedidos = nuevo_pedido;
		
		if(send_socket(&mensaje,aux->sock)<0)
		{
		  printf("\nError send");
		  exit(EXIT_FAILURE);
		}
		//printf("Escritura en disco: %s\n",aux->id);
		aux=aux->sgte;
	}
}

/**
 * Recibe constantemente los pedidos que alla en la cola
 *
 */
void *espera_respuestas(datos **info_ppal)
{
  config_t *config;
  config = calloc(sizeof(config_t), 1);
  config_read(config);
  log_t* log = log_new(config->log_path, "RAID", config->log_mode);
  
  time_t log_time ;
  struct tm *log_tm ;
  struct timeb tmili;
  char str_time[128] ;
  
  nipc_packet  mensaje;
  pedido      *aux_pedidos;
  pedido      *anterior;
  disco       *el_disco;
  
  el_disco = (*info_ppal)->discos;
  while(el_disco != NULL && el_disco->hilo != pthread_self())
    el_disco = el_disco->sgte;
  
  recv_socket(&mensaje,el_disco->sock);
  uint32_t cant_sectores = mensaje.payload.sector;
  
  
  if ((*info_ppal)->discos->sgte != NULL)
  {  /** SINCRONIZAR **/
    log_time = time(NULL);
    log_tm = localtime( &log_time );
    strftime( str_time , 127 , "%H:%M:%S" , log_tm ) ;
    if (ftime (&tmili)){
	perror("[[CRITICAL]] :: No se han podido obtener los milisegundos\n");
	return 0;
    }
    log_info(log, (char *)el_disco->id, "Message info: Comienza sincronizacion: %s - %s.%.3hu ",el_disco->id,str_time, tmili.millitm);
    
    uint32_t i;
    uint8_t *id_disco;
    mensaje.type=1;
    mensaje.len=4;
    for(i=0;i<=cant_sectores;i++)
    {   
      mensaje.payload.sector = i;
      id_disco = distribuir_pedido_lectura(&(*info_ppal)->discos , mensaje , el_disco->sock);
      printf("Pedido Sincronizacion: %s - %d\n", id_disco, i);
    }
    printf("------------------------------\n");
  }
  else
  {    /** UNICO DISCO - MAESTRO **/
    el_disco->sector_sincro = cant_sectores;
    (*info_ppal)->max_sector = cant_sectores;
  }
  
  while(1)
  {
    strcpy((char *)mensaje.buffer, "");
    int32_t recibido = 0;
    recibido = recv_socket(&mensaje,el_disco->sock);
    //printf("recibido: %d - %d",recibido,el_disco->sock);
    //getchar();
    if(recibido > 0)
    {
      mensaje.payload.contenido[mensaje.len-4]='\0';
      //printf("El mensaje es: %d - %d - %d - %s\n",mensaje.type,mensaje.len,mensaje.payload.sector,mensaje.payload.contenido);
      if(mensaje.type != nipc_handshake)
      {
	if(mensaje.type == nipc_CHS)
	{/** nunca va a llegar aca **/
	  printf("Disco: %s - CHS: %s\n",el_disco->id,mensaje.payload.contenido);
	  log_info(log, (char *)el_disco->id, "Message info: Disco: %s - CHS: %s",el_disco->id,mensaje.payload.contenido);
	}
	else
	{      
	  aux_pedidos=el_disco->pedidos;
	  anterior=NULL;
	  while(aux_pedidos != NULL && (aux_pedidos->type != mensaje.type || aux_pedidos->sector != mensaje.payload.sector))
	  {
		anterior=aux_pedidos;
		aux_pedidos=aux_pedidos->sgte;
	  }
	  if(aux_pedidos == NULL)
	  {
	    printf("Sector no solicitado !!!\n");
	  }
	  else
	  {
	    if(mensaje.type == nipc_error)
	    {
		  printf("ERROR: %d - %s\n",mensaje.payload.sector,mensaje.payload.contenido);
		  log_error(log, (char *)el_disco->id, "Message error: Sector:%d Error: %s", mensaje.payload.sector,mensaje.payload.contenido);
	    }
	    if(mensaje.type == nipc_req_read)
	    {
		  printf("Recibida lectura del sector: %d - %s\n",mensaje.payload.sector,mensaje.payload.contenido);
		  log_info(log, (char *)el_disco->id, "Message info: Recibida lectura del sector: %d - %s", mensaje.payload.sector,mensaje.payload.contenido);
	    }
	    if(mensaje.type == nipc_req_write)
	    {
		  printf("Recibida escritura del sector: %d - %s\n",mensaje.payload.sector,mensaje.payload.contenido);
		  log_info(log, (char *)el_disco->id, "Message info: Recibida escritura del sector: %d - %s", mensaje.payload.sector,mensaje.payload.contenido);
	    }
      
	    /**	Enviar a quien hizo el pedido
	           caso PFS se envia tal cual esta!
		   caso PPD se modifica el tipo ya que pasa a ser escritura de un disco nuevo
	    */
	    disco *aux_disco;
	    aux_disco = (*info_ppal)->discos;
	    while(aux_disco != NULL  &&  aux_disco->sock != aux_pedidos->sock)
		    aux_disco = aux_disco->sgte;
	    
	    if (aux_disco != NULL)//(aux_disco->sock == aux_pedidos->sock)
	    {  /** es una sincronizacion de disco **/
	      printf("Envio de sector %d al PPD %s-%d\n",mensaje.payload.sector,aux_disco->id,aux_pedidos->sock);
	      mensaje.type = 2;
	      /** ENVIAR ESCRITURA A DISCO **/
	      if(send_socket(&mensaje,aux_pedidos->sock)<=0)
	      {
		      printf("\nError envio de sincro");
	      }
	      if((aux_disco->sector_sincro + 1) > mensaje.payload.sector)
	      {  /** ya estaba sincronizado este sector **/
		printf("Sector YA ESTABA SINCRONIZADO en disco: %s - %d\n",aux_disco->id,aux_pedidos->sock);
	      }
	      if((aux_disco->sector_sincro + 1) == mensaje.payload.sector)
	      {  /** se incrementa en 1 sector_sincro y se revisa la cola para ver si hay otros **/
		aux_disco->sector_sincro++;
		//printf("Sector preferido en disco: %s - %d\n",aux_disco->id,aux_pedidos->sock);
		sectores_t *aux;
		if (aux_disco->ya_sincro != NULL)
		{ 
		  while((aux_disco->sector_sincro+1) == (aux_disco->ya_sincro)->sector)
		  {
		    aux_disco->sector_sincro++;
		    aux = aux_disco->ya_sincro;
		    aux_disco->ya_sincro = (aux_disco->ya_sincro)->sgte;
		    free(aux);
		    if(aux_disco->ya_sincro == NULL)
		      break;
		  }
		}
	      }
	      if((aux_disco->sector_sincro + 1) < mensaje.payload.sector)
	      {  /** se encola ordenadamente para llevar el control de los sectores sincronizados **/
		//printf("Sector encolado en disco: %s - %d\n",aux_disco->id,aux_pedidos->sock);
		insertar(&(aux_disco->ya_sincro),mensaje.payload.sector);
		
		sectores_t *aux = aux_disco->ya_sincro;
		while(aux!=NULL)
		{
		  aux = aux->sgte;
		}
	      }
	      if(aux_disco->sector_sincro == (*info_ppal)->max_sector)
	      {
		log_time = time(NULL);
		log_tm = localtime( &log_time );
		strftime( str_time , 127 , "%H:%M:%S" , log_tm ) ;
		if (ftime (&tmili)){
		    perror("[[CRITICAL]] :: No se han podido obtener los milisegundos\n");
		    return 0;
		}
		printf("Sincronizacion COMPLETA\n");
		log_info(log, (char *)aux_disco->id, "Message info: Finaliza sincronizacion: %s - %s.%.3hu ",aux_disco->id,str_time, tmili.millitm);
	      }
	    }
	    else
	    {  /** es una respuesta a una escritura de un PFS **/
	      printf("Envio de sector %d al PFS %d\n",mensaje.payload.sector,aux_pedidos->sock);
	      if(send_socket(&mensaje,aux_pedidos->sock)<=0)
	      {
		      printf("\nError envio a PFS");
	      }
	      else
	      printf("Envio OK en PFS: %d\n",aux_pedidos->sock);
	    }
	    if(anterior == NULL)
	    {
	      el_disco->pedidos = (el_disco->pedidos)->sgte;
	    }
	    else
	    {
	      anterior->sgte = aux_pedidos->sgte;
	    }
	    free(aux_pedidos);
	    el_disco->cantidad_pedidos--;
	  }
	  printf("------------------------------\n");
	  listar_pedidos_discos((*info_ppal)->discos);
	}
      }
    }
    else
    {
      printf("Se perdio la conexion con el disco %s\n",(char *)el_disco->id);
      log_error(log, (char *)el_disco->id, "Message error: Se perdio la conexion con el disco %s", (char *)el_disco->id);
      nipc_close(el_disco->sock);
      el_disco->sock = -1;
      limpio_discos_caidos(info_ppal);
      break;
    }
    printf("------------------------------\n");
  }
  printf("------------------------------\n");
  pthread_exit(NULL);
}

/**
 * Inserta sector sincronizado en la lista
 *
 */
void insertar (sectores_t **lista, uint32_t sector)
{
  sectores_t *aux;
  sectores_t *new_sector = (sectores_t *)malloc(sizeof(sectores_t));
  new_sector->sector = sector;
  new_sector->sgte = NULL;
  
  if (*lista == NULL )
  {
    *lista = new_sector;
  }
  else
  {
    if((*lista)->sector > sector)
    {
      new_sector->sgte = *lista;
      *lista = new_sector;
    }
    else
    {
      aux=*lista;
      while(aux->sgte != NULL && (aux->sgte)->sector < sector)
	aux = aux->sgte;
      new_sector->sgte = aux->sgte;
      aux->sgte = new_sector;
    }
  }
}


/**
 * Limpia los PFS caidos
 *
 */
void liberar_pfs_caido(pfs **pedidos_pfs, nipc_socket sock)
{
  pfs *aux_pfs;
  pfs *anterior;
  aux_pfs = *pedidos_pfs;
  anterior=NULL;
  while(aux_pfs != NULL &&  aux_pfs->sock != sock)
  {
    anterior=aux_pfs;
    aux_pfs=aux_pfs->sgte;
  }
  if(anterior == NULL)
  {
    *pedidos_pfs = (*pedidos_pfs)->sgte;
  }
  else
  {
    anterior = anterior->sgte;
  }
  nipc_close(aux_pfs->sock);
  free(aux_pfs);
}

/**
 * Limpia los discos caidos
 *
 */
uint16_t limpio_discos_caidos(datos **info_ppal)
{
  config_t *config;
  config = calloc(sizeof(config_t), 1);
  config_read(config);
  log_t* log = log_new(config->log_path, "PRAID", config->log_mode);
  
  uint32_t estado = ESTADO_FAIL; // inicializa como si no hubiera discos sincronizados
  nipc_packet mensaje;
  disco *aux_discos;
  disco *anterior = NULL;
  aux_discos = (*info_ppal)->discos;
  while(aux_discos != NULL)
  {
    /** Si esta sincronizado y no se perdio la conexion el PRAID sigue funcionando **/
    if(aux_discos->sector_sincro == (*info_ppal)->max_sector && aux_discos->sock != -1)
      estado = ESTADO_OK; // en caso de que alla alguno sincronizado el PRAID esta OK
    if(aux_discos->sock == -1)
    {
      //printf("Limpieza de un disco\n");
      //Saco disco de la lista
      if(anterior == NULL)
	(*info_ppal)->discos = ((*info_ppal)->discos)->sgte;
      else
	anterior->sgte = aux_discos->sgte;
      listar_pedidos_discos((*info_ppal)->discos);
      disco  *liberar_disco;
      if ((*info_ppal)->discos != NULL)
      {
	//Re-Direcciono pedidos de discos caidos
	pedido *aux_pedidos;
	pedido *liberar_pedido;
	aux_pedidos = aux_discos->pedidos;
	while(aux_pedidos != NULL)
	{
	  //saco el pedido del disco
	  if (aux_pedidos->type == nipc_req_read)
	  {
	    mensaje.type = aux_pedidos->type;
	    mensaje.len = 4;
	    mensaje.payload.sector = aux_pedidos->sector;
	    strcpy((char *)mensaje.payload.contenido,(char *)aux_pedidos->contenido);
	    uint8_t *id_disco;
	    id_disco = distribuir_pedido_lectura(&(*info_ppal)->discos,mensaje,aux_pedidos->sock);
	    if(id_disco == (uint8_t*)"NODISK")
	    {
	      printf("No hay discos SINCRONIZADOS\n");
	      break;//return (uint16_t) 1;
	    }
	    else
	    {
	      printf("Re-Distribuir pedido: %d en disco: %s\n",mensaje.payload.sector, id_disco);
	      log_info(log, (char *)aux_discos->id , "Message info: Re-Distribuir pedido: %d en disco: %s", mensaje.payload.sector,id_disco);
	    }
	    aux_discos->cantidad_pedidos--;
	  }
	  aux_discos->pedidos = aux_pedidos->sgte;
	  liberar_pedido = aux_pedidos;
	  free(liberar_pedido);
	  aux_pedidos = aux_pedidos->sgte;
	}
      }
      else
      {  /** Enviar aviso a los PFS que se cerrara el sistema **/
	printf("No hay discos para redireccionar\n");
      }
      liberar_disco = aux_discos;
      free(liberar_disco);
    }
    anterior = aux_discos;
    aux_discos = aux_discos->sgte;
  }
  if (estado == ESTADO_FAIL)
  {
    (*info_ppal)->max_sector = 0;   // informa que no hay discos consistentes 
    printf("paso por aca\n");
  }
  return 0;
}


/**
 * Lee la configuracion
 *
 */
int config_read(config_t *config)
{
    char line[1024], w1[1024], w2[1024];
    FILE *fp;

    fp = fopen("./bin/praid.conf","r");
    if( fp == NULL ) {
        printf("No se encuentra el archivo de configuración\n");
        return 1;
    }

    while( fgets(line, sizeof(line), fp) ) {
        char* ptr;

        if( line[0] == '/' && line[1] == '/' )
            continue;
        if( (ptr = strstr(line, "//")) != NULL )
            *ptr = '\n'; //Strip comments
        if( sscanf(line, "%[^:]: %[^\t\r\n]", w1, w2) < 2 )
            continue;

        //Strip trailing spaces
        ptr = w2 + strlen(w2);
        while (--ptr >= w2 && *ptr == ' ');
        ptr++;
        *ptr = '\0';

        if(strcmp(w1,"host")==0)
            strcpy((char *)(config->server_host), w2);
        else 
        if(strcmp(w1,"puerto")==0)
            config->server_port = atoi(w2);
        else
        if(strcmp(w1,"flag_consola")==0)
        {
            if(strcmp(w1,"true") == 0)
                config->console = 1;
            else
            if(strcmp(w1,"false") == 0)
                config->console = 0;
        }else
        if(strcmp(w1,"log_path")==0)
            strcpy(config->log_path, w2);
        else
        if(strcmp(w1,"log_mode")==0)
        {
            if(strcmp(w2,"none") == 0)
	    config->log_mode = LOG_OUTPUT_NONE;
            else
            if(strcmp(w2,"console") == 0)
                config->log_mode = LOG_OUTPUT_CONSOLE;
            else
            if(strcmp(w2,"file") == 0)
                config->log_mode = LOG_OUTPUT_FILE;
            else
            if(strcmp(w2,"file+console") == 0)
                config->log_mode = LOG_OUTPUT_CONSOLEANDFILE;
        }
        else
            printf("Configuracion desconocida:'%s'\n", w1);
    }

    fclose(fp);
    return 0;
}