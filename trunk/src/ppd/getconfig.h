
#ifndef GETCONFIG_H_
#define GETCONFIG_H_

#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>

FILE * archConf;

typedef struct
{
	char			modoinit[8];
	char			ipppd[16];
	char			ippraid[16];
	char			ippfs [16];
	unsigned short	puertoppd;
	unsigned short	puertopraid;
	unsigned short	puertopfs;
	char			algplan[10];
	unsigned short 	puertoconso;
	char			flaglog;
	char			rutadisco[100];
	char			chs[20];
	int32_t			pistas;
	int32_t			cabezas;
	int32_t			sectores;
	int32_t			posactual;
	int32_t			tiempolec;
	int32_t			tiempoesc;
	int32_t			rpm;
	int32_t			tiempoentrepistas;
	char			rutalog[100];



} __attribute__ ((packed))	config_t;

//HACER ESTRUCTURA DE DATOS DEL CONFIG, config_t

config_t getconfig (char * archConfig);
void initValConfig(config_t * vecConfig);

#endif /* GETCONFIG_H_ */
