#define FUSE_USE_VERSION  28

#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#include "pfs_utils.h"
#include "utils.h"
#include "pfs.h"
#include "direccionamiento.h"

static const char *hello_str = "Hello World!\n"; ////
static const char *hello_path = "/hello"; ////

int fat32_create (const char *path, mode_t mode, struct fuse_file_info *fi)
{
    printf("Create: path:%s\n", path);
    return 0;
}

int fat32_write (const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    printf("Write: path: %s\n", path);
    return 0;
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
    printf("Ftruncate: path: %s\n", path);
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

    int32_t ret = fat32_get_file_from_path((const uint8_t *) path, &ret_attrs, fs_tmp);
    if(ret<0) return ret;

    ret = fat32_get_free_file_entry(fs_tmp->open_files);
    if(ret<0) return ret;

    memcpy(fs_tmp->open_files[ret].path, path, strlen(path));
    fat32_build_name(&ret_attrs, fs_tmp->open_files[ret].filename);
    fs_tmp->open_files[ret].file_size = ret_attrs.file_size;
    fs_tmp->open_files[ret].file_pos = (fi->flags & O_APPEND)?0:ret_attrs.file_size-1;
    fs_tmp->open_files[ret].first_cluster = ret_attrs.first_cluster;
    fs_tmp->open_files[ret].busy = TRUE;

    //seteamos el file handler del file_info como el indice en la tabla de archivos abiertos
    fi->fh = ret;
    log_info(fs_tmp->log, "un_thread", " ret_val: %d\n", ret);

    return 0;
}

static int fat32_read(const char *path, char *buf, size_t size, off_t offset,
                      struct fuse_file_info *fi)
{
    size_t len;
    (void) fi;
    printf("Read: path: %s\n", path);
    if(strcmp(path, hello_path) != 0)
        return -ENOENT;

    len = strlen(hello_str);
    if (offset < len) {
        if (offset + size > len)
            size = len - offset;
        memcpy(buf, hello_str + offset, size);
    } else
        size = 0;

    return size;
}

static void *fat32_init(struct fuse_conn_info *conn)
{
    printf("Init\n");

    fs_fat32_t *fs_tmp = calloc(sizeof(fs_fat32_t), 1);
    fs_tmp->boot_sector.bytes_per_sector = 512;

    fat32_config_read(fs_tmp);

    fs_tmp->log = log_new( fs_tmp->log_path, "process_file_system", fs_tmp->log_mode );

    if(!(fs_tmp->socket = nipc_init(fs_tmp->server_host, fs_tmp->server_port))) {
        printf("La conexion al RAID 1 o planificador de disco no esta lista\n");
        exit(-EADDRNOTAVAIL);
    }

    fat32_handshake(fs_tmp->socket);

    fat32_getsectors(0, 1, fs_tmp->boot_sector.buffer, fs_tmp);
    fs_tmp->system_area_size = fs_tmp->boot_sector.reserved_sectors + ( fs_tmp->boot_sector.fat_count * fs_tmp->boot_sector.sectors_per_fat );

    if (fs_tmp->boot_sector.bytes_per_sector != 512)
        printf("Nos dijeron que se suponian 512 bytes por sector, no %d bytes >.<\n", fs_tmp->boot_sector.bytes_per_sector);

    fat32_getsectors(1, 1, fs_tmp->fsinfo_sector.buffer, fs_tmp);

    fs_tmp->fat = calloc(fs_tmp->boot_sector.bytes_per_sector, fs_tmp->boot_sector.sectors_per_fat);
    fat32_getsectors(fs_tmp->boot_sector.reserved_sectors, fs_tmp->boot_sector.sectors_per_fat, fs_tmp->fat, fs_tmp);
    memcpy(&(fs_tmp->eoc_marker), fs_tmp->fat + 0x04, 4);

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