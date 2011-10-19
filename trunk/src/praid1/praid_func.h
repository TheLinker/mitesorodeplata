#ifndef __FUNC_H_
#define __FUNC_H_

#include <stdint.h>
#include "praid1.h"

void agregarPedidoLectura();
void agregarPedidoEscritura();
void agregarDisco(disco **discos);
void listarPedidosDiscos(disco **discos);
uint32_t menorCantidadPedidos(disco *discos);
void distribuirPedidoLectura(disco **discos);
void distribuirPedidoEscritura(disco **discos);
uint32_t hayPedidosLectura();
uint32_t hayPedidosEscritura();
void estado();
void eliminarCola(disco **discos);
//void *distribuirPedidos(disco **discos);

#endif //__FUNC_H_
