#include "pfs_files.h"

int32_t fat32_get_free_file_entry(file_descriptor *tabla)
{
    int i=0;

    for( i = 0 ; i < MAX_OPEN_FILES ; i++ )
        if ( !tabla[i].busy ) break; 

    if ( i == MAX_OPEN_FILES ) return -EMFILE;

    return i;
}

