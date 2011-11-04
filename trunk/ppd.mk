PPD_BIN = $(BIN_PATH)/ppd

PPD_SRC = $(SRC_PATH)/ppd/ppd.c \
          $(SRC_PATH)/ppd/getconfig.c \
          $(SRC_PATH)/ppd/conexionPFS.c \
          $(SRC_PATH)/ppd/planDisco.c \
          $(SRC_PATH)/common/nipc.c \
          $(SRC_PATH)/common/log.c \
          $(SRC_PATH)/common/utils.c

PPD_INCLUDES = $(SRC_PATH)/ppd/ppd.h \
               $(SRC_PATH)/ppd/getconfig.h \
               $(SRC_PATH)/ppd/conexionPFS.h \
               $(SRC_PATH)/ppd/planDisco.h \
               $(SRC_PATH)/common/nipc.h \
               $(SRC_PATH)/common/log.h \
               $(SRC_PATH)/common/utils.h

