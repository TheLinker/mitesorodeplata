#include "pfs_files.h"

int32_t fat32_get_free_file_entry(file_descriptor *tabla)
{
    int i=0;

    for( i = 0 ; i < MAX_OPEN_FILES ; i++ )
        if ( !tabla[i].busy ) break; 

    if ( i == MAX_OPEN_FILES ) return -EMFILE;

    return i;
}

/**
 * Obtiene un *cluster*.
 * NOTA: el *buffer* debe tener el tamaño suficiente para aceptar los datos.
 *
 * @cluster: Cluster a pedir.
 * @buffer:  Lugar donde se almacena la información recibida.
 * @fs_tmp:  Estructura privada del file system// Ver Wiki.
 * @return:  Código de error o 0 si fue todo bien.
 */
uint8_t fat32_getcluster(uint32_t cluster, void *buffer, struct fs_fat32_t *fs_tmp)
{
    //cluster 0 y 1 estan reservados y es invalido pedir esos clusters
    if(cluster<2) return -EINVAL;

//// SSA =                  | RSC = Reserved Sector Count  | FN = Number of FAT 
//// SF = sectors per FAT   | LSN = Logical Sector Number  | CN = Cluster Number  | SC = Sectors per Cluster

////  SSA=RSC(0x0E) + FN(0x10) * SF(0x24)
////  LSN=SSA + (CN-2) × SC(0x0D)

    uint32_t block_number = 0;

    block_number = (fs_tmp->system_area_size + (cluster - 2) * fs_tmp->boot_sector.sectors_per_cluster) / SECTORS_PER_BLOCK;

    fat32_getblock(block_number,
                   fs_tmp->boot_sector.sectors_per_cluster / SECTORS_PER_BLOCK,
                   buffer, fs_tmp);

    return 0;
}

/**
 * Escribe un *cluster*.
 * NOTA: el *buffer* debe tener el tamaño suficiente para aceptar los datos.
 *
 * @cluster: Cluster a escribir.
 * @buffer:  Lugar donde se almacena la información a escribir.
 * @fs_tmp:  Estructura privada del file system// Ver Wiki.
 * @return:  Código de error o 0 si fue todo bien.
 */
uint8_t fat32_writecluster(uint32_t cluster, void *buffer, struct fs_fat32_t *fs_tmp)
{
    //cluster 0 y 1 estan reservados y es invalido pedir esos clusters
    if(cluster<2) return -EINVAL;

//// SSA = Size of System Area   | RSC = Reserved Sector Count  | FN = Number of FAT 
//// SF = sectors per FAT        | LSN = Logical Sector Number  | CN = Cluster Number  | SC = Sectors per Cluster

////  SSA=RSC(0x0E) + FN(0x10) * SF(0x24)
////  LSN=SSA + (CN-2) × SC(0x0D)

    uint32_t block_number = 0;

    block_number = (fs_tmp->system_area_size + (cluster - 2) * fs_tmp->boot_sector.sectors_per_cluster) / SECTORS_PER_BLOCK;

    fat32_writeblock(block_number,
                     fs_tmp->boot_sector.sectors_per_cluster / SECTORS_PER_BLOCK,
                     buffer, fs_tmp);

    return 0;
}


