#ifndef __FUNC_H_
#define __FUNC_H_

#include <stdint.h>
#include <pthread.h>
#include "praid1.h"
#include "nipc.h"

void         agregar_disco(datos **info_ppal, uint8_t *id_disco,nipc_socket sock_ppd);
void         listar_pedidos_discos(disco *discos);
nipc_socket  menor_cantidad_pedidos(disco *discos, uint32_t sector);
uint8_t     *distribuir_pedido_lectura(disco **discos,nipc_packet mensaje,nipc_socket sock_pfs);
void         distribuir_pedido_escritura(disco **discos,nipc_packet mensaje,nipc_socket sock_pfs);
void        *espera_respuestas(datos **info_ppal);
void         liberar_pfs_caido(pfs **pedidos_pfs,nipc_socket sock_pfs);
void         limpio_discos_caidos(datos **info_ppal);

#endif //__FUNC_H_
