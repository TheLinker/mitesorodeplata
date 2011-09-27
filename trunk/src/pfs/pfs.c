#define FUSE_USE_VERSION  28

#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#include "nipc.h"

uint8_t  server_host[1024] = "localhost";
uint16_t server_port = 1337;
uint16_t cache_size = 1024;

static const char *hello_str = "Hello World!\n"; ////
static const char *hello_path = "/hello"; ////

typedef union boot_t {
    uint8_t buffer[512];
    struct {
        uint8_t  no_usado[11];
        uint16_t bytes_per_sector;    // 0x0B 2
        uint8_t  sectors_per_cluster; // 0x0D 1
        uint16_t reserved_sectors;    // 0x0E 2
        uint8_t  fat_count;           // 0x10 1
        uint8_t  no_usado2[15];
        uint32_t total_sectors;       // 0x20 4
        uint32_t sectors_per_fat;     // 0x24 4
        uint8_t  no_usado3[472];
    } __attribute__ ((packed));       // para que no alinee los miembros de la estructura (si, estupido GCC)
} boot_t;

typedef union fsinfo_t {
    uint8_t buffer[512];
    struct {
        uint8_t  no_usado[488];
        int32_t  free_clusters;      // Clusters libres de la FS Info Sector
        uint8_t  no_usado2[20];
    };
} fsinfo_t;

typedef struct fat32_t {
    boot_t   boot_sector;
    fsinfo_t fsinfo_sector;
    uint32_t *fat;               // usar estructuras para la fat es inutil =)

    uint32_t system_area_size;   // 0x0E + 0x10 * 0x24
    int32_t  eoc_marker;         // Marca usada para fin de cadena de clusters
} fat32_t;

nipc_socket *socket;
fat32_t fat;

/**
 * Obtiene *cantidad* sectores a partir de *sector*
 * NOTA: el *buffer* debe tener el tamaño suficiente para aceptar los datos
 *
 * @sector primer sector a pedir
 * @cantidad cantidad de sectores a partir de *sector* a pedir (inclusive)
 * @buffer lugar donde guardar la información recibida.
 * @return codigo de error o 0 si fue todo bien.
 */
uint8_t fat32_getsectors(uint32_t sector, uint32_t cantidad, void *buffer)
{
    nipc_packet packet;
    int i;

    packet.type = nipc_req_packet;
    packet.len = sizeof(int32_t);
    memcpy(packet.payload, &sector, sizeof(int32_t));

    //Pide los sectores necesarios
    for (i = 0 ; i < cantidad ; i++)
    {
        nipc_send_packet(&packet, socket);
        *((int32_t *)packet.payload) += 1;
    }

    //Espera a obtener todos los sectores pedidos
    for (i = 0 ; i < cantidad ; i++)
    {
        nipc_packet *packet = nipc_recv_packet(socket);
        int32_t rta_sector;
        memcpy(&rta_sector, packet->payload, sizeof(int32_t));
        memcpy(buffer+((rta_sector-sector)*fat.boot_sector.bytes_per_sector), packet->payload+4, fat.boot_sector.bytes_per_sector);
        free(packet);
    }

    return 0;
}

/**
 * Obtiene un *cluster*
 * NOTA: el *buffer* debe tener el tamaño suficiente para aceptar los datos
 *
 * @cluster cluster a pedir
 * @buffer lugar donde guardar la información recibida.
 * @return codigo de error o 0 si fue todo bien.
 */
uint8_t fat32_getcluster(uint32_t cluster, void *buffer)
{
    //cluster 0 y 1 estan reservados y es invalido pedir esos clusters
    if(cluster<2) return -EINVAL;

////  SSA=RSC(0x0E) + FN(0x10) * SF(0x24)
////  LSN=SSA + (CN-2) × SC(0x0D)

    uint32_t logical_sector_number = 0;

    logical_sector_number = fat.system_area_size + (cluster - 2) * fat.boot_sector.sectors_per_cluster;

    fat32_getsectors(logical_sector_number , fat.boot_sector.sectors_per_cluster, buffer);

    return 0;
}


int fat32_create (const char *path, mode_t mode, struct fuse_file_info *fi)
{
    printf("Create: path:%s\n", path);
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
