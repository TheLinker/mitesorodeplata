#include "utils.h"
#include "pfs.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

/**
 * Obtiene la cantidad de clusters libres en la FAT
 *
 * @return cantidad de clusteres libres en la FAT
 */
uint32_t fat32_free_clusters()
{
    if(fat.fsinfo_sector.free_clusters < 0)
    {
        int i=0, free_clusters=0;

        for ( i = 2 ; i < fat.boot_sector.bytes_per_sector * fat.boot_sector.sectors_per_fat / sizeof(int32_t) ; i++ )
            if(fat.fat[i] == 0)
                free_clusters++;

        fat.fsinfo_sector.free_clusters = free_clusters;

        //TODO actualizar fs_info_sector del volumen

        return free_clusters;
    } else {
        return fat.fsinfo_sector.free_clusters;
    }

}

/**
 * Obtiene el primer cluster libre en la tabla fat
 *
 * @return primer cluster libre o un numero menor a cero en caso de error
 */
int32_t fat32_first_free_cluster()
{
    uint32_t i=0;

    for ( i = 2 ; i < fat.boot_sector.bytes_per_sector * fat.boot_sector.sectors_per_fat / sizeof(int32_t) ; i++ )
        if(fat.fat[i] == 0)
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
 * @first_cluster un cluster perteneciente a la cadena donde se desea agregar otro cluster
 */
void fat32_add_cluster(int32_t first_cluster)
{
    int32_t posicion = first_cluster;

    while(fat.fat[posicion] != fat.eoc_marker)
    {
        posicion = fat.fat[posicion];
    }

    int32_t free_cluster = fat32_first_free_cluster();

    fat.fat[posicion] = free_cluster;
    fat.fat[free_cluster] = fat.eoc_marker;

    fat.fsinfo_sector.free_clusters--;
    // Falta hacer la escritura en Disco de la modificacion de la FAT y del FSinfo

}

/**
 * Elimina un cluster al final de la cadena donde esta first_cluster
 *
 * NOTA: si como argumento se pasa el ultimo cluster el algoritmo no hace nada
 *
 * @first_cluster un cluster perteneciente a la cadena donde se desea eliminar el ultimo cluster
 */
void fat32_remove_cluster(int32_t first_cluster)
{
    int32_t pos_act = first_cluster;
    int32_t pos_ant = 0;

    while(fat.fat[pos_act] != fat.eoc_marker)
    {
        pos_ant = pos_act;
        pos_act = fat.fat[pos_act];
    }

    if (pos_ant == 0)
        return;

    fat.fat[pos_ant] = fat.eoc_marker;
    fat.fat[pos_act] = 0;

    fat.fsinfo_sector.free_clusters++;
    // Faltar hacer la escritura en Disco de la modificacion de la FAT y del FSinfo
}

int fat32_config_read()
{
    char line[1024], w1[1024], w2[1024];
    FILE *fp;

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
            strcpy((char *)server_host, w2);
        else 
        if(strcmp(w1,"puerto")==0)
            server_port = atoi(w2);
        else
        if(strcmp(w1,"tamanio_cache")==0)
            cache_size = atoi(w2);
        else
            printf("Configuracion desconocida:'%s'\n", w1);
    }

    fclose(fp);
    return 0;
}

