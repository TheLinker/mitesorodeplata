#ifndef __UTILS_H_
#define __UTILS_H_

#include <stdint.h>
#include "nipc.h"
#include "pfs.h"

uint32_t    fat32_free_clusters(fs_fat32_t *fs_tmp);
int32_t     fat32_first_free_cluster(fs_fat32_t *fs_tmp);
void        fat32_handshake(nipc_socket socket);
void        fat32_add_cluster(int32_t first_cluster, fs_fat32_t *fs_tmp, nipc_socket socket);
void        fat32_remove_cluster(int32_t first_cluster, fs_fat32_t *fs_tmp, nipc_socket socket);
int         fat32_config_read(fs_fat32_t *fs_tmp);
int8_t      fat32_get_entry(int32_t entry_number, int32_t first_cluster, file_entry_t *buffer, fs_fat32_t *fs_tmp, nipc_socket socket);
int32_t     fat32_get_link_n_in_chain(int32_t first_cluster, int32_t cluster_offset, fs_fat32_t *fs_tmp);
uint32_t    fat32_get_file_list(int32_t first_cluster, file_attrs *file_list, fs_fat32_t *fs_tmp, nipc_socket socket);
int32_t     fat32_get_file_from_path(const uint8_t *path, file_attrs *ret_attrs, fs_fat32_t *fs_tmp, nipc_socket socket);
int32_t     fat32_get_entry_from_name(uint8_t *name, file_attrs *file_list, int32_t file_list_len);
void        fat32_build_name(file_attrs *file, int8_t *ret_name);
int32_t     fat32_first_dual_fentry(int32_t first_cluster, fs_fat32_t *fs_tmp, nipc_socket socket);
int32_t     fat32_get_block_from_dentry(int32_t first_cluster, int32_t entry_index, fs_fat32_t *fs_tmp);
void        hex_log(FILE *fp, unsigned char * buff, int cantidad);
nipc_socket fat32_get_socket(fs_fat32_t *fs_tmp);
void        fat32_free_socket(nipc_socket socket, fs_fat32_t *fs_tmp);

#endif //__UTILS_H_
