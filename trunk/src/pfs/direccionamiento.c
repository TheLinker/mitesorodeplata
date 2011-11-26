#include "direccionamiento.h"
#include "nipc.h"
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "pfs.h"
//Hay que cambiar las func para que acepten un socket y hagan las cosas con ese socket

/**
 * Obtiene una *cantidad* de sectores a partir de un *sector* dado como base.
 * NOTA: el *buffer* debe tener el tamaño suficiente para aceptar los datos.
 *
 * @sector:     Primer sector a pedir.
 * @cantidad:   Cantidad de sectores a pedir a partir de *sector*(inclusive).
 * @buffer:     Lugar donde se almacena la información recibida.
 * @fs_tmp:     Estructura privada del file system // Ver Wiki.
 * @return      Código de error o 0 si fue todo bien.
 */
uint8_t fat32_getsectors(uint32_t sector, uint32_t cantidad, void *buffer, struct fs_fat32_t *fs_tmp)
{
    nipc_packet packet;
    int i;

    packet.type = nipc_req_read;
    packet.len = sizeof(int32_t);
    packet.payload.sector = sector;

    //Pide los sectores necesarios.
    for (i = 0 ; i < cantidad ; i++) {
//	log_info(fs_tmp->log, "", "Enviado paqete (Sector: %d).", packet.payload.sector );
        send_socket(&packet, fs_tmp->socket);
        packet.payload.sector += 1;
    }

    //Espera a obtener todos los sectores pedidos.
    for (i = 0 ; i < cantidad ; i++) {
        nipc_packet packet;
        recv_socket(&packet, fs_tmp->socket);
        memcpy(buffer+((packet.payload.sector - sector)*fs_tmp->boot_sector.bytes_per_sector), packet.payload.contenido, fs_tmp->boot_sector.bytes_per_sector);

    }
//int j=0;
//        for(j=0;j<fs_tmp->boot_sector.bytes_per_sector *2;j++)
//            printf("%.2X%c", ((uint8_t *) buffer)[j], ((j+1)%16)?' ':'\n');

    return 0;
}

/**
 * Escribe una *cantidad* de sectores a partir de un *sector* dado como base.
 * NOTA: el *buffer* debe tener los datos a escribir.
 *
 * @sector:     Primer sector a escribir.
 * @cantidad:   Cantidad de sectores a escribir a partir de *sector*(inclusive).
 * @buffer:     Lugar donde se almacena la información a escribir.
 * @fs_tmp:     Estructura privada del file system // Ver Wiki.
 * @return      Código de error o 0 si fue todo bien.
 */
uint8_t fat32_writesectors(uint32_t sector, uint32_t cantidad, void *buffer, struct fs_fat32_t *fs_tmp)
{
    nipc_packet packet;
    int i;

    packet.type = nipc_req_write;
    packet.len = sizeof(int32_t) + fs_tmp->boot_sector.bytes_per_sector;

    //Pide los sectores necesarios.
    for (i = 0 ; i < cantidad ; i++) {
        packet.payload.sector = sector + i;
        
        memcpy(packet.payload.contenido, buffer+(i*fs_tmp->boot_sector.bytes_per_sector), fs_tmp->boot_sector.bytes_per_sector);
        send_socket(&packet, fs_tmp->socket);
    }

    //Espera a obtener la confirmacion de la escritura.
    for (i = 0 ; i < cantidad ; i++) {
        nipc_packet packet; 
        recv_socket(&packet, fs_tmp->socket);
        if(packet.type == nipc_error)
        {
            log_error(fs_tmp->log, "", "%s en sector: %d", packet.payload.contenido, packet.payload.sector);
            return -EIO;
        }
    }

    return 0;
}

/**
 * Obtiene el bloque *block* (1024 bytes) y *cantidad* de bloques siguientes.
 *
 * @block:    Primer bloque a pedir.
 * @cantidad: Cantidad de bloques a pedir.
 * @buffer:   Lugar donde se almacena la información recibida.
 * @fs_tmp:   Estructura privada del file system// Ver Wiki.
 * @return:   Código de error o 0 si fue todo bien.
 */
uint8_t fat32_getblock(uint32_t block, uint32_t cantidad, void *buffer, struct fs_fat32_t *fs_tmp)
{
    //int i=0;
    //for( i = 0 ; i < cantidad ; i++, block++)
    //    fat32_getsectors(block * SECTORS_PER_BLOCK, SECTORS_PER_BLOCK, buffer + (i * BLOCK_SIZE), fs_tmp);
    fat32_getsectors(block * SECTORS_PER_BLOCK, cantidad * SECTORS_PER_BLOCK, buffer /*+ (i * BLOCK_SIZE)*/, fs_tmp);

    return 0;
}

/**
 * Escribe el bloque *block* (1024 bytes) y *cantidad* de bloques siguientes.
 *
 * @block:    Primer bloque a escribir.
 * @cantidad: Cantidad de bloques a escribir.
 * @buffer:   Lugar donde se almacena la información recibida.
 * @fs_tmp:   Estructura privada del file system// Ver Wiki.
 * @return:   Código de error o 0 si fue todo bien.
 */
uint8_t fat32_writeblock(uint32_t block, uint32_t cantidad, void *buffer, struct fs_fat32_t *fs_tmp)
{
        printf("Pedido escritura bloque: %d\n", block);
    //int i=0;
    //for( i = 0 ; i < cantidad ; i++, block++)
    //    fat32_writesectors(block * SECTORS_PER_BLOCK, SECTORS_PER_BLOCK, buffer + (i * BLOCK_SIZE), fs_tmp);
    fat32_writesectors(block * SECTORS_PER_BLOCK, cantidad * SECTORS_PER_BLOCK, buffer /*+ (i * BLOCK_SIZE)*/, fs_tmp);

    return 0;
}

