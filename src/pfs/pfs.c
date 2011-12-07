#include "defs.h"
#include "pfs_utils.h"
#include "pfs_consola.h"
#include "utils.h"
#include "pfs_files.h"
#include "pfs_files.h"
#include "pfs.h"
#include "direccionamiento.h"

#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/statvfs.h>

fs_fat32_t *sig_fs=0;
void fat32_sigusr_handler(int sig)
{
    log_info(sig_fs->log, "SIGUSR1", "Detectado SIGUSR1. Dump de caches.");

    int arch, num_cache;
    time_t timestamp;
    FILE *fp;

    if(!(fp = fopen("cache_dump.txt", "a"))) {
        log_info(sig_fs->log, "SIGUSR1", "No se pudo abrir (o crear) el archivo para dump de cache, Error: %d (%s)", errno, strerror(errno));
        return;
    }

    for( arch = 0 ; arch < MAX_OPEN_FILES ; arch++ ) {
        if (sig_fs->open_files[arch].busy) {
            cache_t *cache=sig_fs->open_files[arch].cache;
            char buff[1024];
            timestamp = time(NULL);
            strftime(buff, 1024, "%Y.%m.%d %H:%M:%S", localtime(&timestamp));

            fprintf(fp, "---------------------------------------------\n");
            fprintf(fp, "Archivo: %s\n", sig_fs->open_files[arch].path);
            fprintf(fp, "Timestamp: %s\n", buff);
            fprintf(fp, "Tamaño de Bloque de Cache: %d\n", BLOCK_SIZE);
            fprintf(fp, "Cantidad de Bloques de Cache: %d\n", sig_fs->cache_size);
            fprintf(fp, "\n");

            sem_wait(&sig_fs->open_files[arch].sem_cache);

            if(cache != NULL)
                for( num_cache = 0 ; num_cache < sig_fs->cache_size ; num_cache++ ) {
                    fprintf(fp, "Contenido de Bloque de Cache %d: ", num_cache);

                    if (cache[num_cache].number != -1) {
                        fprintf(fp, "Bloque: %d - Modificado: %s\n", cache[num_cache].number,
                                                                    (cache[num_cache].modificado)?"Si":"No");
                        hex_log(fp, (unsigned char *)sig_fs->open_files[arch].cache[num_cache].contenido, BLOCK_SIZE);
                    } else {
                        fprintf(fp, "Sin Usar\n");
                    }

                    fprintf(fp, "\n");
                }

            sem_post(&sig_fs->open_files[arch].sem_cache);

            fprintf(fp, "\n");
        }
    }

    fclose(fp);
}

int fat32_create (const char *path, mode_t mode, struct fuse_file_info *fi)
{
    printf("Create: path:%s\n", path);
    return 0;
}

int fat32_ftruncate (const char *path, off_t offset, struct fuse_file_info *fi)
{
    struct fuse_context* context = fuse_get_context();
    fs_fat32_t *fs_tmp = (fs_fat32_t *) context->private_data;
    int32_t cluster_size = fs_tmp->boot_sector.sectors_per_cluster * fs_tmp->boot_sector.bytes_per_sector;

    char buff[10];
    sprintf(buff,"%d",context->pid);
    log_info(fs_tmp->log, buff, "Truncate: path: %s offset: %d", path, offset);

    if (strcmp((char *)fs_tmp->open_files[fi->fh].path, path) != 0)
        return -EINVAL;

    //Si no hay nada que modificar salimos exitosamente
    if(offset == fs_tmp->open_files[fi->fh].file_size)
        return 0;

    nipc_socket socket = fat32_get_socket(fs_tmp);

    //Hay que modificar la FAT?
    //cluster_abm == 0 no modificar la FAT
    //     "      >  0 cantidad de clusters a agregar a la cadena
    //     "      <  0 cantidad de clusters a remover de la cadena
    div_t aux_new = div(offset, cluster_size);
    div_t aux_old = div(fs_tmp->open_files[fi->fh].file_size, cluster_size);
    int32_t cluster_abm = ((aux_new.rem)?aux_new.quot+1:aux_new.quot) -
                          ((aux_old.rem)?aux_old.quot+1:aux_old.quot);

    //Obtenemos la entrada de directorio para modificarla
    file_entry_t entry;
    int32_t entry_index = fs_tmp->open_files[fi->fh].entry_index;

    int8_t ret = fat32_get_entry(entry_index, fs_tmp->open_files[fi->fh].container_cluster, &entry, fs_tmp, socket);
    if (ret<0) {
        fat32_free_socket(socket, fs_tmp);
        return ret;
    }

    //Si no tenia clusteres asociados y el tamaño pedido es != 0
    //agregamos el primer cluster libre que encontremos,
    //y actualizamos la entrada de directorio
    if(fs_tmp->open_files[fi->fh].first_cluster==0 && offset!=0)
    {
        sem_wait(&fs_tmp->mux_fat);
            int32_t free_cluster = fat32_first_free_cluster(fs_tmp);
            fs_tmp->fat[free_cluster] = fs_tmp->eoc_marker;
        sem_post(&fs_tmp->mux_fat);
        fs_tmp->open_files[fi->fh].first_cluster=free_cluster;
        entry.first_cluster_hi = (free_cluster>>16 & 0xFFFF);
        entry.first_cluster_low = free_cluster & 0xFFFF;

        int32_t bloque = (fs_tmp->boot_sector.reserved_sectors + (free_cluster / (fs_tmp->boot_sector.bytes_per_sector / sizeof(int32_t)))) / SECTORS_PER_BLOCK;

        fat32_writeblock(bloque, 1, (fs_tmp->fat) + (bloque - (fs_tmp->boot_sector.reserved_sectors/SECTORS_PER_BLOCK)) * (BLOCK_SIZE/sizeof(int32_t)), NO_CACHE, fs_tmp, socket);

        //Inicializamos los datos del nuevo cluster a 0
        char buffer[fs_tmp->boot_sector.sectors_per_cluster * fs_tmp->boot_sector.bytes_per_sector];
        memset(buffer, '\0', fs_tmp->boot_sector.sectors_per_cluster * fs_tmp->boot_sector.bytes_per_sector);
        fat32_writecluster(free_cluster, buffer, NO_CACHE, fs_tmp, socket);

        cluster_abm--;
    }

    fs_tmp->open_files[fi->fh].file_size = entry.file_size = offset;

    //Modificamos la FAT
    if(cluster_abm > 0)
        while(cluster_abm)
        {
            fat32_add_cluster(fs_tmp->open_files[fi->fh].first_cluster, fs_tmp, socket);
            cluster_abm--;
        }
    else if(cluster_abm < 0)
        while(cluster_abm)
        {
            fat32_remove_cluster(fs_tmp->open_files[fi->fh].first_cluster, fs_tmp, socket);
            cluster_abm++;
        }

    //si el tamaño pedido es 0 y tiene algun cluster asociado,
    //lo liberamos en la FAT, y editamos la entry
    if(fs_tmp->open_files[fi->fh].first_cluster!=0 && offset==0)
    {
        sem_wait(&fs_tmp->mux_fat);
            fs_tmp->fat[fs_tmp->open_files[fi->fh].first_cluster] = 0;
        sem_post(&fs_tmp->mux_fat);
        int32_t bloque = ( fs_tmp->boot_sector.reserved_sectors + ( fs_tmp->open_files[fi->fh].first_cluster / ( fs_tmp->boot_sector.bytes_per_sector / sizeof(int32_t)))) / SECTORS_PER_BLOCK;

        fs_tmp->open_files[fi->fh].first_cluster=0;
        entry.first_cluster_hi = 0;
        entry.first_cluster_low = 0;

        fat32_writeblock(bloque, 1, (fs_tmp->fat) + (bloque - (fs_tmp->boot_sector.reserved_sectors/SECTORS_PER_BLOCK)) * (BLOCK_SIZE/sizeof(int32_t)), NO_CACHE, fs_tmp, socket);
    }

    //Modificamos la entrada en el directorio.
    int32_t target_block = fat32_get_block_from_dentry(fs_tmp->open_files[fi->fh].container_cluster, entry_index, fs_tmp);

    int8_t buffer[BLOCK_SIZE];
    fat32_getblock(target_block, 1, buffer, NO_CACHE, fs_tmp, socket);

    //modificamos entry_offset para que sea con respecto al comienzo del bloque
    int32_t entry_offset= 0;
    entry_offset = entry_index % (cluster_size / 32);
    entry_offset = entry_offset % (BLOCK_SIZE / 32);

    memcpy(buffer + (entry_offset * 32), &entry, sizeof(entry));

    //Pedir la escritura del bloque
    fat32_writeblock(target_block, 1, &buffer, NO_CACHE, fs_tmp, socket);

    fat32_free_socket(socket, fs_tmp);
    return 0;
}

int fat32_write (const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    struct fuse_context* context = fuse_get_context();
    fs_fat32_t *fs_tmp = (fs_fat32_t *) context->private_data;
    int32_t cluster_size = fs_tmp->boot_sector.sectors_per_cluster * fs_tmp->boot_sector.bytes_per_sector;

    char buff[10];
    sprintf(buff,"%d",context->pid);
    log_info(fs_tmp->log, buff, "Write: path: %s size: %d offset: %ld", path, size, offset);

    if (strcmp((char *)fs_tmp->open_files[fi->fh].path, path) != 0)
        return -EINVAL;

    nipc_socket socket = fat32_get_socket(fs_tmp);

    //si el archivo queda chico lo ampliamos
    if(fs_tmp->open_files[fi->fh].file_size < offset+size)
        fat32_ftruncate (path, offset+size, fi);

    int32_t cluster_actual = fs_tmp->open_files[fi->fh].first_cluster;

    cluster_actual = fat32_get_link_n_in_chain( cluster_actual, offset / cluster_size, fs_tmp);
    if (cluster_actual == fs_tmp->eoc_marker) {
        fat32_free_socket(socket, fs_tmp);
        return 0;
    }

    int8_t *buffer = calloc(cluster_size, 1);
    size_t data_to_write = size;
    offset = offset % cluster_size;

    fat32_getcluster(cluster_actual, buffer, fi->fh, fs_tmp, socket);
    int32_t data_cluster = MIN(cluster_size - offset, data_to_write);

    memcpy(buffer + offset, buf, data_cluster);

    fat32_writecluster(cluster_actual, buffer, fi->fh, fs_tmp, socket);

    data_to_write -= data_cluster;

    while(data_to_write) {
        data_cluster = MAX(data_to_write, cluster_size);

        cluster_actual = fat32_get_link_n_in_chain( cluster_actual, 1, fs_tmp);

        if(data_cluster < cluster_size)
            fat32_getcluster(cluster_actual, buffer, fi->fh, fs_tmp, socket);

        memcpy(buffer, buf+size-data_to_write, data_cluster);

        fat32_writecluster(cluster_actual, buffer, fi->fh, fs_tmp, socket);

        data_to_write -= data_cluster;
    }

    fat32_free_socket(socket, fs_tmp);
    return size; //TODO error handling
}

int fat32_flush (const char *path, struct fuse_file_info *fi)
{
    struct fuse_context* context = fuse_get_context();
    fs_fat32_t *fs_tmp = (fs_fat32_t *) context->private_data;

    char buff[10];
    sprintf(buff,"%d",context->pid);
    log_info(fs_tmp->log, buff, "Flush: path: %s", path);

    if (strcmp((char *)fs_tmp->open_files[fi->fh].path, path) != 0)
        return -EINVAL;

    int cache_idx=0;
    cache_t *cache = fs_tmp->open_files[fi->fh].cache;
    nipc_socket socket = fat32_get_socket(fs_tmp);

    sem_wait(&fs_tmp->open_files[fi->fh].sem_cache);
    for ( cache_idx = 0 ; cache_idx < fs_tmp->cache_size ; cache_idx++ )
        if(cache->modificado)
        {
            fat32_writeblock(cache[cache_idx].number, 1, cache[cache_idx].contenido, NO_CACHE, fs_tmp, socket);
            cache[cache_idx].modificado = 0;
        }
    sem_post(&fs_tmp->open_files[fi->fh].sem_cache);

    fat32_free_socket(socket, fs_tmp);

    return 0;
}

int fat32_fsync (const char *path, int datasync, struct fuse_file_info *fi)
{
    struct fuse_context* context = fuse_get_context();
    fs_fat32_t *fs_tmp = (fs_fat32_t *) context->private_data;

    char buff[10];
    sprintf(buff,"%d",context->pid);
    log_info(fs_tmp->log, buff, "Fsync: path: %s", path);

    if (strcmp((char *)fs_tmp->open_files[fi->fh].path, path) != 0)
        return -EINVAL;

    int cache_idx=0;
    cache_t *cache = fs_tmp->open_files[fi->fh].cache;
    nipc_socket socket = fat32_get_socket(fs_tmp);

    sem_wait(&fs_tmp->open_files[fi->fh].sem_cache);
    for ( cache_idx = 0 ; cache_idx < fs_tmp->cache_size ; cache_idx++ )
        if(cache->modificado)
        {
            fat32_writeblock(cache[cache_idx].number, 1, cache[cache_idx].contenido, NO_CACHE, fs_tmp, socket);
            cache[cache_idx].modificado = 0;
        }
    sem_post(&fs_tmp->open_files[fi->fh].sem_cache);

    fat32_free_socket(socket, fs_tmp);

    return 0;
}

int fat32_release (const char *path, struct fuse_file_info *fi)
{
    struct fuse_context* context = fuse_get_context();
    fs_fat32_t *fs_tmp = (fs_fat32_t *) context->private_data;

    char buff[10];
    sprintf(buff,"%d",context->pid);
    log_info(fs_tmp->log, buff, "Release: path: %s", path);

    if (strcmp((char *)fs_tmp->open_files[fi->fh].path, path) != 0)
        return -EINVAL;

    sem_destroy(&(fs_tmp->open_files[fi->fh].sem_cache));

    memset(&(fs_tmp->open_files[fi->fh]), '\0', sizeof(file_descriptor));

    return 0;
}

int fat32_unlink (const char *path)
{
    printf("Unlink: path: %s\n", path);
    return 0;
}

int fat32_rmdir (const char *path)
{
    printf("RmDir: path: %s\n", path);
    return 0;
}

int fat32_mkdir (const char *path, mode_t mode)
{
    printf("MkDir: path: %s\n", path);
    return 0;
}

//Comprobaciones strlen(to) <= 13
int fat32_rename (const char *from, const char *to)
{
    int8_t ret;
    //Se pueden renombrar archivos abiertos???
    struct fuse_context* context = fuse_get_context();
    fs_fat32_t *fs_tmp = (fs_fat32_t *) context->private_data;

    char buff[10];
    sprintf(buff,"%d",context->pid);
    log_info(fs_tmp->log, buff, "Rename: from: %s to: %s", from, to);

    nipc_socket socket = fat32_get_socket(fs_tmp);

    file_attrs reg_file_attrs;
    int32_t cluster_actual = fat32_get_file_from_path((uint8_t *)from, &reg_file_attrs, fs_tmp, socket);

    file_entry_t lfn_entry_old, file_entry_old;
    ret = fat32_get_entry(reg_file_attrs.entry_index - 1, cluster_actual, &lfn_entry_old, fs_tmp, socket);
    if(ret<0) {
        fat32_free_socket(socket, fs_tmp);
        return ret;
    }
    ret = fat32_get_entry(reg_file_attrs.entry_index, cluster_actual, &file_entry_old, fs_tmp, socket);
    if(ret<0) {
        fat32_free_socket(socket, fs_tmp);
        return ret;
    }

    file_entry_t file_entry_new, lfn_entry_new;
    memcpy(&lfn_entry_new,  &lfn_entry_old,  sizeof(lfn_entry_old));
    memcpy(&file_entry_new, &file_entry_old, sizeof(file_entry_old));

    //separamos el nombre de la ruta a el
    int8_t *path2 = calloc(strlen(to) + 1, 1);
    strcpy((char *)path2, to);
    int8_t *nombre_arch = (int8_t *)strrchr((char *)path2, '/');
    nombre_arch[0] = '\0';
    nombre_arch++;
    if(strlen((char *)nombre_arch) > 13)
    {
        log_error(fs_tmp->log, "", "El nombre '%s' excede el limite de 13 caracteres "
                                   "impuesto por el enunciado.", nombre_arch);
        free(path2);
        fat32_free_socket(socket, fs_tmp);
        return -EINVAL;
    }

/* Borramos archivo viejo */
    //obtener bloque(s) viejo(s)
    int32_t bloque_lfn = fat32_get_block_from_dentry(cluster_actual, reg_file_attrs.entry_index - 1, fs_tmp);
    int32_t bloque_file= fat32_get_block_from_dentry(cluster_actual, reg_file_attrs.entry_index, fs_tmp);

    int8_t *bloques = calloc(BLOCK_SIZE, 2);

    fat32_getblock(bloque_lfn, 1, bloques, NO_CACHE, fs_tmp, socket);
    if (bloque_file != bloque_lfn)
        fat32_getblock(bloque_file, 1, bloques + BLOCK_SIZE, NO_CACHE, fs_tmp, socket);

    //modificar bloque(s) viejo(s)
    int32_t entry_offset   = (reg_file_attrs.entry_index - 1) % (fs_tmp->boot_sector.bytes_per_sector *
                                    fs_tmp->boot_sector.sectors_per_cluster / sizeof(file_entry_t));
    entry_offset = entry_offset % (SECTORS_PER_BLOCK * fs_tmp->boot_sector.bytes_per_sector / sizeof(file_entry_t));

    ((lfn_entry_t*)&lfn_entry_old)->seq_number |= 0x80;
    file_entry_old.dos_file_name[0] = 0xE5;

    memcpy(bloques + (entry_offset * sizeof(file_entry_t)), &lfn_entry_old, sizeof(lfn_entry_old));
    if(bloque_file!=bloque_lfn)
        memcpy(bloques + BLOCK_SIZE + (entry_offset * sizeof(file_entry_t)) + sizeof(lfn_entry_old), &file_entry_old, sizeof(lfn_entry_old));
    else
        memcpy(bloques + (entry_offset * sizeof(file_entry_t)) + sizeof(lfn_entry_old), &file_entry_old, sizeof(file_entry_t));

    //pedir escritura bloque viejo
    fat32_writeblock(bloque_lfn, 1, bloques, NO_CACHE, fs_tmp, socket);
    if (bloque_file != bloque_lfn)
        fat32_writeblock(bloque_file, 1, bloques + BLOCK_SIZE, NO_CACHE, fs_tmp, socket);
/* */

/* Creamos archivo nuevo */
    //pedir bloque nuevo
    file_attrs dir_attrs;
    if(!strcmp((char *)path2, ""))
        dir_attrs.first_cluster = 2;
    else
        fat32_get_file_from_path((uint8_t *)path2, &dir_attrs, fs_tmp, socket);

    int32_t entry_index = fat32_first_dual_fentry(dir_attrs.first_cluster, fs_tmp, socket);

    bloque_lfn = fat32_get_block_from_dentry(dir_attrs.first_cluster, entry_index, fs_tmp);
    bloque_file= fat32_get_block_from_dentry(dir_attrs.first_cluster, entry_index + 1, fs_tmp);

    fat32_getblock(bloque_lfn, 1, bloques, NO_CACHE, fs_tmp, socket);
    if (bloque_file != bloque_lfn)
        fat32_getblock(bloque_file, 1, bloques + BLOCK_SIZE, NO_CACHE, fs_tmp, socket);

    //modificar bloque nuevo
    ((lfn_entry_t *)&lfn_entry_new)->seq_number = 0x41;

    uint16_t utf16_name[14];
    memset(utf16_name, 0xff, sizeof(utf16_name));
    unicode_utf8_to_utf16_inbuffer((unsigned char*)nombre_arch, MAX(strlen((char*)nombre_arch) + 1, 13),
                                   utf16_name, NULL);
    memcpy(((lfn_entry_t *)&lfn_entry_new)->lfn_name1, utf16_name,      5 * sizeof(int16_t));
    memcpy(((lfn_entry_t *)&lfn_entry_new)->lfn_name2, utf16_name + 5,  6 * sizeof(int16_t));
    memcpy(((lfn_entry_t *)&lfn_entry_new)->lfn_name3, utf16_name + 11, 2 * sizeof(int16_t));

    int8_t dos_name[11];
    memset(dos_name, 0x20, 11);
    memcpy(dos_name, nombre_arch, MIN(8, strlen((char*)nombre_arch)));
    memcpy(dos_name+8, nombre_arch + strlen((char*)nombre_arch) - 3, MIN(3, strlen((char*)nombre_arch)));

    int8_t i=0;
    for(i=0;i<11;i++) dos_name[i] = toupper(dos_name[i]);
    if(dos_name[0] == 0xe5) dos_name[0] = 0x05;

    memcpy(file_entry_new.dos_file_name, dos_name, 11);

    memcpy(bloques + (entry_index * sizeof(file_entry_t)), &lfn_entry_new, sizeof(file_entry_t));
    if(bloque_file!=bloque_lfn)
        memcpy(bloques + BLOCK_SIZE + ((entry_index + 1) * sizeof(file_entry_t)), &file_entry_new, sizeof(file_entry_t));
    else
        memcpy(bloques + ((entry_index + 1) * sizeof(file_entry_t)), &file_entry_new, sizeof(file_entry_t));

    //pedir escritura bloque nuevo
    fat32_writeblock(bloque_lfn, 1, bloques, NO_CACHE, fs_tmp, socket);
    if (bloque_file != bloque_lfn)
        fat32_writeblock(bloque_file, 1, bloques + BLOCK_SIZE, NO_CACHE, fs_tmp, socket);
/* */

    free(bloques);
    fat32_free_socket(socket, fs_tmp);
    return 0;
}

static int fat32_getattr(const char *path, struct stat *stbuf)
{
    struct fuse_context* context = fuse_get_context();
    fs_fat32_t *fs_tmp = (fs_fat32_t *) context->private_data;

    char buff[10];
    sprintf(buff,"%d",context->pid);
    log_info(fs_tmp->log, buff, "Getattr: path: %s", path);
    memset(stbuf, 0, sizeof(struct stat));

    if(memcmp(path, "/", 2) == 0) {
        stbuf->st_mode = S_IFDIR | S_IRWXU | S_IRWXG | S_IRWXO;
        stbuf->st_nlink = 2;
        stbuf->st_uid = context->uid; /* user ID of owner */
        stbuf->st_gid = context->gid; /* group ID of owner */
        return 0;
    }

    nipc_socket socket = fat32_get_socket(fs_tmp);

    file_attrs ret_attrs;
    int32_t ret = fat32_get_file_from_path((const uint8_t *) path, &ret_attrs, fs_tmp, socket);
    if(ret<0) {
        fat32_free_socket(socket, fs_tmp);
        return ret;
    }

    stbuf->st_mode = S_IRWXU | S_IRWXG | S_IRWXO; /* protection */
    stbuf->st_nlink = 1; /* number of hard links */
    stbuf->st_uid = context->uid; /* user ID of owner */
    stbuf->st_gid = context->gid; /* group ID of owner */
    stbuf->st_size = ret_attrs.file_size; /* total size, in bytes */
    stbuf->st_blksize = 2048 /*BLOCK_SIZE*/; /* blocksize for filesystem I/O */
    stbuf->st_blocks = (ret_attrs.file_size / 2048 /*BLOCK_SIZE*/) + 1; /* number of blocks allocated */

    if (ret_attrs.file_type == ATTR_SUBDIRECTORY)
        stbuf->st_mode |= S_IFDIR;
    else stbuf->st_mode |= S_IFREG;

    fat32_free_socket(socket, fs_tmp);
    return 0;
}

static int fat32_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                         off_t offset, struct fuse_file_info *fi)
{
    (void) offset;
    (void) fi;
    int32_t primer_cluster = 0;

    struct fuse_context* context = fuse_get_context();
    fs_fat32_t *fs_tmp = (fs_fat32_t *) context->private_data;

    char buff[10];
    sprintf(buff,"%d",context->pid);
    log_info(fs_tmp->log, buff, "Readdir: path: %s", path);

    if(path[0] != '/')
        return -EINVAL;

    nipc_socket socket = fat32_get_socket(fs_tmp);

    if(memcmp(path, "/", 2) == 0) {
        filler(buf, ".", NULL, 0);
        filler(buf, "..", NULL, 0);
        primer_cluster = 2;
    } else {
        file_attrs ret_attrs;
        int32_t ret = fat32_get_file_from_path((const uint8_t *) path, &ret_attrs, fs_tmp, socket);
        if(ret<0) {
            fat32_free_socket(socket, fs_tmp);
            return ret;
        }

        primer_cluster = ret_attrs.first_cluster;
    }

    int32_t cantidad_arch = fat32_get_file_list(primer_cluster, NULL, fs_tmp, socket);
    file_attrs *lista = calloc(sizeof(file_attrs), cantidad_arch);
    fat32_get_file_list(primer_cluster, lista, fs_tmp, socket);

    int8_t filename[512];

    int32_t i;
    for( i = 0 ; i < cantidad_arch ; i++ ) {
        memset(filename, '\0', sizeof(filename));
        fat32_build_name(lista+i, filename);

        struct stat var_stat = {
            .st_mode = S_IRWXU | S_IRWXG | S_IRWXO, /* protection */
            .st_nlink = 1, /* number of hard links */
            .st_uid = context->uid, /* user ID of owner */
            .st_gid = context->gid, /* group ID of owner */
            .st_size = lista[i].file_size, /* total size, in bytes */
            .st_blksize = BLOCK_SIZE, /* blocksize for filesystem I/O */
            .st_blocks = (lista[i].file_size / BLOCK_SIZE) + 1 /* number of blocks allocated */
        };

        if (lista[i].file_type == ATTR_SUBDIRECTORY)
            var_stat.st_mode |= S_IFDIR;
        else var_stat.st_mode |= S_IFREG;

        filler(buf,(char *) filename, &var_stat, 0 );

    }

    free(lista);
    fat32_free_socket(socket, fs_tmp);
    return 0;
}

static int fat32_open(const char *path, struct fuse_file_info *fi)
{
    struct fuse_context* context = fuse_get_context();
    fs_fat32_t *fs_tmp = (fs_fat32_t *) context->private_data;

    char buff[10];
    sprintf(buff,"%d",context->pid);
    log_info(fs_tmp->log, buff, "Open: path: %s", path);

    file_attrs ret_attrs;

    nipc_socket socket = fat32_get_socket(fs_tmp);

    int32_t container_cluster = fat32_get_file_from_path((const uint8_t *) path, &ret_attrs, fs_tmp, socket);
    if(container_cluster<0) {
        fat32_free_socket(socket, fs_tmp);
        return container_cluster;
    }

    int32_t ret = fat32_get_free_file_entry(fs_tmp->open_files);
    if(ret<0) {
        fat32_free_socket(socket, fs_tmp);
        return ret;
    }

    memcpy(fs_tmp->open_files[ret].path, path, strlen(path));
    fat32_build_name(&ret_attrs, fs_tmp->open_files[ret].filename);
    fs_tmp->open_files[ret].file_size = ret_attrs.file_size;
    fs_tmp->open_files[ret].first_cluster = ret_attrs.first_cluster;
    fs_tmp->open_files[ret].busy = TRUE;
    fs_tmp->open_files[ret].container_cluster = container_cluster;
    fs_tmp->open_files[ret].entry_index = ret_attrs.entry_index;

    //Ahora vamos por la cache.
    fs_tmp->open_files[ret].cache = calloc(fs_tmp->cache_size, sizeof(cache_t));
    sem_init(&(fs_tmp->open_files[ret].sem_cache), 0, 1);

    int i=0;
    for(i=0;i < (fs_tmp->cache_size);i++)
    {
        fs_tmp->open_files[ret].cache[i].number = -1;
        fs_tmp->open_files[ret].cache[i].modificado = 0;
        fs_tmp->open_files[ret].cache[i].stamp = -1;
    }

    //seteamos el file handler del file_info como el indice en la tabla de archivos abiertos
    fi->fh = ret;

    fat32_free_socket(socket, fs_tmp);
    return 0;
}

static int fat32_read(const char *path, char *buf, size_t size, off_t offset,
                      struct fuse_file_info *fi)
{
    struct fuse_context* context = fuse_get_context();
    fs_fat32_t *fs_tmp = (fs_fat32_t *) context->private_data;
    int32_t cluster_size = fs_tmp->boot_sector.sectors_per_cluster * fs_tmp->boot_sector.bytes_per_sector;

    char buff[10];
    sprintf(buff,"%d",context->pid);
    log_info(fs_tmp->log, buff, "Read: path: %s size: %d offset: %ld", path, size, offset);

    if (strcmp((char *)fs_tmp->open_files[fi->fh].path, path) != 0)
        return -EINVAL;

    //Tamaño de archivo o.O
    if(fs_tmp->open_files[fi->fh].file_size<(offset+size))
        size = fs_tmp->open_files[fi->fh].file_size-offset;

    int32_t cluster_actual = fs_tmp->open_files[fi->fh].first_cluster;

    cluster_actual = fat32_get_link_n_in_chain( cluster_actual, offset / cluster_size, fs_tmp);
    if (cluster_actual == fs_tmp->eoc_marker) return 0;

    offset =  offset % cluster_size;

    nipc_socket socket = fat32_get_socket(fs_tmp);

    int8_t *cluster_buffer = calloc(fs_tmp->boot_sector.bytes_per_sector, fs_tmp->boot_sector.sectors_per_cluster);
    fat32_getcluster(cluster_actual, cluster_buffer, fi->fh, fs_tmp, socket);

    int32_t size_to_read = cluster_size - offset;
    int32_t size_aldy_read = 0;
    if (size_to_read>size) size_to_read = size;
    size_aldy_read += size_to_read;
    size -= size_to_read;

    memcpy(buf, cluster_buffer + offset, size_to_read);

    while((size > 0) && (fs_tmp->fat[cluster_actual] != fs_tmp->eoc_marker))
    {
        cluster_actual = fat32_get_link_n_in_chain(cluster_actual, 1, fs_tmp);
        fat32_getcluster(cluster_actual, cluster_buffer, fi->fh, fs_tmp, socket);
        size_to_read = MIN(cluster_size, size);
        memcpy(buf + size_aldy_read, cluster_buffer, size_to_read);

        size_aldy_read += size_to_read;
        size -= size_to_read;
    }

    free(cluster_buffer);

    fat32_free_socket(socket, fs_tmp);
    return size_aldy_read;
}

static void *fat32_init(struct fuse_conn_info *conn)
{
    fs_fat32_t *fs_tmp = calloc(sizeof(fs_fat32_t), 1);
    fs_tmp->boot_sector.bytes_per_sector = SECT_SIZE;

    fat32_config_read(fs_tmp);

    fs_tmp->log = log_new( fs_tmp->log_path, "process_file_system", fs_tmp->log_mode );

/* Inicializamos los sockets */
    int aux = 0;
    for( aux = 0 ; aux < MAX_CONECTIONS ; aux++ )
    {
        if((fs_tmp->sockets[aux].socket = create_socket((char *)fs_tmp->server_host, fs_tmp->server_port))<0) {
            log_info(fs_tmp->log, "Init", "Error al crear el socket nr. %d : %s", aux, strerror(errno));
            exit(-EADDRNOTAVAIL);
        }

        if(nipc_connect_socket(fs_tmp->sockets[aux].socket, (char *)fs_tmp->server_host, fs_tmp->server_port)) {
            log_error(fs_tmp->log, "Init", "La conexion al RAID 1 o planificador de disco no esta lista, para el socket: %d: %s", aux, strerror(errno));
            exit(-EADDRNOTAVAIL);
        }

        log_info(fs_tmp->log, "Init", "Conectado. Socket: %d, %d", aux, fs_tmp->sockets[aux].socket);

        fat32_handshake(fs_tmp->sockets[aux].socket);
        log_info(fs_tmp->log, "Init", "Handshake aceptado.");

        fs_tmp->sockets[aux].estado = SOCKET_DISP;
    }
/* Semaforos de los sockets*/
    sem_init(&fs_tmp->sem_recursos, 0, MAX_CONECTIONS);
    sem_init(&fs_tmp->mux_sockets, 0, 1);
/**/

/* Inicializamos los descriptores */
    memset(fs_tmp->open_files, '\0', sizeof(fs_tmp->open_files));
/**/

/* Manejador del SIGUSR1 */
    sig_fs = fs_tmp;
    struct sigaction act;

    bzero(&act, sizeof(act));
    act.sa_handler = fat32_sigusr_handler;
    if (sigaction(SIGUSR1, &act, NULL)==-1)
        log_warning(fs_tmp->log, "Init", "Error %d al iniciar la señal SIGUSR1: %s\n", errno, strerror(errno));
/**/

    nipc_socket socket = fat32_get_socket(fs_tmp);

    int8_t block[BLOCK_SIZE];

    fat32_getblock(0, 1, block, NO_CACHE, fs_tmp, socket);

    memset(block+0x3E8, 0xff, 4);
    fat32_writeblock(0, 1, block, NO_CACHE, fs_tmp, socket);

    memcpy(fs_tmp->boot_sector.buffer, block, fs_tmp->boot_sector.bytes_per_sector);
    fs_tmp->system_area_size = fs_tmp->boot_sector.reserved_sectors + ( fs_tmp->boot_sector.fat_count * fs_tmp->boot_sector.sectors_per_fat );

    if (fs_tmp->boot_sector.bytes_per_sector != 512) {
        printf("Nos dijeron que se suponian 512 bytes por sector, no %d bytes >.<\n", fs_tmp->boot_sector.bytes_per_sector);
        exit(-EINVAL);
    }

    fs_tmp->fat = calloc(fs_tmp->boot_sector.bytes_per_sector, fs_tmp->boot_sector.sectors_per_fat);
    fat32_getblock(fs_tmp->boot_sector.reserved_sectors / SECTORS_PER_BLOCK,
                   fs_tmp->boot_sector.sectors_per_fat / SECTORS_PER_BLOCK, fs_tmp->fat, NO_CACHE, fs_tmp, socket);

//    hex_log((unsigned char *)fs_tmp->fat,SECT_SIZE);

    memcpy(&(fs_tmp->eoc_marker), fs_tmp->fat + 1, 4);

    sem_init(&fs_tmp->mux_fat,0,1);

    //creamos el thread de la consola
    pthread_create(&(fs_tmp->thread_consola), NULL, fat32_consola, fs_tmp);

    log_info(fs_tmp->log, "Init", "BPS:%d - SPC:%d - RS:%d - FC:%d - TS:%d - SPF:%d - SAS:%d clusters libres:%d EOC:%ld-",
                                       fs_tmp->boot_sector.bytes_per_sector,
                                       fs_tmp->boot_sector.sectors_per_cluster,
                                       fs_tmp->boot_sector.reserved_sectors,
                                       fs_tmp->boot_sector.fat_count,
                                       fs_tmp->boot_sector.total_sectors,
                                       fs_tmp->boot_sector.sectors_per_fat,
                                       fs_tmp->system_area_size,
                                       fat32_free_clusters(fs_tmp),
                                       fs_tmp->eoc_marker);

    fat32_free_socket(socket, fs_tmp);
    return fs_tmp;

}

int fat32_statfs(const char *path, struct statvfs *statvfs)
{
    struct fuse_context* context = fuse_get_context();
    fs_fat32_t *fs_tmp = (fs_fat32_t *) context->private_data;

    char buff[10];
    sprintf(buff,"%d",context->pid);
    log_info(fs_tmp->log, buff, "Statvfs: path: %s", path);

    statvfs->f_bsize = BLOCK_SIZE;
    statvfs->f_blocks = fs_tmp->boot_sector.total_sectors / SECTORS_PER_BLOCK;
    statvfs->f_bavail = statvfs->f_bfree = (fat32_free_clusters(fs_tmp) * fs_tmp->boot_sector.sectors_per_cluster) / SECTORS_PER_BLOCK;

    return 0;

}

void fat32_destroy(void * foo) //foo es private_data que devuelve el init
{
    /*
    nipc_close(((fs_fat32_t *)foo)->socket);*/

    log_delete(((fs_fat32_t *)foo)->log);

    sem_wait(&((fs_fat32_t *)foo)->mux_fat);
    free(((fs_fat32_t *)foo)->fat);
    sem_destroy(&((fs_fat32_t *)foo)->mux_fat);
    free(foo);
}

static struct fuse_operations fat32_oper = {
    .getattr   = fat32_getattr,
    .readdir   = fat32_readdir,
    .open      = fat32_open,
    .read      = fat32_read,
    .init      = fat32_init,
    .destroy   = fat32_destroy,
    .create    = fat32_create,
    .write     = fat32_write,
    .flush     = fat32_flush,
    .fsync     = fat32_fsync,
    .release   = fat32_release,
    .ftruncate = fat32_ftruncate,
    .unlink    = fat32_unlink,
    .rmdir     = fat32_rmdir,
    .mkdir     = fat32_mkdir,
    .rename    = fat32_rename,
    .statfs    = fat32_statfs
};

int main(int argc, char *argv[])
{
    int ret=0;

    ret = fuse_main(argc, argv, &fat32_oper, NULL);

    return ret;
}

