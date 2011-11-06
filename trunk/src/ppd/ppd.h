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

#include <arpa/inet.h>

#define TAM_SECT 512
#define TAM_PAG 4096
#define PROTOCOLO 0  

char comando[30], buffer[TAM_SECT], bufferent[TAM_SECT];
char * pathArch;
int8_t * dirMap, * dirSect;
int sect, cantSect, offset;


void escucharPedidos(void);

void atenderPedido(void);

void leerSect(int sect);

void escribirSect(int, char [TAM_SECT]);

int abrirArchivoV(char * pathArch);

void * paginaMap (int sect, int dirArch);

void escucharConsola(void);

void atenderConsola(char comando[30]);

void funcInfo(void);

void funcClean(char *);

void funcTrace(char *);

int my_itoa(int val, char* buf);

void errorparam(void);

void msjprueba(nipc_packet *);

void conectarConPraid();

#endif /* PPD_H_ */
