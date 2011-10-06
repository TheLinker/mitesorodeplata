#include "utils.h"
#include "direccionamiento.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

/**
 * Obtiene la cantidad de clusters libres en la FAT
 *
 * @fs_tmp estructura privada del file system
 * @return cantidad de clusteres libres en la FAT
 */
uint32_t fat32_free_clusters(fs_fat32_t *fs_tmp)
{
    if(fs_tmp->fsinfo_sector.free_clusters < 0)
    {
        int i=0, free_clusters=0;

        for ( i = 2 ; i < fs_tmp->boot_sector.bytes_per_sector * fs_tmp->boot_sector.sectors_per_fat / sizeof(int32_t) ; i++ )
            if(fs_tmp->fat[i] == 0)
                free_clusters++;

        fs_tmp->fsinfo_sector.free_clusters = free_clusters;

        //TODO actualizar fs_info_sector del volumen

        return free_clusters;
    } else {
        return fs_tmp->fsinfo_sector.free_clusters;
    }

}

/**
 * Obtiene el primer cluster libre en la tabla fat
 *
 * @fs_tmp estructura privada del file system
 * @return primer cluster libre o un numero menor a cero en caso de error
 */
int32_t fat32_first_free_cluster(fs_fat32_t *fs_tmp)
{
    uint32_t i=0;

    for ( i = 2 ; i < fs_tmp->boot_sector.bytes_per_sector * fs_tmp->boot_sector.sectors_per_fat / sizeof(int32_t) ; i++ )
        if(fs_tmp->fat[i] == 0)
            return i;

    return -ENOSPC; //Si no hay mas clusters libres, devuelve este error, este error esta zarpado en gato.
}

/**
 * Envia un handshake al proceso RAID o DISCO
 *
 * @return en caso de ser rechazada la conexion,
 * envia el mensaje de error y sale del programa
 */
void fat32_handshake(nipc_socket *socket)
{
    nipc_packet *packet = malloc(sizeof(nipc_packet));

    packet->type = nipc_handshake;
    packet->len = 0;
    nipc_send_packet(packet, socket);
    free(packet);

    packet = nipc_recv_packet(socket);

    if (packet->type)
    {
        printf("Error: tipo %d recibido en vez del handshake\n", packet->type);
        exit(-ECONNREFUSED);
    }

    if (packet->len)
    {
        printf("Error: %s\n", packet->payload);
        exit(-ECONNREFUSED);
    }

    free(packet);

    return;
}

/**
 * Añade un cluster al final de la cadena donde esta first_cluster
 *
 * @fs_tmp estructura privada del file system
 * @first_cluster un cluster perteneciente a la cadena donde se desea agregar otro cluster
 */
void fat32_add_cluster(int32_t first_cluster, fs_fat32_t *fs_tmp)
{
    int32_t posicion = first_cluster;

    while(fs_tmp->fat[posicion] != fs_tmp->eoc_marker)
    {
        posicion = fs_tmp->fat[posicion];
    }

    int32_t free_cluster = fat32_first_free_cluster(fs_tmp);

    fs_tmp->fat[posicion] = free_cluster;
    fs_tmp->fat[free_cluster] = fs_tmp->eoc_marker;

    fs_tmp->fsinfo_sector.free_clusters--;
    // Falta hacer la escritura en Disco de la modificacion de la FAT y del FSinfo

}

/**
 * Elimina un cluster al final de la cadena donde esta first_cluster
 *
 * NOTA: si como argumento se pasa el ultimo cluster el algoritmo no hace nada
 *
 * @fs_tmp estructura privada del file system
 * @first_cluster un cluster perteneciente a la cadena donde se desea eliminar el ultimo cluster
 */
void fat32_remove_cluster(int32_t first_cluster, fs_fat32_t *fs_tmp)
{
    int32_t pos_act = first_cluster;
    int32_t pos_ant = 0;

    while(fs_tmp->fat[pos_act] != fs_tmp->eoc_marker)
    {
        pos_ant = pos_act;
        pos_act = fs_tmp->fat[pos_act];
    }

    if (pos_ant == 0)
        return;

    fs_tmp->fat[pos_ant] = fs_tmp->eoc_marker;
    fs_tmp->fat[pos_act] = 0;

    fs_tmp->fsinfo_sector.free_clusters++;
    // Faltar hacer la escritura en Disco de la modificacion de la FAT y del FSinfo
}

/**
 * Lee la configuracion
 *
 * @fs_tmp estructura privada del file system
 */
int fat32_config_read(fs_fat32_t *fs_tmp)
{
    char line[1024], w1[1024], w2[1024];
    FILE *fp;

    memcpy((char *) (fs_tmp->server_host), "localhost", 10);
    fs_tmp->server_port = 1337;
    fs_tmp->cache_size = 1024;

    fp = fopen("pfs.conf","r");
    if( fp == NULL )
    {
        printf("No se encuentra el archivo de configuración\n");
        return 1;
    }

    while( fgets(line, sizeof(line), fp) )
    {
        char* ptr;

        if( line[0] == '/' && line[1] == '/' )
            continue;
        if( (ptr = strstr(line, "//")) != NULL )
            *ptr = '\n'; //Strip comments
        if( sscanf(line, "%[^:]: %[^\t\r\n]", w1, w2) < 2 )
            continue;

        //Strip trailing spaces
        ptr = w2 + strlen(w2);
        while (--ptr >= w2 && *ptr == ' ');
        ptr++;
        *ptr = '\0';
            
        if(strcmp(w1,"host")==0)
            strcpy((char *)(fs_tmp->server_host), w2);
        else 
        if(strcmp(w1,"puerto")==0)
            fs_tmp->server_port = atoi(w2);
        else
        if(strcmp(w1,"tamanio_cache")==0)
            fs_tmp->cache_size = atoi(w2);
        else
            printf("Configuracion desconocida:'%s'\n", w1);
    }

    fclose(fp);
    return 0;
}

/**
 * Llena el buffer con la entrada numero entry_number a partir de first_cluster.
 *
 * NOTA: el buffer tiene que estar asignada memoria.
 *
 * @entry_number numero de entrada solicitada a partir de un cluster dado
 * @first_cluster primer cluster de la cadena
 * @buffer destino de los 32 bytes de la entrada solicitada
 * @fs_tmp estructura privada del file system
 * @return 0 si exito
 *         -EINVAL en caso de error
 */
int8_t fat32_get_entry(int32_t entry_number, int32_t first_cluster, int8_t *buffer, fs_fat32_t *fs_tmp)
{
    int32_t cluster_offset = entry_number / (fs_tmp->boot_sector.bytes_per_sector *
                                             fs_tmp->boot_sector.sectors_per_cluster / 32);

    int32_t entry_offset = entry_number % (fs_tmp->boot_sector.bytes_per_sector *
                                           fs_tmp->boot_sector.sectors_per_cluster / 32);

    int32_t cluster = fat32_get_link_n_in_chain(first_cluster, cluster_offset, fs_tmp);
    if (cluster < 0)
        return cluster;

    int8_t *cluster_buffer = calloc(fs_tmp->boot_sector.bytes_per_sector, fs_tmp->boot_sector.sectors_per_cluster);
    fat32_getcluster(cluster, cluster_buffer, fs_tmp);
    memcpy(buffer, cluster_buffer + (entry_offset * 32), 32);
    free(cluster_buffer);

    return 0;
}

/**
 * Obtiene el numero de cluster en la cadena a partir de first_cluster, desplazado cluster_offset clusters
 *
 * @first_cluster primer cluster de la cadena
 * @cluster_offset posicion relativa del cluster dentro de la lista enlazada
 * @fs_tmp estructura privada del file system
 * @return numero de cluster de la cadena a partir de first_cluster, desplazado cluster_offset clusters
 *         -EINVAL en caso de error
 */
int32_t fat32_get_link_n_in_chain(int32_t first_cluster, int32_t cluster_offset, fs_fat32_t *fs_tmp)
{
    int32_t cluster = first_cluster;

    while(cluster_offset > 0 && cluster != fs_tmp->eoc_marker)
    {
        cluster = fs_tmp->fat[cluster];
        cluster_offset--;
    }

    if (cluster_offset > 0)
        return -EINVAL;

    return cluster;
}

