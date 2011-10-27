#ifndef __PFS_FILES_H_
#define __PFS_FILES_H_
#include <stdint.h>
#include <errno.h>

#define MAX_OPEN_FILES 512

typedef struct file_descriptor {
    int8_t    path[1024];
    int8_t    filename[512];
    uint32_t  file_size;
    uint32_t  file_pos;
    int32_t   first_cluster;
    int32_t   container_cluster;
    int32_t   entry_index;
    int8_t    busy; //1 no disponible, 0 disponible
} file_descriptor;

int32_t fat32_get_free_file_entry(file_descriptor *tabla);

#endif //__PFS_FILES_H_
