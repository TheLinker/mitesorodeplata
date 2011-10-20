#ifndef __FUNC_H_
#define __FUNC_H_

#include <stdint.h>
#include "praid1.h"
#include "nipc.h"

void agregarPedidoLectura();
void agregarPedidoEscritura();
void agregarDisco(disco **discos, char id_disco[20],uint32_t sock);
void listarPedidosDiscos(disco **discos);
uint32_t menorCantidadPedidos(disco *discos);
void distribuirPedidoLectura(disco **discos,nipc_packet mensaje);
void distribuirPedidoEscritura(disco **discos);
uint32_t hayPedidosLectura();
uint32_t hayPedidosEscritura();
void estado();
void eliminarCola(disco **discos);
void *esperaRespuestas(uint32_t sock);

#endif //__FUNC_H_
