#ifndef PPD_H_
#define PPD_H_

#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <unistd.h>
#include "getconfig.h"
#include "planDisco.h"
#include "conexionPFS.h"
#include <signal.h>   
#include <sys/types.h>   
#include <sys/socket.h>   
#include <sys/un.h> 
#include <pthread.h>
#include <errno.h>
#include <fcntl.h>
#include <semaphore.h>
#include <unistd.h>

#include <arpa/inet.h>

#define TAM_SECT 512
#define TAM_PAG 4096
#define PROTOCOLO 0  

char comando[200], buffer[TAM_SECT], bufferent[TAM_SECT];
char * pathArch;
int8_t * dirMap, * dirSect;
uint32_t sect, cantSect, offset;


void escucharPedidos(nipc_socket *sock);

void atenderPedido(void);

void leerSect(int32_t sect, nipc_socket sock);

void escribirSect(int, char [TAM_SECT], nipc_socket sock);

int32_t abrirArchivoV(char * pathArch);

void * discoMap (int32_t sectores, int32_t dirArch);

void escucharConsola(void);

void atenderConsola(char comando[100], int32_t cliente);

void funcInfo(int32_t cliente);

void funcClean(char *, int32_t cliente);

void funcTrace(char *, int32_t cliente);

void traceSect(int32_t sect, int32_t nextsect, int32_t cliente, int32_t posCab, int32_t cola[20]);

int32_t my_itoa(int32_t val, char* buf);

void errorparam(void);

void conectarConPraid();

int32_t calcularSector(char structSect[25]);

#endif /* PPD_H_ */
