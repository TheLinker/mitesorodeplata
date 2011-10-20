PFS_BIN = $(BIN_PATH)/process_file_system

PFS_SRC = $(SRC_PATH)/common/ex_nipc.c \
          $(SRC_PATH)/common/utils.c \
          $(SRC_PATH)/common/log.c \
          $(SRC_PATH)/pfs/direccionamiento.c \
          $(SRC_PATH)/pfs/pfs_utils.c \
          $(SRC_PATH)/pfs/pfs_files.c \
          $(SRC_PATH)/pfs/pfs.c \

PFS_INCLUDES = $(SRC_PATH)/common/ex_nipc.h \
               $(SRC_PATH)/common/utils.h \
               $(SRC_PATH)/common/log.h \
               $(SRC_PATH)/pfs/direccionamiento.h \
               $(SRC_PATH)/pfs/pfs_utils.h \
               $(SRC_PATH)/pfs/pfs_files.h \
               $(SRC_PATH)/pfs/pfs.h \

