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
#include "consola.h"

#define TAM_COMANDO 100

int atenderComando(int);

void conectarPpd(void);

void funcInfo(char*);

void funcClean(char*);

void funcTrace(char*);

int errParamClean(void);

int errParamTrace(void);

#endif
