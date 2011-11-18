#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "pfs.h"
#include "pfs_utils.h"

void *fat32_consola(void *arg)
{
    char w1[1024], path[1024];
    fs_fat32_t *fs_tmp = (fs_fat32_t *)arg;
    int8_t argc;

    while(42)
    {
        fprintf(stdout, "PFS> ");
        argc = fscanf(stdin, "%s%*[ ]%s", w1, path);

        if (!strcmp(w1, "fsinfo")) {
            printf("Clusters Ocupados: %d\n"
                   "Clusters Libres: %d\n"
                   "Tamaño de un sector: %d\n"
                   "Tamaño de un cluster: %d\n"
                   "Tamaño de la Tabla FAT: %d\n",
                   (fs_tmp->boot_sector.total_sectors - fs_tmp->system_area_size) /
                       fs_tmp->boot_sector.sectors_per_cluster - fat32_free_clusters(fs_tmp),
                   fat32_free_clusters(fs_tmp),
                   fs_tmp->boot_sector.bytes_per_sector,
                   fs_tmp->boot_sector.bytes_per_sector * fs_tmp->boot_sector.sectors_per_cluster,
                   fs_tmp->boot_sector.bytes_per_sector * fs_tmp->boot_sector.sectors_per_fat / 1024);
        } else if (!strcmp(w1, "finfo") && (argc == 2)) {
            file_attrs file;
            int32_t ret = fat32_get_file_from_path((uint8_t *)path, &file, fs_tmp);

            if(ret == -ENOENT)
                printf("Archivo o directorio no encontrado\n");

            int32_t cluster_actual = file.first_cluster;
            int8_t  contador = 0;

            while ((cluster_actual != fs_tmp->eoc_marker) && (contador < 20))
            {
                contador++;
                printf("%d -> ", cluster_actual);
                cluster_actual = fat32_get_link_n_in_chain(cluster_actual, 1, fs_tmp);
            }

            if (contador < 20)
                printf("EOC\n");
            else printf("...\n");

        } else
            log_warning(fs_tmp->log, "consola", "Comando '%s' no reconocido o argumenos inválidos", w1);

        memset(w1, '\0', sizeof(w1));
        memset(path, '\0', sizeof(path));
    }
}

