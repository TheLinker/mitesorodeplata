/*
 ============================================================================
 Name        : test_cache.c
 Author      : fviale
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>

#define BUFFER_SIZE 1024

struct timeval timer;
double startTime, currentTime;

void start()
{
    gettimeofday(&timer, NULL);
    startTime = timer.tv_sec+(timer.tv_usec/1000000.0);
}

double getTime()
{
    gettimeofday(&timer, NULL);
    currentTime = timer.tv_sec+(timer.tv_usec/1000000.0);
    return (currentTime-startTime)*1000.0;
}

int main(int argc, char **argv) {

	if( argc != 3 ){
		printf("- Invalid number of parameters\n");
		return EXIT_FAILURE;
	}

	FILE *file = fopen(argv[1], "r+");

	// size in KB
	int cache_size = atoi(argv[2]);

	char buffer[BUFFER_SIZE];

	printf("[[ PRUEBAS DE LECTURA ]]\n\n");

	printf("Leyendo los primeros 1024 bytes ( Se accede a disco ) ...\n");
    start();
	fread(buffer, BUFFER_SIZE, 1, file);
    printf("leido en: %f ms\n\n", getTime());

//	sleep(3);

	fseek(file, 0, SEEK_SET);

	printf("Leyendo nuevamente los primeros 1024 bytes ( Se lee de la cache ) ...\n");
    start();
	fread(buffer, BUFFER_SIZE, 1, file);
    printf("leido en: %f ms\n\n", getTime());

//	sleep(6);

	int cont;

	printf("Llenando la cache ...\n");
	for(cont=1; cont<cache_size; cont ++){
		printf("- Leyendo 1Kb ");
        start();
		fread(buffer, BUFFER_SIZE, 1, file);
        printf("leido en: %f ms\n", getTime());
		//sleep(1);
	}

	printf("\n");

//	sleep(6);

	printf("La cache no tiene espacio, la siguiente lectura reemplaza un elemento ...\n");
    start();
	fread(buffer, BUFFER_SIZE, 1, file);
    printf("leido en: %f ms\n\n", getTime());

//	sleep(6);
	fseek(file, 0, SEEK_SET);

	printf("Leyendo todo nuevamente, todo debe esta cacheado salvo un elemento ...\n");
	for(cont=0; cont<cache_size; cont ++){
		printf("- Leyendo 1Kb ");
        start();
		fread(buffer, BUFFER_SIZE, 1, file);
        printf("leido en: %f ms\n", getTime());
//		sleep(1);
	}

	printf("\n\n[[ PRUEBAS DE ESCRITURA ]]\n\n");
//    fgetc(stdin);

	fseek(file, 0, SEEK_SET);
	memset(buffer, 'A', BUFFER_SIZE);

	printf("Generando escrituras ( todos los pedidos deben ser cacheados ) ...\n");
	for(cont=0; cont<cache_size; cont ++){
		printf("- Escribiendo 1Kb ");
        start();
		fwrite(buffer, BUFFER_SIZE, 1, file);
        printf("escrito en: %f ms\n", getTime());
//		sleep(1);
	}
	printf("\n");

//	sleep(5);
//    fgetc(stdin);
	printf("Vaciando toda la cache ... ");
    start();
    int ret;
	if ((ret = fflush(file))!=0)
        printf("Error %d %s", errno, strerror(errno));
    printf("flusheado en: %f ms\n", getTime());
	printf("\n");
//    fgetc(stdin);

	fseek(file, 0, SEEK_SET);

	printf("Validando todo lo escrito ...\n");
	for(cont=0; cont<cache_size; cont ++){
		char tmp_buffer[BUFFER_SIZE];
		printf("- Leyendo y comparando\n");
		fread(tmp_buffer, BUFFER_SIZE, 1, file);

		if( memcmp(buffer, tmp_buffer, BUFFER_SIZE) != 0 ){
			printf("[ERROR] Bloque mal persistido\n");
			exit(0);
		}
//		sleep(1);
	}

	printf("\n");
//    fgetc(stdin);
	memset(buffer, 'B', BUFFER_SIZE);
	fseek(file, 0, SEEK_SET);

	printf("Generando escrituras ( todos los pedidos deben ser cacheados ) ...\n");
	for(cont=0; cont<cache_size; cont ++){
		puts("- Escribiendo 1Kb");
        ftell(file);
        start();
		fwrite(buffer, BUFFER_SIZE, 1, file);
        printf("escrito en: %f ms\n", getTime());
		//sleep(1);
		/*printf("- Escribiendo 1Kb bloque:%d ", ftell(file)/BUFFER_SIZE);
		fwrite(buffer, BUFFER_SIZE, 1, file);
		sleep(1);*/
	}

	printf("--\n");
//    fgetc(stdin);
//	sleep(3);

	printf("Generando lecturas para forzar la limpieza de la cache ...\n");
	for(cont=0; cont<cache_size; cont ++){
		char tmp_buffer[BUFFER_SIZE];
		printf("- Leyendo 1Kb ");
        start();
		fread(tmp_buffer, BUFFER_SIZE, 1, file);
        printf("leido en: %f ms\n", getTime());
//		sleep(1);
	}

	printf("\n");
//    fgetc(stdin);
//	sleep(3);
	fseek(file, 0, SEEK_SET);

	printf("Validando todo lo escrito ...\n");
	for(cont=0; cont<cache_size; cont ++){
		char tmp_buffer[BUFFER_SIZE];
		printf("- Leyendo y comparando\n");
		fread(tmp_buffer, BUFFER_SIZE, 1, file);

		if( memcmp(buffer, tmp_buffer, BUFFER_SIZE) != 0 ){
			printf("[ERROR] Bloque mal persistido\n");
			exit(0);
		}
//		sleep(1);
	}

	printf("\n");
//    fgetc(stdin);
	fclose(file);

	puts("Finished");

	return EXIT_SUCCESS;
}
