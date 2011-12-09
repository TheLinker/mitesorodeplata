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

void leerSect(int sect, nipc_socket sock);

void escribirSect(int, char [TAM_SECT], nipc_socket sock);

int abrirArchivoV(char * pathArch);

void * discoMap (int sectores, int dirArch);

void escucharConsola(void);

void atenderConsola(char comando[100], int cliente);

void funcInfo(int cliente);

void funcClean(char *, int cliente);

void funcTrace(char *, int cliente);

void traceSect(int sect, int32_t nextsect, int cliente, int32_t);

int my_itoa(int val, char* buf);

void errorparam(void);

void conectarConPraid();

int calcularSector(char structSect[25]);

#endif /* PPD_H_ */
