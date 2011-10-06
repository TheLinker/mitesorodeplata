#ifndef __PFS_H_
#define __PFS_H_

#include "nipc.h"

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
    } __attribute__ ((packed));
} fsinfo_t;

typedef struct fs_fat32_t {
    boot_t   boot_sector;
    fsinfo_t fsinfo_sector;
    uint32_t *fat;               // usar estructuras para la fat es inutil =)

    uint32_t system_area_size;   // 0x0E + 0x10 * 0x24
    int32_t  eoc_marker;         // Marca usada para fin de cadena de clusters

    uint8_t  server_host[1024];
    uint16_t server_port;
    uint16_t cache_size;

    nipc_socket *socket;
} __attribute__ ((packed)) fs_fat32_t;


#endif //__PFS_H_