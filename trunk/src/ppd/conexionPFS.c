#include "conexionPFS.h"
#include "ppd.h"

extern nipc_socket sock_new;
pthread_t thEscucharPedidos, thAtenderpedidos;
int  thidEscucharPedidos, thidAtenderpedidos;
char* mensajet = NULL;

void conectarConPFS(config_t vecConfig)
{
	datos               *info_ppal;
	pfs                 *aux_pfs;
	nipc_packet          mensaje;

	//nipc_socket          sock_new;  DECLARADO EN EL MAIN

	//Necesario para el SELECT

	uint32_t             max_sock;      //numero del mayor socket
	fd_set               set_socket;    //estructura principal del SELECT

	info_ppal = (datos *)malloc(sizeof(datos));

	//inicializo estructura principal del PPD

	info_ppal->sock_ppd = -1;		// socket principal de escucha del PPD
	info_ppal->lista_pfs = NULL;	// lista de conexiones con el PFS

	// creo el socket principal del PPD

	info_ppal->sock_ppd = create_socket();

	nipc_bind_socket(info_ppal->sock_ppd,vecConfig.ipppd,vecConfig.puertoppd);

	// lo pongo a escuchar nuevas conexiones

	nipc_listen(info_ppal->sock_ppd);
	printf("------------------------------\n");
	printf("--- Socket escucha PPD: %d ---\n",info_ppal->sock_ppd);
	printf("------------------------------\n");

	/** El SELECT se coloca dentro de un bucle para que se consulte varias veces.
	  *   Cuando comienza el bucle se debe inicialñizar la estructura SELECT.
	  *   Se le agrega el socket principal de escucha.
	  *   Luego se le agregan en la misma todos los socket a los cuales queres estar atentos de nuevos pedidos.
	  *   Se van consultando uno a uno para ver si tal socket fue el que tubo algun cambio
	  */
	  while(1)
	  {
	    FD_ZERO(&set_socket);              			// Aca se vacia todo el contenido de la estructura del SELECT: se inicializa

	    FD_SET(info_ppal->sock_ppd, &set_socket);   // Agrego el socket principal, de escucha, del RAID

	    max_sock = info_ppal->sock_ppd;   			// al ser el primer y unico socket en la estructura SELECT lo guardo como el mayor

	    /** Recorro la lista de conexiones del PFS y las agrego a la estructura del SELECT
		*   Ademas comparo si es un socket mayor al anterior y lo actualizo
	    */

		aux_pfs = info_ppal->lista_pfs;

		while(aux_pfs != NULL)
		    {
		      if(aux_pfs->sock > max_sock)     			// comparo si es mayor al que teniamos antes como max_sock
			    max_sock = aux_pfs->sock;      			// actualizo si es mayor
		      FD_SET (aux_pfs->sock, &set_socket);   	// agrego la conexion del PFS a la estructura del SELECT
		      aux_pfs=aux_pfs->sgte;
		    }

			// Funcion SELECT, es bloqueante:
			// Se queda a la espera de algun cambio en los socket anteriormente agregados a la estuctura del SELECT
		    select(max_sock+1, &set_socket, NULL, NULL, NULL);

		    /*
		     * Lista de socket PFS
			 * Recorro la lista de las conexiones PFS y consulto si tubo algun cambio
			 * De ser asi, se recbe el mensaje y se procesa
		     */
		    /*aux_pfs=info_ppal->lista_pfs;
		    while(aux_pfs != NULL)
		    {
		      if(FD_ISSET(aux_pfs->sock, &set_socket)>0)   // se consulta si ese socket tubo algun cambio
		      {
				if(recv_socket(&mensaje,aux_pfs->sock)>0)   // si el mensaje recibido es mayor a 0 bytes se procesa, en caso contrario se perdio la conexion
				{
				 if(mensaje.type == nipc_handshake)     // pregunto si es de tipo HANDSHAKE
				 {
				   printf("BASTA DE HANDSHAKE!!!!\n");   //en este paso no debo resivir un HANDSHAKE, ya que los HANDSHAKE lo resive el socket principal
				   //log_warning(log, "Principal", "Message warning: %s", "Ya se realizo el HANDSHAKE");
				 }
				  if(mensaje.type == nipc_req_read)    // si es de tipo lectura
				  {
					if(mensaje.len == 4)       // se trata de un pedido nuevo de lectura
					{
					  //uint8_t *id_disco;
					  printf("Pedido de lectura del FS: %d\n",mensaje.payload.sector);

					  //id_disco = distribuir_pedido_lectura(&info_ppal->discos,mensaje,aux_pfs->sock);
					  //log_info(log, "Principal", "Message info: Pedido lectura sector %d en disco %s", mensaje.payload.sector,id_disco);
					  printf("------------------------------\n");
					}
					else
					{
					  printf("MANDASTE CUALQUIER COSA\n");
					  //log_warning(log, "Principal", "Message warning: %s", "Formato de mensaje no reconocido");
					}
				  }
				  if(mensaje.type == nipc_req_write)   // si es de tipo escritura
				  {
					if(mensaje.len != 4)      // se trata de un pedido nuevo de escritura
					{
					  printf("Pedido de escritura del FS: %d - %s\n",mensaje.payload.sector,mensaje.payload.contenido);
					  //distribuir_pedido_escritura(&info_ppal->discos,mensaje,aux_pfs->sock);
					  //log_info(log, "Principal", "Message info: Pedido escritura sector %d", mensaje.payload.sector);
					  printf("------------------------------\n");
					}
					else
					{
					  printf("MANDASTE CUALQUIER COSA\n");
					  //log_warning(log, "Principal", "Message warning: %s", "Formato de mensaje no reconocido");
					}
				  }
				  if(mensaje.type == nipc_error)     // si es de tipo error       HAY QUE CHARLAR SI MANEJAMOS ERRORES CON UN TIPO NUEVO O NO
				  {
					printf("ERROR: %d - %s\n",mensaje.payload.sector,mensaje.payload.contenido);
					//log_error(log, "Principal", "Message error: Sector:%d Error: %s", mensaje.payload.sector,mensaje.payload.contenido);
				  }
				}
				else // en caso de que la recepcion del mensaje retorne 0 o negativo significa que se ha perdido la conexion
				{
				  printf("Se cayo la conexion con el PFS: %d\n",aux_pfs->sock);
				  //log_warning(log, "Principal", "Message warning: Se cayo la conexion con el PFS:%d",aux_pfs->sock);
				  liberar_pfs_caido(&info_ppal->lista_pfs,aux_pfs->sock);  	// elimino la conexion caida de la lista de conexiones PFS
				  printf("------------------------------\n");
				}
		      }
		      aux_pfs = aux_pfs->sgte;
		    }*/

		    /*
		     * SOCKECT PRINCIPAL DE ESCUCHA
		     */
		    // Consulto si el socket principal tubo algun cambio

		    if(FD_ISSET(info_ppal->sock_ppd, &set_socket)>0)
		    {
		      if( (sock_new=accept(info_ppal->sock_ppd,NULL, NULL)) <0) // se acepta una nueva conexion
		      {
				printf("ERROR en la nueva conexion\n");
				//log_error(log, "Principal", "Message error: %s", "No se pudo establecer la conexion");
			  }
			  else
			  {
				if(recv_socket(&mensaje,sock_new)>=0)  // se recibe el mensaje de la nueva conexion
				{
				  if(mensaje.type == nipc_handshake)  // se consulta si es de tipo HANDSHAKE
				  {
					if(mensaje.len == 0)    // si el tamaño es distinto de 0 se trata de un PPD ya que el mensaje contiene el ID del disco
					{
					 	printf("Nueva conexion PFS: %d\n",sock_new);
						pfs *nuevo_pfs;
						int env;
						nuevo_pfs = (pfs *)malloc(sizeof(pfs));
						nuevo_pfs->sock=sock_new;
						nuevo_pfs->sgte = info_ppal->lista_pfs;
						info_ppal->lista_pfs = nuevo_pfs;
						FD_SET (nuevo_pfs->sock, &set_socket);  // se agrega la nueva conexion PFS a la lista de PFS
						//log_info(log, "Principal", "Message info: Nueva conexion PFS: %d", sock_new);
						/*nipc_packet buffer2;
						buffer2.type = 0;
						buffer2.len = 0;
						buffer2.payload.sector = -3;
						strcpy(buffer2.payload.contenido, "lllllllll");
						env = send_socket(&buffer2 ,nuevo_pfs->sock);
						printf("%s,%d, %d \n", buffer2.payload.contenido, nuevo_pfs->sock, env);
						printf("------------------------------\n");*/

						thidEscucharPedidos = pthread_create( &thEscucharPedidos, NULL, escucharPedidos, (void*) mensajet);

						thidAtenderpedidos = pthread_create( &thAtenderpedidos, NULL, atenderPedido, (void*) mensajet);
					  }
					  else
					  {
						//ENVIAR ERROR DE CONEXION
						//printf("No hay discos conectados!\n");
						//log_error(log, "Principal", "Message error: %s", "No hay discos conectados!");
						printf("cerrar conexion: %d\n",sock_new);
						nipc_close(sock_new);
					  }
					}
				  }
				  if(mensaje.type == nipc_req_read)  // no debe recibir ningun tipo lectura
				  {
					printf("ATENCION!!! Formato de mensaje no reconocido: %d - %d - %d - %s\n",mensaje.type, mensaje.len,
						mensaje.payload.sector, mensaje.payload.contenido);
					//log_warning(log, "Principal", "Message warning: Formato de mensaje no reconocido: %d - %d - %d - %s",
					//	mensaje.type, mensaje.len, mensaje.payload.sector, mensaje.payload.contenido);
					printf("------------------------------\n");
				  }
				  if(mensaje.type == nipc_req_write) // no debe recibir ningun tipo escritura
				  {
					printf("ATENCION!!! Formato de mensaje no reconocido: %d - %d - %d - %s\n",mensaje.type, mensaje.len,mensaje.payload.sector, mensaje.payload.contenido);
					//log_warning(log, "Principal", "Message warning: Formato de mensaje no reconocido: %d - %d - %d - %s",
					//	mensaje.type, mensaje.len, mensaje.payload.sector, mensaje.payload.contenido);
					printf("------------------------------\n");
				  }
				  if(mensaje.type == nipc_error)  // IGUAL QUE ANTES CON RESPECTO A LOS ERRORES
				  {
					printf("ERROR: %d - %s\n",mensaje.payload.sector,mensaje.payload.contenido);
					//log_error(log, "Principal", "Message error: Sector:%d Error: %s",
					//	  mensaje.payload.sector,mensaje.payload.contenido);
				  }
				}
		      }
		    }
		  printf("\n\n POR ALGO SALIII \n\n");
		  //log_warning(log, "Principal", "Message warning: Sali del sistema, cosa dificil");
		  //log_delete(log);
		  exit(EXIT_SUCCESS);

}

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
