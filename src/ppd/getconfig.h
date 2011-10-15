/*
 * Getconfig.h
 *
 *  Created on: Oct 7, 2011
 *      Author: lucas
 */

#ifndef GETCONFIG_H_
#define GETCONFIG_H_

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
	int				pistas;
	int				cabezas;
	int				sectores;
	int				tiempolec;
	int				tiempoesc;
	int				rpm;
	int				tiempoentrepistas;


} __attribute__ ((packed))	config_t;

//HACER ESTRUCTURA DE DATOS DEL CONFIG, config_t

config_t getconfig (char * archConfig);
void initValConfig(config_t * vecConfig);

#endif /* GETCONFIG_H_ */
