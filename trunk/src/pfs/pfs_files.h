#ifndef __PFS_FILES_H_
#define __PFS_FILES_H_
#include "defs.h"
struct fs_fat32_t;

#include <stdint.h>
#include <errno.h>

#include <semaphore.h>

typedef struct cache_t {
    int32_t number;
    int8_t  modificado;
    int32_t stamp;

    int8_t  contenido[BLOCK_SIZE];
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
int8_t  fat32_is_in_cache(int32_t block, cache_t *cache, int8_t *buffer, struct fs_fat32_t *fs_tmp);
int32_t fat32_get_cache_slot(cache_t *cache, struct fs_fat32_t *fs_tmp);
void    fat32_save_in_cache(int32_t slot, int32_t block, int8_t *block_cache, int8_t modif, cache_t *cache);

#endif //__PFS_FILES_H_
