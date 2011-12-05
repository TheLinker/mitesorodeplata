#define FUSE_USE_VERSION  28

#define SECT_SIZE 512
#define BLOCK_SIZE 1024
#define SECTORS_PER_BLOCK (BLOCK_SIZE / SECT_SIZE)

#define MAX_OPEN_FILES 512

#define MAX(a,b) (a)>(b)?(a):(b)
#define MIN(a,b) (a)<(b)?(a):(b)

#define NO_CACHE -1
