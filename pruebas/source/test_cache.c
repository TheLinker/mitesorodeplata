/*
 ============================================================================
 Name        : test_cache.c
 Author      : fviale
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define BUFFER_SIZE 1024

int main(int argc, char **argv) {


	if( argc != 3 ){
		puts("- Invalid number of parameters");
		return EXIT_FAILURE;
	}


	FILE *file = fopen(argv[1], "r+");
	setvbuf (file , NULL , _IONBF , 0 );

	// size in KB
	int cache_size = atoi(argv[2]);

	char buffer[BUFFER_SIZE];

	puts("[[ PRUEBAS DE LECTURA ]]\n\n");

	puts("Leyendo los primeros 1024 bytes ( Se accede a disco ) ...\n");
	fread(buffer, BUFFER_SIZE, 1, file);

	sleep(3);

	fseek(file, 0, SEEK_SET);

	puts("Leyendo nuevamente los primeros 1024 bytes ( Se lee de la cache ) ...\n");
	fread(buffer, BUFFER_SIZE, 1, file);

	sleep(6);

	int cont;

	puts("Llenando la cache ...");
	for(cont=1; cont<cache_size; cont ++){
		puts("- Leyendo 1Kb");
		fread(buffer, BUFFER_SIZE, 1, file);
		sleep(1);
	}

	puts("");

	sleep(6);

	puts("La cache no tiene espacio, la siguiente lectura reemplaza un elemento ...\n");
	fread(buffer, BUFFER_SIZE, 1, file);

	sleep(6);
	fseek(file, 0, SEEK_SET);

	puts("Leyendo todo nuevamente, todo debe esta cacheado salvo un elemento ...\n");
	for(cont=0; cont<cache_size; cont ++){
		puts("- Leyendo 1Kb");
		fread(buffer, BUFFER_SIZE, 1, file);
		sleep(1);
	}

	puts("\n\n[[ PRUEBAS DE ESCRITURA ]]\n\n");

	fseek(file, 0, SEEK_SET);
	memset(buffer, 'A', BUFFER_SIZE);

	puts("Generando escrituras ( todos los pedidos deben ser cacheados ) ...\n");
	for(cont=0; cont<cache_size; cont ++){
		puts("- Escribiendo 1Kb");
		fwrite(buffer, BUFFER_SIZE, 1, file);
		sleep(1);
	}

	sleep(5);
	puts("Vaciando toda la cache ... \n");
	fflush(file);

	fseek(file, 0, SEEK_SET);

	puts("Validando todo lo escrito ...\n");
	for(cont=0; cont<cache_size; cont ++){
		char tmp_buffer[BUFFER_SIZE];
		puts("- Leyendo y comparando");
		fread(tmp_buffer, BUFFER_SIZE, 1, file);

		if( memcmp(buffer, tmp_buffer, BUFFER_SIZE) != 0 ){
			puts("[ERROR] Bloque mal persistido");
			exit(0);
		}
		sleep(1);
	}

	memset(buffer, 'B', BUFFER_SIZE);
	fseek(file, 0, SEEK_SET);

	puts("Generando escrituras ( todos los pedidos deben ser cacheados ) ...\n");
	for(cont=0; cont<cache_size; cont ++){
		puts("- Escribiendo 1Kb");
		fwrite(buffer, BUFFER_SIZE, 1, file);
		sleep(1);
	}

	sleep(3);

	puts("Generando lecturas para forzar la limpieza de la cache ...\n");
	for(cont=0; cont<cache_size; cont ++){
		char tmp_buffer[BUFFER_SIZE];
		puts("- Leyendo 1Kb");
		fread(tmp_buffer, BUFFER_SIZE, 1, file);
		sleep(1);
	}

	sleep(3);
	fseek(file, 0, SEEK_SET);

	puts("Validando todo lo escrito ...\n");
	for(cont=0; cont<cache_size; cont ++){
		char tmp_buffer[BUFFER_SIZE];
		puts("- Leyendo y comparando");
		fread(tmp_buffer, BUFFER_SIZE, 1, file);

		if( memcmp(buffer, tmp_buffer, BUFFER_SIZE) != 0 ){
			puts("[ERROR] Bloque mal persistido");
			exit(0);
		}
		sleep(1);
	}

	fclose(file);

	puts("Finished");

	return EXIT_SUCCESS;
}
