#ifndef __PFS_H_
#define __PFS_H_

#include "nipc.h"
#include "log.h"
#define BLOCK_SIZE 1024

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

    char        log_path[1024];
    log_output  log_mode;
    log_t      *log;
} __attribute__ ((packed)) fs_fat32_t;

//lfn = long file name
typedef struct lfn_entry_t {
    uint8_t  seq_number;
    uint16_t lfn_name1[5];
    uint8_t  no_usado[2];
    uint8_t  checksum;
    uint16_t lfn_name2[6];
    uint8_t  no_usado2[2];
    uint16_t lfn_name3[2];
} __attribute__ ((packed)) lfn_entry_t;

typedef struct file_entry_t {
    uint8_t  dos_file_name[8];
    uint8_t  dos_file_ext[3];
    uint8_t  file_attr;
    uint8_t  no_usado[8];
    uint16_t first_cluster_hi;
    uint8_t  no_usado2[4];
    uint16_t first_cluster_low;
    uint32_t file_size;
} __attribute__ ((packed)) file_entry_t;

#define ATTR_SUBDIRECTORY   0x10
#define ATTR_ARCHIVE        0x20
#define ATTR_LONG_FILE_NAME 0x0F

#define LAST_LFN_ENTRY      0x40
#define DELETED_LFN         0x80

#define DELETED_FILE        0xE5
#define AVAIL_ENTRY         0x00

typedef struct file_attrs {
    uint8_t  filename[512];
    int32_t  filename_len;
    uint8_t  dos_filename[8];
    uint8_t  dos_fileext[3];
    uint8_t  file_type;
    uint32_t first_cluster;
    uint32_t file_size;
} __attribute__ ((packed)) file_attrs;

#endif //__PFS_H_
