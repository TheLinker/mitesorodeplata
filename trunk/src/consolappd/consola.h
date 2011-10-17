#ifndef CONSOLA_H_
#define CONSOLA_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "consola.h"

#define TAM_COMANDO 100

int atenderComando(void);
void funcInfo(void);
void conectarPpd(void);
void funcClean(void);
void funcTrace(void);

#endif
