#include "direccionamiento.h"
#include "nipc.h"
#include <string.h>
#include <stdlib.h>
#include <errno.h>

/**
 * Obtiene *cantidad* sectores a partir de *sector*
 * NOTA: el *buffer* debe tener el tamaño suficiente para aceptar los datos
 *
 * @sector primer sector a pedir
 * @cantidad cantidad de sectores a partir de *sector* a pedir (inclusive)
 * @buffer lugar donde guardar la información recibida.
 * @fs_tmp estructura privada del file system
 * @return codigo de error o 0 si fue todo bien.
 */
uint8_t fat32_getsectors(uint32_t sector, uint32_t cantidad, void *buffer, fs_fat32_t *fs_tmp)
{
    nipc_packet packet;
    int i;

    packet.type = nipc_req_packet;
    packet.len = sizeof(int32_t);
    memcpy(packet.payload, &sector, sizeof(int32_t));

    //Pide los sectores necesarios
    for (i = 0 ; i < cantidad ; i++) {
        nipc_send_packet(&packet, fs_tmp->socket);
        *((int32_t *)packet.payload) += 1;
    }

    //Espera a obtener todos los sectores pedidos
    for (i = 0 ; i < cantidad ; i++) {
        nipc_packet *packet = nipc_recv_packet(fs_tmp->socket);
        int32_t rta_sector;
        memcpy(&rta_sector, packet->payload, sizeof(int32_t));
        memcpy(buffer+((rta_sector-sector)*fs_tmp->boot_sector.bytes_per_sector), packet->payload+4, fs_tmp->boot_sector.bytes_per_sector);
        free(packet);
    }

    return 0;
}

/**
 * Obtiene un *cluster*
 * NOTA: el *buffer* debe tener el tamaño suficiente para aceptar los datos
 *
 * @cluster cluster a pedir
 * @buffer lugar donde guardar la información recibida.
 * @fs_tmp estructura privada del file system
 * @return codigo de error o 0 si fue todo bien.
 */
uint8_t fat32_getcluster(uint32_t cluster, void *buffer, fs_fat32_t *fs_tmp)
{
    //cluster 0 y 1 estan reservados y es invalido pedir esos clusters
    if(cluster<2) return -EINVAL;

////  SSA=RSC(0x0E) + FN(0x10) * SF(0x24)
////  LSN=SSA + (CN-2) × SC(0x0D)

    uint32_t logical_sector_number = 0;

    logical_sector_number = fs_tmp->system_area_size + (cluster - 2) * fs_tmp->boot_sector.sectors_per_cluster;

    fat32_getsectors(logical_sector_number , fs_tmp->boot_sector.sectors_per_cluster, buffer, fs_tmp);

    return 0;
}


