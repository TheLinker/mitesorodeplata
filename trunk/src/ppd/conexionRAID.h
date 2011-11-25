#ifndef CONEXION_RAID_H_
#define CONEXION_RAID_H_

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "getconfig.h"
#include <netdb.h>

void conectarConRAID(config_t);

#endif /* CONEXION_RAID_H_ */