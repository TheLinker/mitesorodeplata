#ifndef __PFS_H_
#define __PFS_H_

nipc_socket *socket;

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

typedef struct fat32_t {
    boot_t   boot_sector;
    fsinfo_t fsinfo_sector;
    uint32_t *fat;               // usar estructuras para la fat es inutil =)

    uint32_t system_area_size;   // 0x0E + 0x10 * 0x24
    int32_t  eoc_marker;         // Marca usada para fin de cadena de clusters
} __attribute__ ((packed)) fat32_t;

nipc_socket *socket;
fat32_t fat;

#endif //__PFS_H_
