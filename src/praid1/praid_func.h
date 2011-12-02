#ifndef __FUNC_H_
#define __FUNC_H_

#include <stdint.h>
#include <pthread.h>
#include "praid1.h"
#include "nipc.h"

void         agregar_disco(datos **info_ppal, uint8_t *id_disco,nipc_socket sock_ppd);
void         listar_pedidos_discos(disco *discos);
void         listar_discos(disco *discos);
nipc_socket  menor_cantidad_pedidos(disco *discos, int32_t sector);
nipc_socket  uno_y_uno(datos **info_ppal, int32_t sector);
uint8_t     *distribuir_pedido_lectura(datos **info_ppal,nipc_packet mensaje,nipc_socket sock_pfs);
void         distribuir_pedido_escritura(datos **info_ppal,nipc_packet mensaje,nipc_socket sock_pfs);
void        *espera_respuestas(datos **info_ppal);
void         liberar_pfs_caido(datos **info_ppal,nipc_socket sock_pfs);
uint16_t     limpio_discos_caidos(datos **info_ppal,nipc_socket sock_ppd);
void         insertar(sectores_t **lista, uint32_t sector);
int32_t      config_read(config_t *config);

#endif //__FUNC_H_
