#ifndef CONSOLA_H_
#define CONSOLA_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#define TAM_COMANDO 100

int32_t atenderComando(int);

void conectarPpd(void);

void funcInfo(char*);

void funcClean(char*);

void funcTrace(char*);

int32_t errParamClean(void);

int32_t errParamTrace(void);

int32_t obtenerRecCant(int32_t psect,int32_t ssect,int32_t pposactual,int32_t sposactual);

#endif
