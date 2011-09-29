#ifndef __DIRECCIONAMIENTO_H_
#define __DIRECCIONAMIENTO_H_

#include <stdint.h>

uint8_t fat32_getsectors(uint32_t sector, uint32_t cantidad, void *buffer);
uint8_t fat32_getcluster(uint32_t cluster, void *buffer);

#endif //__DIRECCIONAMIENTO_H_
