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

int atenderComando(void);

void conectarPpd(void);

void funcInfo(void);

void funcClean(void);

void funcTrace(void);

int errParamClean(void);

int errParamTrace(void);

#endif
