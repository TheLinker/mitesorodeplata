#ifndef __DIRECCIONAMIENTO_H_
#define __DIRECCIONAMIENTO_H_

#include "defs.h"

#include <stdint.h>
#include "pfs.h"

uint8_t fat32_getblock(uint32_t block, uint32_t cantidad, void *buffer, fs_fat32_t *fs_tmp, nipc_socket socket);
uint8_t fat32_writeblock(uint32_t block, uint32_t cantidad, void *buffer, fs_fat32_t *fs_tmp, nipc_socket socket);
uint8_t fat32_getcluster(uint32_t cluster, void *buffer, fs_fat32_t *fs_tmp, nipc_socket socket);
uint8_t fat32_writecluster(uint32_t cluster, void *buffer, fs_fat32_t *fs_tmp, nipc_socket socket);

#endif //__DIRECCIONAMIENTO_H_
