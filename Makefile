#Magia, no tocar.

FLAGS      = -Wall -Isrc/common/ -Wno-unused-label `pkg-config fuse --cflags`
LIBFLAGS   = -licui18n -licuuc `pkg-config fuse --libs`

BUILD_PATH = tmp
BIN_PATH   = bin
SRC_PATH   = src

include pfs.mk
include praid.mk
include ppd.mk
include consolappd.mk

all: pfs praid ppd consolappd

##############################
#Reglas para el Proceso File System
##############################

pfs: $(PFS_BIN)

PFS_OBJS = $(addprefix $(BUILD_PATH)/, $(patsubst %.c, %.o, $(notdir $(PFS_SRC) ) ) )

$(PFS_BIN): $(PFS_OBJS) $(PFS_INCLUDES)
	@echo 'LINKEANDO $@'
	@gcc $(PFS_OBJS) -o $@ $(LIBFLAGS)

##############################
#Reglas para el Proceso RAID 1
##############################

praid: $(PRAID_BIN)

PRAID_OBJS = $(addprefix $(BUILD_PATH)/, $(patsubst %.c, %.o, $(notdir $(PRAID_SRC) ) ) )

$(PRAID_BIN): $(PRAID_OBJS) $(PRAID_INCLUDES)
	@echo 'LINKEANDO $@'
	@gcc $(LIBFLAGS) $(PRAID_OBJS) -o $@


##############################
#Reglas para el Proceso Planificador de Disco
##############################

ppd: $(PPD_BIN)

PPD_OBJS = $(addprefix $(BUILD_PATH)/, $(patsubst %.c, %.o, $(notdir $(PPD_SRC) ) ) )

$(PPD_BIN): $(PPD_OBJS) $(PPD_INCLUDES)
	@echo 'LINKEANDO $@'
	@gcc $(LIBFLAGS) $(PPD_OBJS) -o $@

##############################
#Reglas para la consola
##############################

consolappd: $(CONSOLAPPD_BIN)

CONSOLAPPD_OBJS = $(addprefix $(BUILD_PATH)/, $(patsubst %.c, %.o, $(notdir $(CONSOLAPPD_SRC) ) ) )

$(CONSOLAPPD_BIN): $(CONSOLAPPD_OBJS) $(CONSOLAPPD_INCLUDES)
	@echo 'LINKEANDO $@'
	@gcc $(LIBFLAGS) $(CONSOLAPPD_OBJS) -o $@


##############################
#Reglas de compilacion
##############################

$(BUILD_PATH)/%.o: $(SRC_PATH)/common/%.c $(SRC_PATH)/common/%.h
	@echo 'COMPILANDO $< -> $@'
	@gcc $(FLAGS) $< -o $@ -c

$(BUILD_PATH)/%.o: $(SRC_PATH)/pfs/%.c $(PFS_INCLUDES)
	@echo 'COMPILANDO $< -> $@'
	@gcc $(FLAGS) $< -o $@ -c

$(BUILD_PATH)/%.o: $(SRC_PATH)/praid1/%.c $(PRAID_INCLUDES)
	@echo 'COMPILANDO $< -> $@'
	@gcc $(FLAGS) $< -o $@ -c

$(BUILD_PATH)/%.o: $(SRC_PATH)/ppd/%.c $(PPD_INCLUDES)
	@echo 'COMPILANDO $< -> $@'
	@gcc $(FLAGS) $< -o $@ -c

$(BUILD_PATH)/%.o: $(SRC_PATH)/consolappd/%.c $(CONSOLAPPD_INCLUDES)
	@echo 'COMPILANDO $< -> $@'
	@gcc $(FLAGS) $< -o $@ -c


clean:
	@echo 'Limpiando todo'
	@rm $(BUILD_PATH)/* $(PFS_BIN) $(PRAID_BIN) 2> /dev/null || true

.PHONY: pfs praid ppd consolappd clean all

