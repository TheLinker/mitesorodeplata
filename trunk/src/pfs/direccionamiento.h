#ifndef __DIRECCIONAMIENTO_H_
#define __DIRECCIONAMIENTO_H_

#include <stdint.h>
struct fs_fat32_t;

#define BLOCK_SIZE 1024
#define SECTORS_PER_BLOCK (BLOCK_SIZE / 512)

uint8_t fat32_getblock(uint32_t block, uint32_t cantidad, void *buffer, struct fs_fat32_t *fs_tmp);
uint8_t fat32_writeblock(uint32_t block, uint32_t cantidad, void *buffer, struct fs_fat32_t *fs_tmp);

#endif //__DIRECCIONAMIENTO_H_
