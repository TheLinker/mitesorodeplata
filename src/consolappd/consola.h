#ifndef CONSOLA_H_
#define CONSOLA_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
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
