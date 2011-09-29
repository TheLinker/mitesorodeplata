#define FUSE_USE_VERSION  28

#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

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
    printf("Release: path: %s\n", path);
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
    int res = 0;
    printf("Getattr: path: %s\n", path);
    memset(stbuf, 0, sizeof(struct stat));
    if(strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
    }
    else if(strcmp(path, hello_path) == 0) {
        stbuf->st_mode = S_IFREG | 0444;
        stbuf->st_nlink = 1;
        stbuf->st_size = strlen(hello_str);
    }
    else
        res = -ENOENT;

    return res;
}

static int fat32_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                         off_t offset, struct fuse_file_info *fi)
{
    (void) offset;
    (void) fi;
    printf("Readdir: path: %s\n", path);

    if(strcmp(path, "/") != 0)
        return -ENOENT;

    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);
    filler(buf, hello_path + 1, NULL, 0);

    return 0;
}

static int fat32_open(const char *path, struct fuse_file_info *fi)
{
    printf("Open: path: %s\n", path);
    if(strcmp(path, hello_path) != 0)
        return -ENOENT;

    if((fi->flags & 3) != O_RDONLY)
        return -EACCES;

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

    fat32_getsectors(0, 1, fat.boot_sector.buffer);
    fat.system_area_size = fat.boot_sector.reserved_sectors + ( fat.boot_sector.fat_count * fat.boot_sector.sectors_per_fat );

    if (fat.boot_sector.bytes_per_sector != 512)
        printf("Nos dijeron que se suponian 512 bytes por sector, no %d bytes >.<\n", fat.boot_sector.bytes_per_sector);

    fat32_getsectors(1, 1, fat.fsinfo_sector.buffer);

    fat.fat = calloc(fat.boot_sector.bytes_per_sector, fat.boot_sector.sectors_per_fat);
    fat32_getsectors(fat.boot_sector.reserved_sectors, fat.boot_sector.sectors_per_fat, fat.fat);
    memcpy(&(fat.eoc_marker), fat.fat + 0x04, 4);

    printf("BPS:%d - SPC:%d - RS:%d - FC:%d - TS:%d - SPF:%d - SAS:%d clusters libres:%d -\n",
            fat.boot_sector.bytes_per_sector,
            fat.boot_sector.sectors_per_cluster,
            fat.boot_sector.reserved_sectors,
            fat.boot_sector.fat_count,
            fat.boot_sector.total_sectors,
            fat.boot_sector.sectors_per_fat,
            fat.system_area_size,
            fat32_free_clusters());

    return NULL;

}

void fat32_destroy(void * foo)
{
    free(fat.fat);
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

    fat.boot_sector.bytes_per_sector = 512;

    fat32_config_read();

    if(!(socket = nipc_init(server_host, server_port)))
    {
        printf("La conexion al RAID 1 o planificador de disco no esta lista\n");
        exit(-EADDRNOTAVAIL);
    }

    fat32_handshake(socket);

    ret = fuse_main(argc, argv, &fat32_oper, NULL);

    nipc_close(socket);

    return ret;
}
