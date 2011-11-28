#include "conexionPFS.h"
#include "ppd.h"

pthread_t thEscucharPedidos;
int  thidEscucharPedidos;
char* mensajet = NULL;

void conectarConPFS(config_t vecConfig)
{
	datos               *info_ppal;
	pfs                 *aux_pfs;
	nipc_packet          mensaje;
	nipc_socket 		*socket;
	nipc_socket			sock_new;

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
						nipc_packet buffer;

					 	printf("Nueva conexion PFS: %d\n",sock_new);
						pfs *nuevo_pfs;
						//int env;
						nuevo_pfs = (pfs *)malloc(sizeof(pfs));
						nuevo_pfs->sock=sock_new;
						nuevo_pfs->sgte = info_ppal->lista_pfs;
						info_ppal->lista_pfs = nuevo_pfs;
						FD_SET (nuevo_pfs->sock, &set_socket);  // se agrega la nueva conexion PFS a la lista de PFS

						buffer.type = 0;
						buffer.len = 0;
						buffer.payload.sector = -3;
						strcpy(buffer.payload.contenido, "l");
						send_socket(&buffer ,sock_new);

						socket = malloc(sizeof(nipc_socket));
						*socket = sock_new;

						thidEscucharPedidos = pthread_create( &thEscucharPedidos, NULL, escucharPedidos, socket);

					  }
					  else
					  {
						//ENVIAR ERROR DE CONEXION
						printf("cerrar conexion: %d\n",sock_new);
						nipc_close(sock_new);
					  }
					}
				  }
				  if(mensaje.type == nipc_req_read)  // no debe recibir ningun tipo lectura
				  {
					printf("ATENCION!!! Formato de mensaje no reconocido: %d - %d - %d - %s\n",mensaje.type, mensaje.len,
						mensaje.payload.sector, mensaje.payload.contenido);

					printf("------------------------------\n");
				  }
				  if(mensaje.type == nipc_req_write) // no debe recibir ningun tipo escritura
				  {
					printf("ATENCION!!! Formato de mensaje no reconocido: %d - %d - %d - %s\n",mensaje.type, mensaje.len,mensaje.payload.sector, mensaje.payload.contenido);
					printf("------------------------------\n");
				  }
				  if(mensaje.type == nipc_error)  // IGUAL QUE ANTES CON RESPECTO A LOS ERRORES
				  {
					printf("ERROR: %d - %s\n",mensaje.payload.sector,mensaje.payload.contenido);
				  }
				}
		      }
		    }
		  printf("\n\n POR ALGO SALIII \n\n");
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
