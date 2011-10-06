#ifndef __DIRECCIONAMIENTO_H_
#define __DIRECCIONAMIENTO_H_

#include <stdint.h>
#include "pfs.h"

uint8_t fat32_getsectors(uint32_t sector, uint32_t cantidad, void *buffer, fs_fat32_t *fs_tmp);
uint8_t fat32_getcluster(uint32_t cluster, void *buffer, fs_fat32_t *fs_tmp);

#endif //__DIRECCIONAMIENTO_H_
