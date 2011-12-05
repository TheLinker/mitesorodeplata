#include "pfs_files.h"
#include "pfs.h"

#include <string.h>

int32_t fat32_get_free_file_entry(file_descriptor *tabla)
{
    int i=0;

    for( i = 0 ; i < MAX_OPEN_FILES ; i++ )
        if ( !tabla[i].busy ) break;

    if ( i == MAX_OPEN_FILES ) return -EMFILE;

    return i;
}

int8_t fat32_is_in_cache(int32_t block, cache_t *cache, int8_t *buffer, struct fs_fat32_t *fs_tmp)
{
    int i=0;
    for(i=0;i<fs_tmp->cache_size;i++)
        if(cache[i].number == block)
        {
            memcpy(buffer, cache[i].contenido, BLOCK_SIZE);
            return TRUE;
        }

    return FALSE;
}

int32_t fat32_get_cache_slot(cache_t *cache, struct fs_fat32_t *fs_tmp)
{
    //Vamos por LRU como dijo Facundo , aunque Clock 2nd chance me gustaba mas
    uint32_t i=0, slot=0, stamp=0xFFFFFFFF;

    //Si hay alguno libre, retornamos ese
    for (i=0;i<fs_tmp->cache_size;i++)
        if(cache[i].number == -1)
            return i;

    //Eleginos el de menor timestamp
    for (i=0;i<fs_tmp->cache_size;i++)
        if(cache[i].stamp < stamp)
        {
            slot = i;
            stamp = cache[i].stamp;
        }

    return slot;
}

void fat32_save_in_cache(int32_t slot, int32_t block, int8_t *block_cache, int8_t modif, cache_t *cache)
{
    static int stamp = 0;
    memcpy(cache[slot].contenido, block_cache, BLOCK_SIZE);
    cache[slot].number = block;
    cache[slot].modificado = modif;
    cache[slot].stamp = stamp++;
}
