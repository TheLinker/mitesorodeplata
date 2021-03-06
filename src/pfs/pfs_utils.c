#include "pfs_utils.h"
#include "utils.h"
#include "direccionamiento.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>

/**
 * Obtiene la cantidad de clusters libres en la FAT
 *
 * @fs_tmp estructura privada del file system
 * @return cantidad de clusteres libres en la FAT
 */
uint32_t fat32_free_clusters(fs_fat32_t *fs_tmp)
{
    int i=0, free_clusters=0;

    for ( i = 2 ; i < fs_tmp->boot_sector.bytes_per_sector *
                      fs_tmp->boot_sector.sectors_per_fat / sizeof(int32_t) ; i++ )
        if(fs_tmp->fat[i] == 0)
            free_clusters++;

    return free_clusters;
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
void fat32_handshake(nipc_socket socket)
{
    nipc_packet packet;
    memset(&packet, '\0', sizeof(nipc_packet));

    packet.type = nipc_handshake;
    packet.len = 0;
    send_socket(&packet, socket);

    recv_socket(&packet, socket);

    if (packet.type != nipc_handshake) {
        printf("Error: tipo %d recibido en vez del handshake\n", packet.type);
        exit(-ECONNREFUSED);
    }

    if (packet.len) {
        printf("Error %d: %s\n", packet.payload.sector, packet.payload.contenido);
        exit(-ECONNREFUSED);
    }

    return;
}

/**
 * Añade un cluster al final de la cadena donde esta first_cluster
 *
 * @fs_tmp estructura privada del file system
 * @first_cluster un cluster perteneciente a la cadena donde se desea agregar otro cluster
 */
void fat32_add_cluster(int32_t first_cluster, fs_fat32_t *fs_tmp, nipc_socket socket)
{
    int32_t posicion = first_cluster;

    while(fs_tmp->fat[posicion] != fs_tmp->eoc_marker) {
        posicion = fs_tmp->fat[posicion];
    }

    sem_wait(&fs_tmp->mux_fat);
        int32_t free_cluster = fat32_first_free_cluster(fs_tmp);

        fs_tmp->fat[posicion] = free_cluster;
        fs_tmp->fat[free_cluster] = fs_tmp->eoc_marker;
    sem_post(&fs_tmp->mux_fat);

    int32_t bloque1 = (fs_tmp->boot_sector.reserved_sectors + (posicion / (fs_tmp->boot_sector.bytes_per_sector / sizeof(int32_t)))) / SECTORS_PER_BLOCK;
    int32_t bloque2 = (fs_tmp->boot_sector.reserved_sectors + (free_cluster / (fs_tmp->boot_sector.bytes_per_sector / sizeof(int32_t)))) / SECTORS_PER_BLOCK;

    fat32_writeblock(bloque1, 1, (fs_tmp->fat) + (bloque1 - (fs_tmp->boot_sector.reserved_sectors/SECTORS_PER_BLOCK)) * (BLOCK_SIZE/sizeof(int32_t)), NO_CACHE, fs_tmp, socket);
    if(bloque2 != bloque1)
        fat32_writeblock(bloque2, 1, (fs_tmp->fat) + ((bloque2 - (fs_tmp->boot_sector.reserved_sectors/SECTORS_PER_BLOCK)) * (BLOCK_SIZE/sizeof(int32_t))), NO_CACHE, fs_tmp, socket);

    //Inicializamos los datos del nuevo cluster a 0
    char buffer[fs_tmp->boot_sector.sectors_per_cluster * fs_tmp->boot_sector.bytes_per_sector];
    memset(buffer, '\0', fs_tmp->boot_sector.sectors_per_cluster * fs_tmp->boot_sector.bytes_per_sector);
    fat32_writecluster(free_cluster, buffer, NO_CACHE, fs_tmp, socket);
}

/**
 * Elimina un cluster al final de la cadena donde esta first_cluster
 *
 * NOTA: si como argumento se pasa el ultimo cluster el algoritmo no hace nada
 *
 * @fs_tmp estructura privada del file system
 * @first_cluster un cluster perteneciente a la cadena donde se desea eliminar el ultimo cluster
 */
void fat32_remove_cluster(int32_t first_cluster, fs_fat32_t *fs_tmp, nipc_socket socket)
{
    int32_t pos_act = first_cluster;
    int32_t pos_ant = 0;

    while(fs_tmp->fat[pos_act] != fs_tmp->eoc_marker) {
        pos_ant = pos_act;
        pos_act = fs_tmp->fat[pos_act];
    }

    if (pos_ant == 0)
        return;

    sem_wait(&fs_tmp->mux_fat);
        fs_tmp->fat[pos_ant] = fs_tmp->eoc_marker;
        fs_tmp->fat[pos_act] = 0;
    sem_post(&fs_tmp->mux_fat);

    int32_t bloque1 = (fs_tmp->boot_sector.reserved_sectors + (pos_ant / (fs_tmp->boot_sector.bytes_per_sector / sizeof(int32_t)))) / SECTORS_PER_BLOCK;
    int32_t bloque2 = (fs_tmp->boot_sector.reserved_sectors + (pos_act / (fs_tmp->boot_sector.bytes_per_sector / sizeof(int32_t)))) / SECTORS_PER_BLOCK;

    fat32_writeblock(bloque1, 1, fs_tmp->fat + ((bloque1 - (fs_tmp->boot_sector.reserved_sectors/SECTORS_PER_BLOCK)) * (BLOCK_SIZE/sizeof(int32_t))), NO_CACHE, fs_tmp, socket);
    if(bloque2 != bloque1)
        fat32_writeblock(bloque2, 1, fs_tmp->fat + ((bloque2 - (fs_tmp->boot_sector.reserved_sectors/SECTORS_PER_BLOCK)) * (BLOCK_SIZE/sizeof(int32_t))), NO_CACHE, fs_tmp, socket);
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
    fs_tmp->cache_size = 0;
    memcpy((char *) (fs_tmp->log_path), "/tmp/pfs.log", 13);
    fs_tmp->log_mode = LOG_OUTPUT_CONSOLEANDFILE;

    fp = fopen("pfs.conf","r");
    if( fp == NULL ) {
        printf("No se encuentra el archivo de configuración\n");
        return 1;
    }

    while( fgets(line, sizeof(line), fp) ) {
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
        if(strcmp(w1,"log_path")==0)
            strcpy(fs_tmp->log_path, w2);
        else
        if(strcmp(w1,"log_mode")==0)
        {
            if(strcmp(w1,"none") == 0)
                fs_tmp->log_mode = LOG_OUTPUT_NONE;
            else
            if(strcmp(w1,"console") == 0)
                fs_tmp->log_mode = LOG_OUTPUT_CONSOLE;
            else
            if(strcmp(w1,"file") == 0)
                fs_tmp->log_mode = LOG_OUTPUT_FILE;
            else
            if(strcmp(w1,"file+console") == 0)
                fs_tmp->log_mode = LOG_OUTPUT_CONSOLEANDFILE;
        }
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
int8_t fat32_get_entry(int32_t entry_number, int32_t first_cluster, file_entry_t *buffer, fs_fat32_t *fs_tmp, nipc_socket socket)
{
    int32_t cluster_offset = entry_number / (fs_tmp->boot_sector.bytes_per_sector *
                                             fs_tmp->boot_sector.sectors_per_cluster / 32);

    int32_t entry_offset = entry_number % (fs_tmp->boot_sector.bytes_per_sector *
                                           fs_tmp->boot_sector.sectors_per_cluster / 32);

    int32_t cluster = fat32_get_link_n_in_chain(first_cluster, cluster_offset, fs_tmp);
    if (cluster < 0)
        return cluster;

    int8_t *cluster_buffer = calloc(fs_tmp->boot_sector.bytes_per_sector, fs_tmp->boot_sector.sectors_per_cluster);
    fat32_getcluster(cluster, cluster_buffer, NO_CACHE, fs_tmp, socket);
    memcpy((int8_t*)buffer, cluster_buffer + (entry_offset * 32), 32);
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

    while((cluster_offset > 0) && (cluster != fs_tmp->eoc_marker)) {
        cluster = fs_tmp->fat[cluster];
        cluster_offset--;
    }

    if (cluster_offset > 0)
        return -EINVAL;

    return cluster;
}

/**
 * Obtiene una lista de archivos (file_attrs *ret_list) a partir de del cluster first_cluster.
 *
 * @first_cluster primer cluster en la cadena.
 * @ret_list si != NULL: 'buffer' donde almacenar cada entrada de archivo.
 *                       Debe estar asignado con tamaño suficiente para contener todas las entradas.
 *           si == NULL: la funcion solo devuelve la cantidad de entradas de directorio encontradas.
 * @fs_tmp estructura privada del file system.
 * @return cantidad de entradas de directorio halladas.
 */
uint32_t fat32_get_file_list(int32_t first_cluster, file_attrs *ret_list, fs_fat32_t *fs_tmp, nipc_socket socket)
{
    uint8_t directory_entry[32];
    file_attrs nuevo_archivo;
    memset(&nuevo_archivo, '\0', sizeof(file_attrs));
    uint16_t utf16_filename[256];
    uint32_t utf16_filename_len = 0;
    uint32_t cantidad_entradas=0;
    uint32_t current_entry=0;
    uint8_t  checksum=0;

    fat32_get_entry(current_entry, first_cluster, (file_entry_t *)directory_entry, fs_tmp, socket);

    while ( directory_entry[0] != AVAIL_ENTRY ) {
        switch( ((file_entry_t *)directory_entry)->file_attr ) {
            case ATTR_LONG_FILE_NAME:
                if(!ret_list) break;

                if(directory_entry[0] & DELETED_LFN)
                    break;

                //orden de la entrada de nombre largo en el nombre largo
                int8_t orden_lfn = (directory_entry[0] & 0x1F);

                if(directory_entry[0] & LAST_LFN_ENTRY)
                    checksum = ((lfn_entry_t *)directory_entry)->checksum;

                if(checksum != ((lfn_entry_t *)directory_entry)->checksum)
                    printf("El checksum anterior (0x%.2X) no coincide con el nuevo (0x%.2X) - Entrada: %d Cluster: %d\n",
                            checksum, 
                            ((lfn_entry_t *)directory_entry)->checksum,
                            current_entry,
                            first_cluster);

                //esto es para meter los 3 pedazos de nombre esparcidos alrededor del lfn y reconstruir el nombre largo.
                memcpy(utf16_filename+(13*(orden_lfn-1)+11), ((lfn_entry_t *)directory_entry)->lfn_name3, 2 * sizeof(uint16_t));
                memcpy(utf16_filename+(13*(orden_lfn-1)+5), ((lfn_entry_t *)directory_entry)->lfn_name2, 6 * sizeof(uint16_t));
                memcpy(utf16_filename+(13*(orden_lfn-1)), ((lfn_entry_t *)directory_entry)->lfn_name1, 5 * sizeof(uint16_t));

                break;

            case ATTR_SUBDIRECTORY:
            case ATTR_ARCHIVE:
                if(directory_entry[0] == DELETED_FILE)
                    break;

                //si llegamos hasta aca, la entrada es un archivo valido
                cantidad_entradas++;

                if(!ret_list) break;

                if(directory_entry[0] == 0x05) directory_entry[0] = 0xE5;

                utf16_filename_len = unicode_strlen(utf16_filename);

                unicode_utf16_to_utf8_inbuffer(utf16_filename, utf16_filename_len * sizeof(uint16_t),
                                               (uint8_t *)&(nuevo_archivo.filename), NULL);

                nuevo_archivo.filename_len = strlen((char *) nuevo_archivo.filename);

                memcpy(&(nuevo_archivo.dos_filename), directory_entry, 8);
                memcpy(&(nuevo_archivo.dos_fileext), directory_entry+8, 3);

                //reemplazamos los espacios por NULL, asi puedo hacer la comparacion con el path
                int i;
                for(i=0;i<8;i++)
                    if (nuevo_archivo.dos_filename[i] == 0x20) nuevo_archivo.dos_filename[i] = 0x00;
                for(i=0;i<3;i++)
                    if (nuevo_archivo.dos_fileext[i] == 0x20) nuevo_archivo.dos_fileext[i] = 0x00;

                memcpy(&(nuevo_archivo.file_type), &(((file_entry_t *)directory_entry)->file_attr), 1);
                memcpy(&(nuevo_archivo.file_size), &(((file_entry_t *)directory_entry)->file_size), 4);

                uint16_t cluster_hi=0, cluster_lo=0;
                memcpy(&cluster_hi, &(((file_entry_t *)directory_entry)->first_cluster_hi), 2);
                memcpy(&cluster_lo, &(((file_entry_t *)directory_entry)->first_cluster_low), 2);
                nuevo_archivo.first_cluster = cluster_hi * 0x100 + cluster_lo;
                nuevo_archivo.entry_index = current_entry;

                memcpy(ret_list+cantidad_entradas-1,&nuevo_archivo,sizeof(file_attrs));

                memset(utf16_filename, '\0', sizeof(utf16_filename));
                utf16_filename_len = 0;
                memset(&nuevo_archivo, '\0', sizeof(file_attrs));

                break;
        }

        current_entry++;
        fat32_get_entry(current_entry, first_cluster, (file_entry_t *)directory_entry, fs_tmp, socket);
    }

    return cantidad_entradas;
}

/**
 * Obtiene la informacion del archivo (o directorio) a partir del path. y devuelve el primer cluster
 * del directorio contenedor.
 *
 * @path path absoluto del archivo o directorio.
 * @ret_attrs buffer donde va a ir la estructura con la info del archivo o directorio.
 * @fs_tmp estructura privada del file system.
 * @return >0 primer cluster del directorio contenedor.
 *         -EINVAL argumento(s) invalidos.
 *         -ENOTDIR uno de los 'directorios' en el path no es un directorio.
 *         -ENOENT archivo o directorio inexistente.
 */
int32_t fat32_get_file_from_path(const uint8_t *path, file_attrs *ret_attrs, fs_fat32_t *fs_tmp, nipc_socket socket)
{
    if(!ret_attrs)
        return -EINVAL;

    int32_t ret_val=0;
    uint8_t **token_array = string_split4( path, (int8_t *) "/" );
    int32_t file_list_len = 0;
    file_attrs *file_list = 0;
    int i=0, j=0;
    int32_t cluster_actual = 2;

    //Parece ser que con string_split4 no es taaaan necesario el path absoluto.
    //Voy a seguir haciendo pruebas....
//    if(token_array[0] != '\0') {
//        printf("el path '%s' no es absoluto!!", path);
//        ret_val = -EINVAL;
//        goto error_2;
//    }
    
    for ( i = 1 ; token_array[i] ; i++ ) {
        if(token_array[i][0] == '\0')
            continue;

        file_list_len = fat32_get_file_list(cluster_actual, NULL, fs_tmp, socket);
        file_list = calloc(sizeof(file_attrs), file_list_len);
        fat32_get_file_list(cluster_actual, file_list, fs_tmp, socket);

        j = fat32_get_entry_from_name(token_array[i], file_list, file_list_len );

        if( j < 0 ) {
            ret_val = j;
            goto error_1;
        }
 
        //ultima entrada el path
        if(!token_array[i+1]) {
            ret_val = cluster_actual;
            goto salida_exitosa;
        }

        //si no es un directorio
        if((file_list[j].file_type & ATTR_SUBDIRECTORY) != ATTR_SUBDIRECTORY) {
            ret_val = -ENOTDIR;
            goto error_1;
        }

        //obtenemos el primer cluster del dir.
        if(!(cluster_actual = file_list[j].first_cluster))
            cluster_actual = 2;

        free(file_list);
    }

salida_exitosa:
    memcpy(ret_attrs, &file_list[j], sizeof(file_attrs));

error_1:
    free(file_list);

error_2:
    for ( i = 0 ; token_array[i] ; i++ ) free(token_array[i]);
    free(token_array);

    return ret_val;
}

/**
 * Obtiene el indice en la lista de archivos, a partir del nombre.
 * Obs: No se que hacer con el nombre DOS.
 *
 * @name Nombre del archivo a buscar.
 * @file_list lista de archivos a hacer la búsqueda.
 * @filfile_list_len cantidad de archivos en la lista.
 * @return >=0 indice en la lista del archivo buscado.
 *         -ENOENT archivo o directorio inexistente.
 */
int32_t fat32_get_entry_from_name(uint8_t *name, file_attrs *file_list, int32_t file_list_len)
{
    int i=0;
    int8_t buffer[512];
    for (i=0;i<file_list_len;i++)
    {
        fat32_build_name(file_list+i, buffer);

        if(!strcmp((char *)buffer, (char *) name)) //Encontrado!!
            return i;

        memset(buffer,'\0',sizeof(buffer));
    }

    return -ENOENT;
}

/**
 * Obtiene el nombre de archivo a partir de sus atributos (file).
 * Ya sea, nombre largo o nombre DOS(8.3), y lo almacena en ret_name.
 *
 * @file atributos del archivo.
 * @ret_name buffer donde se almacena el nombre
 */
void fat32_build_name(file_attrs *file, int8_t *ret_name)
{
    if(file->filename[0]) {
        memcpy(ret_name, file->filename, file->filename_len);
    } else {
        strncpy((char *)ret_name, (char *)file->dos_filename, 8);
        int8_t *fin_cadena = (int8_t *)strchr((char *)ret_name, '\0');
        if (ret_name[0] != '.') *fin_cadena = '.';
        strncpy((char *)(fin_cadena + 1),(char *) file->dos_fileext, 3);
    }
}

/**
 * Obtiene la primer entrada libre en el directory table a partir de
 * *first_cluster* que posea una entrada consecutiva tambien libre
 *
 * @first_cluster primer cluster del directorio.
 * @fs_tmp estructura privada del file system.
 * @return >=0 primer entrada libre.
 *         < 0 error.
 */
int32_t fat32_first_dual_fentry(int32_t first_cluster, fs_fat32_t *fs_tmp, nipc_socket socket)
{
#define is_free(entry) (((file_entry_t*)(entry))->file_attr == ATTR_LONG_FILE_NAME && (entry)[0] & 0x80) || (entry)[0] == 0xE5 || (entry)[0] == 0x00

    int32_t ret_val = -1;
    int32_t entry_n = 0;
    int8_t  buffer[sizeof(file_entry_t)];

    int8_t ret = fat32_get_entry(entry_n, first_cluster,(file_entry_t *) buffer, fs_tmp, socket);

    while(ret >= 0)
    {
        if(&buffer[0] == 0x00)
            return entry_n;

        if(is_free(buffer)) {
            if(ret_val == -1) {
                ret_val = entry_n;
            } else {
                return ret_val;
            }
        } else if (ret_val != -1) ret_val = -1;

        entry_n++;
        ret = fat32_get_entry(entry_n, first_cluster,(file_entry_t *) buffer, fs_tmp, socket);
    }

    return ret;
}

/**
 * Obtiene el numero de bloque a partir de un numero de entrada de directorio
 *
 * @first_cluster: Primer cluster del directorio contenedor.
 * @entry_index:   Indice de la directory entry en el directorio contenedor.
 * @fs_tmp:        estructura privada del file system.
 * @return:        Numero de bloque donde se encuentra la entrada dada.
 */
int32_t fat32_get_block_from_dentry(int32_t first_cluster, int32_t entry_index, fs_fat32_t *fs_tmp)
{
    int32_t cluster_size = fs_tmp->boot_sector.sectors_per_cluster * fs_tmp->boot_sector.bytes_per_sector;
    int32_t cluster_offset = entry_index / (cluster_size / 32);
    int32_t entry_offset   = entry_index % (cluster_size / 32);

    int32_t target_cluster = fat32_get_link_n_in_chain(first_cluster, cluster_offset, fs_tmp);
    if(target_cluster<0) return target_cluster;

    int32_t target_block = ( fs_tmp->system_area_size +
                     ((target_cluster - 2) * fs_tmp->boot_sector.sectors_per_cluster) +
                     (entry_offset / (SECT_SIZE / 32) )) / SECTORS_PER_BLOCK;
    
    return target_block;

}

/**
 * Imprime el hexdump de datos a un stream.
 *
 * @fp:       Descriptor de archivo donde imprimr la información.
 * @buff:     Ubicacion de la informacion a imprimir.
 * @cantidad: Cantidad de bytes a imprimir.
 */
void hex_log(FILE* fp, unsigned char * buff, int cantidad)
{
    int i=0, j=0;

    for (i=0;i < (cantidad / 16);i++) // filas
    {
        fprintf(fp, "%.6x  ", i*16);

        for (j=0;j < 16;j++) // columnas
        {
            if(i*16 + j > cantidad) fprintf(fp, "  ");
            else fprintf(fp, "%.2x", buff[i*16+j]);

            if ((j+1)%8) fprintf(fp, " ");
            else if ((j+1)%16) fprintf(fp, "  ");
        }

        fprintf (fp, "  |");

        for (j=0;j < 16;j++) // columnas
        {
            if(i*16 + j > cantidad) fprintf(fp, " ");
            else if (isprint(buff[i*16+j])) fprintf(fp, "%c", buff[i*16+j]);
            else fprintf(fp, ".");
        }

        fprintf(fp, "|\n");
    }
}

/**
 * Obtiene un socket libre del pool de conexiones, o bloques hasta obtener uno.
 *
 * @fs_tmp: Estructura privada del file system.
 * @return: Socket libre del pool.
 */
nipc_socket fat32_get_socket(fs_fat32_t *fs_tmp)
{
    nipc_socket socket = -1;
    sem_wait(&fs_tmp->mux_sockets);
    sem_wait(&fs_tmp->sem_recursos);
        int i = 0;
        for(i=0;i<sizeof(fs_tmp->sockets);i++) {
            if(fs_tmp->sockets[i].estado == SOCKET_DISP) {
                socket = fs_tmp->sockets[i].socket;
                fs_tmp->sockets[i].estado = SOCKET_OCUP;
                break;
            }
        }

        if(socket == -1) printf("Super error recontra fatal alto loco.");
    sem_post(&fs_tmp->mux_sockets);

    return socket;
}

/**
 * Libera un socket.
 *
 * @socket: Socket a ser liberado
 * @fs_tmp: Estructura privada del file system.
 */
void fat32_free_socket(nipc_socket socket, fs_fat32_t *fs_tmp)
{
    int i=0;
    for(i=0;i<sizeof(fs_tmp->sockets);i++) {
        if(fs_tmp->sockets[i].socket == socket) {
            fs_tmp->sockets[i].estado = SOCKET_DISP;
            break;
        }
    }

    sem_post(&fs_tmp->sem_recursos);
}

