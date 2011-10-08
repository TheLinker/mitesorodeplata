#ifndef __UTILS_H_
#define __UTILS_H_

#include <stdint.h>
#include "nipc.h"
#include "pfs.h"

uint32_t fat32_free_clusters(fs_fat32_t *fs_tmp);
int32_t fat32_first_free_cluster(fs_fat32_t *fs_tmp);
void fat32_handshake(nipc_socket *socket);
void fat32_add_cluster(int32_t first_cluster, fs_fat32_t *fs_tmp);
void fat32_remove_cluster(int32_t first_cluster, fs_fat32_t *fs_tmp);
int fat32_config_read(fs_fat32_t *fs_tmp);
int8_t fat32_get_entry(int32_t entry_number, int32_t first_cluster, uint8_t *buffer, fs_fat32_t *fs_tmp);
int32_t fat32_get_link_n_in_chain(int32_t first_cluster, int32_t cluster_offset, fs_fat32_t *fs_tmp);
file_attrs *fat32_get_file_list(int32_t first_cluster, fs_fat32_t *fs_tmp);

#endif //__UTILS_H_
