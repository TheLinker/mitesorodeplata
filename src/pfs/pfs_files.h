#ifndef __PFS_FILES_H_
#define __PFS_FILES_H_
#include "defs.h"

#include <stdint.h>
#include <errno.h>

#include <semaphore.h>

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

int32_t fat32_get_free_file_entry(file_descriptor *tabla);

#endif //__PFS_FILES_H_
