FLAGS      = -Wall -Isrc/common/
BUILD_PATH = tmp
BIN_PATH   = bin
SRC_PATH   = src

all: $(BIN_PATH)/process_file_system

$(BUILD_PATH)/nipc.o: $(SRC_PATH)/common/nipc.c $(SRC_PATH)/common/nipc.h
	gcc $(FLAGS) `pkg-config fuse --cflags` $(SRC_PATH)/common/nipc.c -o $(BUILD_PATH)/nipc.o -c

$(BUILD_PATH)/direccionamiento.o: $(SRC_PATH)/pfs/direccionamiento.c $(SRC_PATH)/pfs/direccionamiento.h $(SRC_PATH)/common/nipc.h
	gcc $(FLAGS) `pkg-config fuse --cflags` $(SRC_PATH)/pfs/direccionamiento.c -o $(BUILD_PATH)/direccionamiento.o -c

$(BUILD_PATH)/utils.o: $(SRC_PATH)/pfs/utils.c $(SRC_PATH)/pfs/utils.h $(SRC_PATH)/common/nipc.h
	gcc $(FLAGS) `pkg-config fuse --cflags` $(SRC_PATH)/pfs/utils.c -o $(BUILD_PATH)/utils.o -c

$(BUILD_PATH)/pfs.o: $(SRC_PATH)/pfs/pfs.c $(SRC_PATH)/pfs/direccionamiento.h $(SRC_PATH)/common/nipc.h
	gcc $(FLAGS) `pkg-config fuse --cflags` $(SRC_PATH)/pfs/pfs.c -o $(BUILD_PATH)/pfs.o -c

$(BIN_PATH)/process_file_system: $(BUILD_PATH)/pfs.o $(BUILD_PATH)/nipc.o $(BUILD_PATH)/utils.o $(BUILD_PATH)/direccionamiento.o
	gcc $(FLAGS) `pkg-config fuse --libs` $(BUILD_PATH)/pfs.o $(BUILD_PATH)/nipc.o $(BUILD_PATH)/utils.o $(BUILD_PATH)/direccionamiento.o -o $(BIN_PATH)/process_file_system

clean:
	rm $(BUILD_PATH)/* $(BIN_PATH)/process_file_system

.PHONY: process_file_system clean all

