#Magia, no tocar.

FLAGS      = -Wall -Isrc/common/
LIBFLAGS   = -licui18n -licuuc
BUILD_PATH = tmp
BIN_PATH   = bin
SRC_PATH   = src

include pfs.mk
include praid.mk

all: pfs praid

##############################
#Reglas para el Proceso File System
##############################

pfs: $(PFS_BIN)

PFS_OBJS = $(addprefix $(BUILD_PATH)/, $(patsubst %.c, %.o, $(notdir $(PFS_SRC) ) ) )

$(PFS_BIN): $(PFS_OBJS) $(PFS_INCLUDES)
	@echo 'LINKEANDO $@'
	@gcc $(LIBFLAGS) `pkg-config fuse --libs` $(PFS_OBJS) -o $@



##############################
#Reglas para el Proceso RAID 1
##############################

praid: $(PRAID_BIN)

PRAID_OBJS = $(addprefix $(BUILD_PATH)/, $(patsubst %.c, %.o, $(notdir $(PRAID_SRC) ) ) )

$(PRAID_BIN): $(PRAID_OBJS) $(PRAID_INCLUDES)
	@echo 'LINKEANDO $@'
	@gcc $(LIBFLAGS) `pkg-config fuse --libs` $(PRAID_OBJS) -o $@



##############################
#Reglas de compilacion
##############################

$(BUILD_PATH)/%.o: $(SRC_PATH)/common/%.c $(SRC_PATH)/common/%.h
	@echo 'COMPILANDO $< -> $@'
	@gcc $(FLAGS) `pkg-config fuse --cflags` $< -o $@ -c

$(BUILD_PATH)/%.o: $(SRC_PATH)/pfs/%.c $(PFS_INCLUDES)
	@echo 'COMPILANDO $< -> $@'
	@gcc $(FLAGS) `pkg-config fuse --cflags` $< -o $@ -c

$(BUILD_PATH)/%.o: $(SRC_PATH)/praid1/%.c $(PRAID_INCLUDES)
	@echo 'COMPILANDO $< -> $@'
	@gcc $(FLAGS) `pkg-config fuse --cflags` $< -o $@ -c


clean:
	@echo 'Limpiando todo'
	@rm $(BUILD_PATH)/* $(PFS_BIN) $(PRAID_BIN) 2> /dev/null || true

.PHONY: pfs praid clean all

