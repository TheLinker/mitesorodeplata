#include "direccionamiento.h"
#include "nipc.h"
#include <string.h>
#include <stdlib.h>
#include <errno.h>
void hex_log(unsigned char * buff, int cantidad);

/**
 * Pide una *cantidad* de sectores a partir de un *sector* dado como base.
 *
 * @sector:   Primer sector a pedir.
 * @cantidad: Cantidad de sectores a pedir a partir de *sector*(inclusive).
 * @socket:   Socket de comunicacion.
 * @return    Código de error o 0 si fue todo bien.
 */
int8_t fat32_getsectors_pet(uint32_t sector, uint32_t cantidad, nipc_socket socket)
{
    nipc_packet packet;
    int i;

    packet.type = nipc_req_read;
    packet.len = sizeof(int32_t);
    packet.payload.sector = sector;

    //Pide los sectores necesarios.
    for (i = 0 ; i < cantidad ; i++) {
        send_socket(&packet, socket);
        packet.payload.sector += 1;
    }

    return 0;
}

/**
 * Obtiene una *cantidad* de sectores a partir de un *sector* dado como base.
 * NOTA: el *buffer* debe tener el tamaño suficiente para aceptar los datos.
 *
 * @sector:   Primer sector a pedir.
 * @cantidad: Cantidad de sectores a pedir a partir de *sector*(inclusive).
 * @buffer:   Lugar donde se almacena la información recibida.
 * @fs_tmp:   Estructura privada del file system // Ver Wiki.
 * @socket:   Socket de comunicacion.
 * @return    Primer sector obtenido o -1 en caso de error.
 */
int32_t fat32_getsectors_rsp(uint32_t sector, uint32_t cantidad, void *buffer, fs_fat32_t *fs_tmp, nipc_socket socket)
{
    int i;
    nipc_packet packet;
    int32_t ret=-1;

    for (i = 0 ; i < cantidad ; i++) {
        recv_socket(&packet, socket);
        //printf("%d\n", packet.payload.sector);
        memcpy(buffer+((packet.payload.sector - sector)*fs_tmp->boot_sector.bytes_per_sector), packet.payload.contenido, fs_tmp->boot_sector.bytes_per_sector);
        //hex_log(packet.payload.contenido, 512);
        if(packet.type == nipc_error)
        {
            log_error(fs_tmp->log, "", "%s en sector: %d", packet.payload.contenido, packet.payload.sector);
            return -EIO;
        } else
        if (packet.type != nipc_req_read)
        {
            log_error(fs_tmp->log, "", "Uat??: %d", packet.payload.sector);
            i--; continue;
        }

        if(ret<0) ret=packet.payload.sector;
    }

    return ret;
}

/**
 * Obtiene 2 sectores para terminar obteniendo un bloque.
 * NOTA: el *buffer* debe tener el tamaño suficiente para aceptar los datos.
 *
 * @buffer: Lugar donde se almacena la información recibida.
 * @fs_tmp: Estructura privada del file system // Ver Wiki.
 * @socket: Socket de comunicacion.
 * @return  Primer sector obtenido o -1 en caso de error.
 */
int32_t fat32_getsectors_rsp2(void *buffer, fs_fat32_t *fs_tmp, nipc_socket socket)
{
    int i;
    nipc_packet packet;
    int32_t ret=-1;

    for (i = 0 ; i < 2 ; i++) {
        recv_socket(&packet, socket);
        memcpy(buffer+((packet.payload.sector % 2)*fs_tmp->boot_sector.bytes_per_sector), packet.payload.contenido, fs_tmp->boot_sector.bytes_per_sector);

        if(packet.type == nipc_error)
        {
            log_error(fs_tmp->log, "", "%s en sector: %d", packet.payload.contenido, packet.payload.sector);
            return -EIO;
        } else
        if (packet.type != nipc_req_read)
        {
            log_error(fs_tmp->log, "", "Uat??: %d", packet.payload.sector);
            i--; continue;
        }

        if(ret<0) ret=packet.payload.sector;
    }

    return ret;
}

/**
 * Pide la escritura de una *cantidad* de sectores a partir de un *sector* dado como base.
 * NOTA: el *buffer* debe tener los datos a escribir.
 *
 * @sector:   Primer sector a escribir.
 * @cantidad: Cantidad de sectores a escribir a partir de *sector*(inclusive).
 * @buffer:   Lugar donde se almacena la información a escribir.
 * @fs_tmp:   Estructura privada del file system // Ver Wiki.
 * @socket:   Socket de comunicacion.
 * @return    Código de error o 0 si fue todo bien.
 */
uint8_t fat32_writesectors_pet(uint32_t sector, uint32_t cantidad, void *buffer, fs_fat32_t *fs_tmp, nipc_socket socket)
{
    nipc_packet packet;
    int i;

    packet.type = nipc_req_write;
    packet.len = sizeof(int32_t) + fs_tmp->boot_sector.bytes_per_sector;

    //Pide los sectores necesarios.
    for (i = 0 ; i < cantidad ; i++) {
        packet.payload.sector = sector + i;

        memcpy(packet.payload.contenido, buffer+(i*fs_tmp->boot_sector.bytes_per_sector), fs_tmp->boot_sector.bytes_per_sector);
        send_socket(&packet, socket);
    }

    return 0;
}

/**
 * Obtiene confirmacion de una operacion de escritura de una *cantidad* de sectores
 *
 * @cantidad: Cantidad de sectores a escribir a partir de *sector*(inclusive).
 * @fs_tmp:   Estructura privada del file system // Ver Wiki.
 * @socket:   Socket de comunicacion.
 * @return    Código de error o 0 si fue todo bien.
 */
uint8_t fat32_writesectors_rsp(uint32_t cantidad, fs_fat32_t *fs_tmp, nipc_socket socket)
{
    int i;
    for (i = 0 ; i < cantidad ; i++) {
        nipc_packet packet;
        recv_socket(&packet, socket);
        if(packet.type == nipc_error)
        {
            log_error(fs_tmp->log, "", "%s en sector: %d", packet.payload.contenido, packet.payload.sector);
            return -EIO;
        } /*else
        if (packet.type != nipc_req_write)
        {
            log_error(fs_tmp->log, "", "Uat??: %d", packet.payload.contenido, packet.payload.sector);
            return -EIO;
        }*/
    }

    return 0;
}

/**
 * Obtiene el bloque *block* (1024 bytes) y *cantidad* de bloques siguientes.
 *
 * @block:    Primer bloque a pedir.
 * @cantidad: Cantidad de bloques a pedir.
 * @buffer:   Lugar donde se almacena la información recibida.
 * @fid:      Id del archivo o NO_BUFFER para omitir el buffer.
 * @fs_tmp:   Estructura privada del file system// Ver Wiki.
 * @socket:   Socket de comunicacion.
 * @return:   Código de error o 0 si fue todo bien.
 */
uint8_t fat32_getblock(uint32_t block, uint32_t cantidad, void *buffer, int32_t fid, fs_fat32_t *fs_tmp, nipc_socket socket)
{
    //Si se llama desde un lugar sin cache
    if (fid == NO_CACHE || !fs_tmp->cache_size) {
        fat32_getsectors_pet(block * SECTORS_PER_BLOCK, cantidad * SECTORS_PER_BLOCK, socket);
        fat32_getsectors_rsp(block * SECTORS_PER_BLOCK, cantidad * SECTORS_PER_BLOCK, buffer, fs_tmp, socket);
        return 0;
    } else {
        int i=0;
        int8_t block_cache[BLOCK_SIZE];
        cache_t *cache = fs_tmp->open_files[fid].cache;
        
        sem_wait(&fs_tmp->open_files[fid].sem_cache);

        for(i=0;i<cantidad;i++)
            if(fat32_is_in_cache(block+i, cache, block_cache, fs_tmp) != -1) {
                memcpy(buffer+(i*BLOCK_SIZE), block_cache, BLOCK_SIZE);
            } else {

                //lugar donde guardar el bloque
                int32_t slot = fat32_get_cache_slot(cache, fs_tmp);
                int32_t rsp_block=0;

                //si el bloque a reemplazar fue modificado, lo escribo.
                if(cache[slot].modificado == 1)
                {
                    fat32_writeblock(cache[slot].number, 1, cache[slot].contenido, NO_CACHE, fs_tmp, socket);
                    cache[slot].modificado = 0;
                }

                fat32_getsectors_pet((block+i) * SECTORS_PER_BLOCK, SECTORS_PER_BLOCK, socket);

                //obtengo uno de los bloques
                rsp_block = fat32_getsectors_rsp2(block_cache, fs_tmp, socket) / SECTORS_PER_BLOCK;

                //lo almaceno en la cache
                fat32_save_in_cache(slot, rsp_block, block_cache, 0, cache);

                //Copio la resp en el buffer
                memcpy(buffer+((rsp_block-block)*BLOCK_SIZE), block_cache, BLOCK_SIZE);
            }

        sem_post(&fs_tmp->open_files[fid].sem_cache);

        return 0;
    }
}

/**
 * Escribe el bloque *block* (1024 bytes) y *cantidad* de bloques siguientes.
 *
 * @block:    Primer bloque a escribir.
 * @cantidad: Cantidad de bloques a escribir.
 * @buffer:   Lugar donde se almacena la información recibida.
 * @fid:      Id del archivo o NO_BUFFER para omitir el buffer.
 * @fs_tmp:   Estructura privada del file system// Ver Wiki.
 * @socket:   Socket de comunicacion.
 * @return:   Código de error o 0 si fue todo bien.
 */
uint8_t fat32_writeblock(uint32_t block, uint32_t cantidad, void *buffer, int32_t fid, fs_fat32_t *fs_tmp, nipc_socket socket)
{

    //Si se llama desde un lugar sin cache
    if (fid == NO_CACHE || !fs_tmp->cache_size) {
        fat32_writesectors_pet(block * SECTORS_PER_BLOCK, cantidad * SECTORS_PER_BLOCK, buffer, fs_tmp, socket);
        fat32_writesectors_rsp(cantidad * SECTORS_PER_BLOCK, fs_tmp, socket);
        return 0;
    } else {
        int i=0, rsp=0;
        cache_t *cache = fs_tmp->open_files[fid].cache;

        sem_wait(&fs_tmp->open_files[fid].sem_cache);

        for(i=0;i<cantidad;i++)
            if((rsp = fat32_is_in_cache(block+i, cache, NULL, fs_tmp)) != -1) {
                fat32_save_in_cache(rsp, block+i, buffer+(i*BLOCK_SIZE), 1, cache);
            } else {
                int32_t slot = fat32_get_cache_slot(cache, fs_tmp);

                //si el bloque a reemplazar fue modificado, lo escribo.
                if(cache[slot].modificado == 1)
                {
                    fat32_writeblock(cache[slot].number, 1, cache[slot].contenido, NO_CACHE, fs_tmp, socket);
                    cache[slot].modificado = 0;
                }

                fat32_save_in_cache(slot, block+i, buffer+(i*BLOCK_SIZE), 1, cache);
            }

        sem_post(&fs_tmp->open_files[fid].sem_cache);

        return 0;
    }
}

/**
 * Obtiene un *cluster*.
 * NOTA: el *buffer* debe tener el tamaño suficiente para aceptar los datos.
 *
 * @cluster: Cluster a pedir.
 * @buffer:  Lugar donde se almacena la información recibida.
 * @fid:     Id del archivo o NO_BUFFER para omitir el buffer.
 * @fs_tmp:  Estructura privada del file system// Ver Wiki.
 * @socket:  Socket de comunicacion.
 * @return:  Código de error o 0 si fue todo bien.
 */
uint8_t fat32_getcluster(uint32_t cluster, void *buffer, int32_t fid, fs_fat32_t *fs_tmp, nipc_socket socket)
{
    //cluster 0 y 1 estan reservados y es invalido pedir esos clusters
    if(cluster<2) return -EINVAL;

//// SSA =                  | RSC = Reserved Sector Count  | FN = Number of FAT
//// SF = sectors per FAT   | LSN = Logical Sector Number  | CN = Cluster Number  | SC = Sectors per Cluster

////  SSA=RSC(0x0E) + FN(0x10) * SF(0x24)
////  LSN=SSA + (CN-2) × SC(0x0D)

    uint32_t block_number = 0;

    block_number = (fs_tmp->system_area_size + (cluster - 2) * fs_tmp->boot_sector.sectors_per_cluster) / SECTORS_PER_BLOCK;

    fat32_getblock(block_number,
                   fs_tmp->boot_sector.sectors_per_cluster / SECTORS_PER_BLOCK,
                   buffer, fid, fs_tmp, socket);

    return 0;
}

/**
 * Escribe un *cluster*.
 * NOTA: el *buffer* debe tener el tamaño suficiente para aceptar los datos.
 *
 * @cluster: Cluster a escribir.
 * @buffer:  Lugar donde se almacena la información a escribir.
 * @fid:     Id del archivo o NO_BUFFER para omitir el buffer.
 * @fs_tmp:  Estructura privada del file system// Ver Wiki.
 * @socket:  Socket de comunicacion.
 * @return:  Código de error o 0 si fue todo bien.
 */
uint8_t fat32_writecluster(uint32_t cluster, void *buffer, int32_t fid, fs_fat32_t *fs_tmp, nipc_socket socket)
{
    //cluster 0 y 1 estan reservados y es invalido pedir esos clusters
    if(cluster<2) return -EINVAL;

//// SSA = Size of System Area   | RSC = Reserved Sector Count  | FN = Number of FAT
//// SF = sectors per FAT        | LSN = Logical Sector Number  | CN = Cluster Number  | SC = Sectors per Cluster

////  SSA=RSC(0x0E) + FN(0x10) * SF(0x24)
////  LSN=SSA + (CN-2) × SC(0x0D)

    uint32_t block_number = 0;

    block_number = (fs_tmp->system_area_size + (cluster - 2) * fs_tmp->boot_sector.sectors_per_cluster) / SECTORS_PER_BLOCK;

    fat32_writeblock(block_number,
                     fs_tmp->boot_sector.sectors_per_cluster / SECTORS_PER_BLOCK,
                     buffer, fid, fs_tmp, socket);

    return 0;
}


