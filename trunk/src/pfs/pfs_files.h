#ifndef __PFS_FILES_H_
#define __PFS_FILES_H_

struct fs_fat32_t;
#define MAX_OPEN_FILES 512

#include <stdint.h>
#include <errno.h>
#include <semaphore.h>
#include "direccionamiento.h"


typedef struct cache_t {
    int32_t number;
    int8_t modificado;
    int8_t usado;

    int8_t contenido[BLOCK_SIZE];
} cache_t;

typedef struct file_descriptor {
    int8_t    path[1024];
    int8_t    filename[512];
    uint32_t  file_size;
    int32_t   first_cluster;
    int32_t   container_cluster;
    int32_t   entry_index;
    int8_t    busy; //1 no disponible, 0 disponible
    sem_t     sem_cache;
    cache_t  *cache;
} file_descriptor;

#include "pfs.h"
int32_t fat32_get_free_file_entry(file_descriptor *tabla);

uint8_t fat32_getcluster(uint32_t cluster, void *buffer, struct fs_fat32_t *fs_tmp);
uint8_t fat32_writecluster(uint32_t cluster, void *buffer, struct fs_fat32_t *fs_tmp);

#endif //__PFS_FILES_H_
