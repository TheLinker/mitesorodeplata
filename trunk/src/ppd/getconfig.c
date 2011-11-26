
#include "getconfig.h"
#define TAM_LINE 1024

config_t getconfig(char * archivoConfig)
{
	config_t vecConfig;
	size_t intDif;
	char * var, * ptr;
	char readLine[TAM_LINE]; //uso del tamline

	initValConfig(&vecConfig);

	if (NULL == (archConf = fopen(archivoConfig, "r")))
	{
		printf("No existe archivo config.txt\n");
	}
	else
	{
		while (!feof(archConf))
		{
			fgets(readLine, TAM_LINE, archConf);
			var = strchr(readLine, '=');
			if (!(var == NULL))
			{
				intDif = var - readLine;

				ptr = readLine + strlen(readLine);
				while (--ptr >= readLine && (*ptr =='\n' || *ptr=='\r'));
						ptr++;
				        *ptr = '\0';

				if (strncmp("modoinit",readLine,intDif) == 0)
					strcpy(vecConfig.modoinit, var+1);

				if (strncmp("ipppd",readLine,intDif) == 0)
					strcpy(vecConfig.ipppd , var+1);

				if (strncmp("ippraid",readLine,intDif) == 0)
					strcpy(vecConfig.ippraid , var+1);

				if (strncmp("ippfs",readLine,intDif) == 0)
					strcpy(vecConfig.ippfs , var+1);

				if (strncmp("puertoppd",readLine,intDif) == 0)
					vecConfig.puertoppd = atoi(var+1);

				if (strncmp("puertopraid",readLine,intDif) == 0)
					vecConfig.puertopraid = atoi(var+1);

				if (strncmp("puertopfs",readLine,intDif) == 0)
					vecConfig.puertopfs = atoi(var+1);

				if (strncmp("algplan",readLine,intDif) == 0)
					strcpy(vecConfig.algplan , var+1);

				if (strncmp("puertoconso",readLine,intDif) == 0)
					vecConfig.puertoconso = atoi(var+1);

				if (strncmp("flaglog",readLine,intDif) == 0)
					vecConfig.flaglog = atoi(var+1);

				if (strncmp("rutadisco",readLine,intDif) == 0)
					strcpy(vecConfig.rutadisco , var+1);

				if (strncmp("chs",readLine,intDif) == 0)
					strcpy(vecConfig.chs , var+1);

				if (strncmp("pistas",readLine,intDif) == 0)
					vecConfig.pistas = atoi(var+1);

				if (strncmp("sectores",readLine,intDif) == 0)
					vecConfig.sectores = atoi(var+1);

				if (strncmp("tiempolec",readLine,intDif) == 0)
					vecConfig.tiempolec = atoi(var+1);

				if (strncmp("posactual",readLine,intDif) == 0)
					vecConfig.posactual = atoi(var+1);

				if (strncmp("tiempoesc",readLine,intDif) == 0)
					vecConfig.tiempoesc = atoi(var+1);

				if (strncmp("rpm",readLine,intDif) == 0)
					vecConfig.rpm = atoi(var+1);

				if (strncmp("tiempoentrepistas",readLine,intDif) == 0)
					vecConfig.tiempoentrepistas = atoi(var+1);
			}
		}

	}

	return vecConfig;
}

 void initValConfig(config_t * vecConfig)
{
		vecConfig->modoinit[0] = '\0';
		vecConfig->ipppd[0] = '\0';
		vecConfig->ippraid[0] = '\0';
		vecConfig->ippfs[0] = '\0';
		vecConfig->puertoppd = 0;
		vecConfig->puertopraid = 0;
		vecConfig->puertopfs = 0;
		vecConfig->algplan[0] = '\0';
		vecConfig->puertoconso = 0;
		vecConfig->flaglog = -1;
	 	vecConfig->rutadisco[0] = '\0';
	 	vecConfig->chs[0] = '\0';
	 	vecConfig->pistas = -1;
	 	vecConfig->cabezas = -1;
	 	vecConfig->sectores = -1;
	 	vecConfig->posactual = 0;
	 	vecConfig->tiempolec = -1;
	 	vecConfig->tiempoesc = -1;
	 	vecConfig->rpm = -1;
	 	vecConfig->tiempoentrepistas = -1;
}
