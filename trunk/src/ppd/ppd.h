#ifndef PPD_H_
#define PPD_H_

#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <unistd.h>
#include "getconfig.h"
#include "planDisco.h"
#include <signal.h>   
#include <sys/types.h>   
#include <sys/socket.h>   
#include <sys/un.h> 
#include "../common/nipc.h"

#define TAM_SECT 512
#define TAM_PAG 4096
#define PROTOCOLO 0  

char comando[30], buffer[TAM_SECT], bufferent[TAM_SECT];
char * pathArch;
void * dirMap, * dirSect;
int sect, cantSect, offset;

size_t len = 100;
FILE * dirArch;

void escucharPedidos(void);

void atenderPedido(void);

void leerSect(int sect, FILE * dirArch);

void escribirSect(char bufferent[TAM_SECT], int sect, FILE * dirArch);

FILE * abrirArchivoV(char * pathArch);

void * paginaMap (int sect, FILE * dirArch);

void escucharConsola(void);

void atenderConsola(char comando[30]);

void funcInfo(void);

void funcClean(char *);

void funcTrace(char *);

int my_itoa(int val, char* buf);

void errorparam(void);

void msjprueba(nipc_packet *);

#endif /* PPD_H_ */
