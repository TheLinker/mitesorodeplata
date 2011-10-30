#define FUSE_USE_VERSION  28

#define MAX(a,b) (a)>(b)?(a):(b)

#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#include "pfs_utils.h"
#include "pfs_consola.h"
#include "utils.h"
#include "pfs.h"
#include "direccionamiento.h"

int fat32_create (const char *path, mode_t mode, struct fuse_file_info *fi)
{
    printf("Create: path:%s\n", path);
    return 0;
}

int fat32_write (const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
     struct fuse_context* context = fuse_get_context();
    fs_fat32_t *fs_tmp = (fs_fat32_t *) context->private_data;
    int32_t cluster_size = fs_tmp->boot_sector.sectors_per_cluster * fs_tmp->boot_sector.bytes_per_sector;

    log_info(fs_tmp->log, "un_thread", "Write: path: %s size: %d offset: %ld", path, size, offset);

    if (strcmp((char *)fs_tmp->open_files[fi->fh].path, path) != 0)
        return -EINVAL;

    int32_t cluster_actual = fs_tmp->open_files[fi->fh].first_cluster;

    cluster_actual = fat32_get_link_n_in_chain( cluster_actual, offset / cluster_size, fs_tmp);

    //Suponemos que FUSE llama a truncate para pedir espacio si es que detecta que no tiene.

    int8_t *buffer = calloc(cluster_size, 1);
    size_t data_to_write = size;
    offset = offset % cluster_size;

    fat32_getcluster(cluster_actual, buffer, fs_tmp);
    int32_t data_cluster = cluster_size - offset;

    memcpy(buffer + offset, buf, data_cluster);

    fat32_writecluster(cluster_actual, buffer, fs_tmp);

    data_to_write -= data_cluster;

    while(data_to_write) {
        data_cluster = MAX(data_to_write, cluster_size);

        cluster_actual = fat32_get_link_n_in_chain( cluster_actual, 1, fs_tmp);

        if(data_cluster < cluster_size)
            fat32_getcluster(cluster_actual, buffer, fs_tmp);

        memcpy(buffer, buf+size-data_to_write, data_cluster);

        fat32_writecluster(cluster_actual, buffer, fs_tmp);

        data_to_write -= data_to_write;
    }

    return size; //TODO error handling
}

int fat32_flush (const char *path, struct fuse_file_info *fi)
{
    printf("Flush: path: %s\n", path);
    return 0;
}

int fat32_release (const char *path, struct fuse_file_info *fi)
{
    struct fuse_context* context = fuse_get_context();
    fs_fat32_t *fs_tmp = (fs_fat32_t *) context->private_data;

    log_info(fs_tmp->log, "un_thread", "Release: path: %s", path);

    if (strcmp((char *)fs_tmp->open_files[fi->fh].path, path) != 0)
        return -EINVAL;

    memset(&(fs_tmp->open_files[fi->fh]), '\0', sizeof(file_descriptor));

    return 0;
}

int fat32_ftruncate (const char *path, off_t offset, struct fuse_file_info *fi)
{
    struct fuse_context* context = fuse_get_context();
    fs_fat32_t *fs_tmp = (fs_fat32_t *) context->private_data;
    int32_t cluster_size = fs_tmp->boot_sector.sectors_per_cluster * fs_tmp->boot_sector.bytes_per_sector;

    log_info(fs_tmp->log, "un_thread", "Truncate: path: %s offset: %d", path, offset);

    if (strcmp((char *)fs_tmp->open_files[fi->fh].path, path) != 0)
        return -EINVAL;

    //Si no hay nada que modificar salimos exitosamente
    if(offset == fs_tmp->open_files[fi->fh].file_size)
        return 0;

    //Hay que modificar la FAT?
    int32_t cluster_abm = (offset / cluster_size) - (fs_tmp->open_files[fi->fh].file_size / cluster_size);

    //cluster_abm == 0 no modificar la FAT
    //     "      >  0 cantidad de clusters a agregar a la cadena
    //     "      <  0 cantidad de clusters a remover de la cadena

    file_attrs entry;
    int32_t entry_index = fs_tmp->open_files[fi->fh].entry_index;

    int8_t ret = fat32_get_entry(entry_index, fs_tmp->open_files[fi->fh].first_cluster, &entry, fs_tmp);
    if (ret<0) return ret;

    entry.file_size = offset;

    int32_t cluster_offset = entry_index / (fs_tmp->boot_sector.bytes_per_sector *
                                    fs_tmp->boot_sector.sectors_per_cluster / 32);

    int32_t entry_offset   = entry_index % (fs_tmp->boot_sector.bytes_per_sector *
                                    fs_tmp->boot_sector.sectors_per_cluster / 32);

    int32_t target_cluster = fat32_get_link_n_in_chain(fs_tmp->open_files[fi->fh].first_cluster, cluster_offset, fs_tmp);

    int32_t target_block = ( fs_tmp->system_area_size + (target_cluster - 2) * 
                     fs_tmp->boot_sector.sectors_per_cluster +
                     entry_offset / fs_tmp->boot_sector.sectors_per_cluster ) / SECTORS_PER_BLOCK;

    int8_t buffer[BLOCK_SIZE];
    fat32_getblock(target_block, 1, buffer, fs_tmp);

    //modificamos entry_offset para que sea con respecto al comienzo del bloque
    entry_offset = entry_offset % (SECTORS_PER_BLOCK * fs_tmp->boot_sector.bytes_per_sector / 32);

    memcpy(buffer + (entry_offset * 32), &buffer, BLOCK_SIZE);

    //Pedir la escritura del bloque
    fat32_writeblock(target_block, 1, &buffer, fs_tmp);

    if(cluster_abm > 0)
        while(cluster_abm)
        {
            fat32_add_cluster(fs_tmp->open_files[fi->fh].first_cluster, fs_tmp);
            cluster_abm--;
        }
    else if(cluster_abm < 0)
        while(cluster_abm)
        {
            fat32_remove_cluster(fs_tmp->open_files[fi->fh].first_cluster, fs_tmp);
            cluster_abm++;
        }

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

int fat32_rename (const char *from, const char *to)
{
    printf("Rename: from: %s to: %s\n", from, to);
    return 0;
}

static int fat32_getattr(const char *path, struct stat *stbuf)
{
    struct fuse_context* context = fuse_get_context();
    fs_fat32_t *fs_tmp = (fs_fat32_t *) context->private_data;

    log_info(fs_tmp->log, "un_thread", "Getattr: path: %s", path);
    memset(stbuf, 0, sizeof(struct stat));

    if(memcmp(path, "/", 2) == 0) {
        stbuf->st_mode = S_IFDIR | S_IRWXU | S_IRWXG | S_IRWXO;
        stbuf->st_nlink = 2;
        stbuf->st_uid = context->uid; /* user ID of owner */
        stbuf->st_gid = context->gid; /* group ID of owner */
        return 0;
    }

    file_attrs ret_attrs;
    int32_t ret = fat32_get_file_from_path((const uint8_t *) path, &ret_attrs, fs_tmp);
    if(ret<0) return ret;

    stbuf->st_mode = S_IRWXU | S_IRWXG | S_IRWXO; /* protection */
    stbuf->st_nlink = 1; /* number of hard links */
    stbuf->st_uid = context->uid; /* user ID of owner */
    stbuf->st_gid = context->gid; /* group ID of owner */
    stbuf->st_size = ret_attrs.file_size; /* total size, in bytes */
    stbuf->st_blksize = BLOCK_SIZE; /* blocksize for filesystem I/O */
    stbuf->st_blocks = (ret_attrs.file_size / BLOCK_SIZE) + 1; /* number of blocks allocated */

    if (ret_attrs.file_type == ATTR_SUBDIRECTORY)
        stbuf->st_mode |= S_IFDIR;
    else stbuf->st_mode |= S_IFREG;

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

    log_info(fs_tmp->log, "un_thread", "Readdir: path: %s", path);

    if(path[0] != '/')
        return -EINVAL;

    if(memcmp(path, "/", 2) == 0) {
        filler(buf, ".", NULL, 0);
        filler(buf, "..", NULL, 0);
        primer_cluster = 2;
    } else {
        file_attrs ret_attrs;
        int32_t ret = fat32_get_file_from_path((const uint8_t *) path, &ret_attrs, fs_tmp);
        if(ret<0) return ret;

        primer_cluster = ret_attrs.first_cluster;
    }

    int32_t cantidad_arch = fat32_get_file_list(primer_cluster, NULL, fs_tmp);
    file_attrs *lista = calloc(sizeof(file_attrs), cantidad_arch);
    fat32_get_file_list(primer_cluster, lista, fs_tmp);

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
    return 0;
}

static int fat32_open(const char *path, struct fuse_file_info *fi)
{
    struct fuse_context* context = fuse_get_context();
    fs_fat32_t *fs_tmp = (fs_fat32_t *) context->private_data;

    log_info(fs_tmp->log, "un_thread", "Open: path: %s", path);

    file_attrs ret_attrs;

    int32_t container_cluster = fat32_get_file_from_path((const uint8_t *) path, &ret_attrs, fs_tmp);
    if(container_cluster<0) return container_cluster;

    int32_t ret = fat32_get_free_file_entry(fs_tmp->open_files);
    if(ret<0) return ret;

    memcpy(fs_tmp->open_files[ret].path, path, strlen(path));
    fat32_build_name(&ret_attrs, fs_tmp->open_files[ret].filename);
    fs_tmp->open_files[ret].file_size = ret_attrs.file_size;
    fs_tmp->open_files[ret].file_pos = (fi->flags & O_APPEND)?0:ret_attrs.file_size-1;
    fs_tmp->open_files[ret].first_cluster = ret_attrs.first_cluster;
    fs_tmp->open_files[ret].busy = TRUE;
    fs_tmp->open_files[ret].container_cluster = container_cluster;
    fs_tmp->open_files[ret].entry_index = ret_attrs.entry_index;

    //seteamos el file handler del file_info como el indice en la tabla de archivos abiertos
    fi->fh = ret;

    return 0;
}

static int fat32_read(const char *path, char *buf, size_t size, off_t offset,
                      struct fuse_file_info *fi)
{
    struct fuse_context* context = fuse_get_context();
    fs_fat32_t *fs_tmp = (fs_fat32_t *) context->private_data;
    int32_t cluster_size = fs_tmp->boot_sector.sectors_per_cluster * fs_tmp->boot_sector.bytes_per_sector;

    log_info(fs_tmp->log, "un_thread", "Read: path: %s size: %d offset: %ld", path, size, offset);

    if (strcmp((char *)fs_tmp->open_files[fi->fh].path, path) != 0)
        return -EINVAL;

    int32_t cluster_actual = fs_tmp->open_files[fi->fh].first_cluster;

    cluster_actual = fat32_get_link_n_in_chain( cluster_actual, offset / cluster_size, fs_tmp);

    offset =  offset % cluster_size;

    int8_t *buffer = calloc(fs_tmp->boot_sector.bytes_per_sector, fs_tmp->boot_sector.sectors_per_cluster);
    fat32_getcluster(cluster_actual, buffer, fs_tmp);

    int32_t size_to_read = cluster_size - offset;
//    int32_t size_aldy_read = 0;
    if (size_to_read>size) size_to_read = size;
//    size_aldy_read += size_to_read;

    memcpy(buf, buffer, size_to_read);
    
//    Tal vez la vayamos a usar, segun lo que este pasando con esta funcion
//    especificamente con el size
//    while(size <= 0 && cluster_actual != fs_tmp->eoc_marker)
//    {
//        cluster_actual = fat32_get_link_n_in_chain(cluster_actual, 1, fs_tmp);
//    printf("1: %d\n", cluster_actual);
//        fat32_getcluster(cluster_actual, buffer, fs_tmp);
//        size_to_read = (size>cluster_size)?cluster_size:size;
//        memcpy(buf, buffer + size_aldy_read, size_to_read);
//
//        size_aldy_read += size_to_read;
//        size -= size_to_read;
//    }

    free(buffer);

    return size_to_read;
}

static void *fat32_init(struct fuse_conn_info *conn)
{
    printf("Init\n");

    fs_fat32_t *fs_tmp = calloc(sizeof(fs_fat32_t), 1);
    fs_tmp->boot_sector.bytes_per_sector = 512;

    fat32_config_read(fs_tmp);

    fs_tmp->log = log_new( fs_tmp->log_path, "process_file_system", fs_tmp->log_mode );

    if(!(fs_tmp->socket = create_socket((char *)fs_tmp->server_host, fs_tmp->server_port))) {
        printf("La conexion al RAID 1 o planificador de disco no esta lista\n");
        exit(-EADDRNOTAVAIL);
    }

    fat32_handshake(fs_tmp->socket);

    int8_t block[BLOCK_SIZE];

    fat32_getblock(0, 1, block, fs_tmp);
    memcpy(fs_tmp->boot_sector.buffer, block, fs_tmp->boot_sector.bytes_per_sector);
    fs_tmp->system_area_size = fs_tmp->boot_sector.reserved_sectors + ( fs_tmp->boot_sector.fat_count * fs_tmp->boot_sector.sectors_per_fat );

    if (fs_tmp->boot_sector.bytes_per_sector != 512)
        printf("Nos dijeron que se suponian 512 bytes por sector, no %d bytes >.<\n", fs_tmp->boot_sector.bytes_per_sector);

    memcpy(fs_tmp->fsinfo_sector.buffer, block + fs_tmp->boot_sector.bytes_per_sector, fs_tmp->boot_sector.bytes_per_sector);

    fs_tmp->fat = calloc(fs_tmp->boot_sector.bytes_per_sector, fs_tmp->boot_sector.sectors_per_fat);
    fat32_getblock(fs_tmp->boot_sector.reserved_sectors / SECTORS_PER_BLOCK,
                   fs_tmp->boot_sector.sectors_per_fat / SECTORS_PER_BLOCK, fs_tmp->fat, fs_tmp);
    memcpy(&(fs_tmp->eoc_marker), fs_tmp->fat + 0x04, 4);

    //creamos el thread de la consola
    pthread_create(fs_tmp->thread_consola, NULL, fat32_consola, fs_tmp);

    log_info(fs_tmp->log, "un_thread", "BPS:%d - SPC:%d - RS:%d - FC:%d - TS:%d - SPF:%d - SAS:%d clusters libres:%d -",
                                       fs_tmp->boot_sector.bytes_per_sector,
                                       fs_tmp->boot_sector.sectors_per_cluster,
                                       fs_tmp->boot_sector.reserved_sectors,
                                       fs_tmp->boot_sector.fat_count,
                                       fs_tmp->boot_sector.total_sectors,
                                       fs_tmp->boot_sector.sectors_per_fat,
                                       fs_tmp->system_area_size,
                                       fat32_free_clusters(fs_tmp));

    return fs_tmp;

}

void fat32_destroy(void * foo) //foo es private_data que devuelve el init
{
    nipc_close(((fs_fat32_t *)foo)->socket);

    log_delete(((fs_fat32_t *)foo)->log);

    free(((fs_fat32_t *)foo)->fat);
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
    .release   = fat32_release,
    .ftruncate = fat32_ftruncate,
    .unlink    = fat32_unlink,
    .rmdir     = fat32_rmdir,
    .mkdir     = fat32_mkdir,
    .rename    = fat32_rename,
};

int main(int argc, char *argv[])
{
    int ret=0;

    ret = fuse_main(argc, argv, &fat32_oper, NULL);

    return ret;
}
