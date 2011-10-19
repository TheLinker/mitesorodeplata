#ifndef __DIRECCIONAMIENTO_H_
#define __DIRECCIONAMIENTO_H_

#include <stdint.h>
#include "pfs.h"

#define BLOCK_SIZE 1024
#define SECTORS_PER_BLOCK BLOCK_SIZE / 512

uint8_t fat32_getblock(uint32_t block, uint32_t cantidad, void *buffer, fs_fat32_t *fs_tmp);
uint8_t fat32_getcluster(uint32_t cluster, void *buffer, fs_fat32_t *fs_tmp);

#endif //__DIRECCIONAMIENTO_H_
