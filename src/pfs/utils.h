#ifndef __UTILS_H_
#define __UTILS_H_

#include <stdint.h>
#include "nipc.h"

uint32_t fat32_free_clusters();
int32_t fat32_first_free_cluster();
void fat32_handshake(nipc_socket *socket);
void fat32_add_cluster(int32_t first_cluster);
void fat32_remove_cluster(int32_t first_cluster);
int fat32_config_read();

#endif //__UTILS_H_
