#define FUSE_USE_VERSION  28

#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#include "nipc.h"

uint8_t *server_host = (uint8_t *)"asd.com";
uint16_t server_port = 1500;

static const char *hello_str = "Hello World!\n"; ////
static const char *hello_path = "/hello"; ////

typedef struct fat32_t {
    uint16_t bytes_per_sector;   // 0x0B 2
    uint8_t  sector_per_cluster; // 0x0D 1
    uint16_t reserved_sectors;   // 0x0E 2
    uint8_t  fat_count;          // 0x10 1
    uint32_t total_sectors;      // 0x20 4
    uint32_t sectors_per_fat;    // 0x24 4
    uint32_t system_area_size;   // 0x0E + 0x10 * 0x24
    int32_t  free_clusters;
    uint8_t *boot;
    uint8_t *fs_info;
    uint32_t *fat;
} fat32_t;

nipc_socket *socket;
fat32_t fat;

uint8_t *fat32_getcluster(uint32_t cluster)
{
//    if(!sector_buffer) free(sector_buffer);
//    sector_buffer = calloc(bytes_per_sector, sector_per_cluster);
//
////  SSA=RSC(0x0E) + FN(0x10) * SF(0x24)
////  LSN=SSA + (CN-2) × SC(0x0D)
//
//    fseek(SEEK_SET, sector_per_cluster*bytes_per_sector); ////
//    fread(sector_buffer,bytes_per_sector,sector_per_cluster,fp); ////

    return NULL;
}

uint8_t *fat32_getsectors(uint32_t sector, uint32_t cantidad)
{
    uint8_t *buffer=0;
    nipc_packet packet;
    int i;

    packet.type = nipc_req_packet;
    packet.len = sizeof(int32_t);
    packet.payload = calloc(sizeof(int32_t), 1);
    memcpy(packet.payload, &sector, sizeof(int32_t));

    //Pide los sectores necesarios
    for (i = 0 ; i < cantidad ; i++)
    {
        nipc_send_packet(&packet, socket);
        *((int32_t *)packet.payload) += 1;
    }

    free(packet.payload);

    buffer = calloc(fat.bytes_per_sector, cantidad);

    //Espera a obtener todos los sectores pedidos
    for (i = 0 ; i < cantidad ; i++)
    {
        nipc_packet *packet = nipc_recv_packet(socket);
        int32_t rta_sector;
        memcpy(&rta_sector, packet->payload, sizeof(int32_t));
        memcpy(buffer+((rta_sector-sector)*fat.bytes_per_sector), packet->payload+4, fat.bytes_per_sector);
        free(packet);
    }

    return buffer;
}


int fat32_create (const char *path, mode_t mode, struct fuse_file_info *fi)
{
    printf("Create\n");
    return 0;
}

//int fat32_open (const char *path, struct fuse_file_info *fi)
//{
//
//}

//int fat32_read (const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
//{
//
//}

int fat32_write (const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    printf("Write\n");
    return 0;
}

int fat32_flush (const char *path, struct fuse_file_info *fi)
{
    printf("Flush\n");
    return 0;
}

int fat32_release (const char *path, struct fuse_file_info *fi)
{
    printf("Release\n");
    return 0;
}

int fat32_ftruncate (const char *path, off_t offset, struct fuse_file_info *fi)
{
    printf("Ftruncate\n");
    return 0;
}

int fat32_unlink (const char *path)
{
    printf("Unlink\n");
    return 0;
}

int fat32_rmdir (const char *path)
{
    printf("RmDir\n");
    return 0;
}

int fat32_mkdir (const char *path, mode_t mode)
{
    printf("MkDir\n");
    return 0;
}

//int fat32_readdir (const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi)
//{
//
//}

//int fat32_fgetattr (const char *path, struct stat *stbuf, struct fuse_file_info *fi)
//{
//
//}

int fat32_rename (const char *from, const char *to)
{
    printf("Rename\n");
    return 0;
}



static int fat32_getattr(const char *path, struct stat *stbuf)
{
    int res = 0;
    printf("Getattr\n");
    memset(stbuf, 0, sizeof(struct stat));
    printf("%s\n", path);
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
    printf("Readdir\n");

    if(strcmp(path, "/") != 0)
        return -ENOENT;

    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);
    filler(buf, hello_path + 1, NULL, 0);

    return 0;
}

static int fat32_open(const char *path, struct fuse_file_info *fi)
{
    printf("Open\n");
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
    printf("Read\n");
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

int fat32_free_clusters()
{
    if(fat.free_clusters < 0)
    {
        int i=0, free_clusters=0;

        for ( i = 2 ; i < fat.bytes_per_sector * fat.sectors_per_fat / sizeof(int32_t) ; i++ )
            if(fat.fat[i] == 0)
                free_clusters++;
        
        fat.free_clusters = free_clusters;

        //TODO actualizar fs_info_sector del volumen 

        return free_clusters;
    } else {
        return fat.free_clusters;
    }

}

static void *fat32_init(struct fuse_conn_info *conn)
{
    printf("Init\n");

    fat.boot = fat32_getsectors(0,1);

    memcpy(&(fat.bytes_per_sector),   fat.boot + 0x0B, 2);
    memcpy(&(fat.sector_per_cluster), fat.boot + 0x0D, 1);
    memcpy(&(fat.reserved_sectors),   fat.boot + 0x0E, 2);
    memcpy(&(fat.fat_count),          fat.boot + 0x10, 1);
    memcpy(&(fat.total_sectors),      fat.boot + 0x20, 4);
    memcpy(&(fat.sectors_per_fat),    fat.boot + 0x24, 4);

    fat.system_area_size = fat.reserved_sectors + ( fat.fat_count * fat.sectors_per_fat );

    fat.fs_info = fat32_getsectors(1,1);
    memcpy(&(fat.free_clusters), fat.fs_info + 0x1E8, 4);

    fat.fat =(uint32_t *) fat32_getsectors(fat.reserved_sectors, fat.sectors_per_fat);
    printf("BPS:%d - SPC:%d - RS:%d - FC:%d - TS:%d - SPF:%d - SAS:%d clusters libres:%d -\n",
            fat.bytes_per_sector,
            fat.sector_per_cluster,
            fat.reserved_sectors,
            fat.fat_count,
            fat.total_sectors,
            fat.sectors_per_fat,
            fat.system_area_size,
            fat32_free_clusters());

    return NULL;

}

void fat32_destroy(void * foo)
{
    //free(fat.boot);
    //free(fat.fs_info);
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

void fat32_handshake(nipc_socket *socket)
{
    nipc_packet *packet = malloc(sizeof(nipc_packet));

    packet->type = nipc_handshake;
    packet->len = 0;
    packet->payload = NULL;
    nipc_send_packet(packet, socket);
    free(packet);

    packet = nipc_recv_packet(socket);
    if (!packet->payload)
    {
        printf("Error: %s\n", packet->payload);
        exit(-ECONNREFUSED);
    }
    free(packet);

    return;
}

int main(int argc, char *argv[])
{
    int ret=0;

    //En el PPD se asume esto, no se si en el PFS hay que asumirlo tmb
    fat.bytes_per_sector = 512;

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
