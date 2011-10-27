#ifndef __FUNC_H_
#define __FUNC_H_

#include <stdint.h>
#include <pthread.h>
#include "praid1.h"
#include "nipc.h"

void       agregarPedidoLectura();
void       agregarPedidoEscritura();
void       agregarDisco(disco **discos, uint8_t id_disco[20],nipc_socket sock);
void       listarPedidosDiscos(disco **discos);
uint32_t   menorCantidadPedidos(disco *discos);
uint8_t   *distribuirPedidoLectura(disco **discos,nipc_packet mensaje);
void       distribuirPedidoEscritura(disco **discos,nipc_packet mensaje);
uint32_t   hayPedidosLectura();
uint32_t   hayPedidosEscritura();
void       estado();
void       eliminarCola(disco **discos);
void      *espera_respuestas(disco *su_disco);
void       liberar_pfs_caido(pfs **pedidos_pfs,nipc_socket sock);
pthread_t  crear_hilo_respuestas();
void      *enviar_respuetas(void *nada);
#endif //__FUNC_H_
