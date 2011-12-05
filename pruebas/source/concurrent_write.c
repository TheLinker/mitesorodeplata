/*
 ============================================================================
 Name        : concurrent_write.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#define _BSD_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#define FILENAME_HEAD "th_"
#define FILENAME_EXT ".txt"
#define MAX_FILENAME sizeof(FILENAME_HEAD) + 5 + sizeof(FILENAME_EXT)
#define BUFFER_SIZE 1024

void write_file(void *args){
	int *aux_args =  (int*)args;
	char write_buffer[BUFFER_SIZE];
	char filename[ MAX_FILENAME ];
	int thread_num = aux_args[0];
	int file_size = aux_args[2];
	char character = aux_args[1];

	memset(write_buffer, character, BUFFER_SIZE);

	sprintf(filename,"%s%d%s",FILENAME_HEAD, thread_num, FILENAME_EXT);

	FILE *file = fopen(filename, "r+");
	if( file == NULL ){
		printf("- Creating %s ...\n", filename);
		file = fopen(filename, "wb");
	}

	while( file_size - BUFFER_SIZE >= 0 ){
		fwrite_unlocked(write_buffer, 1, BUFFER_SIZE, file);
		file_size = file_size - BUFFER_SIZE;
	}

	if( file_size > 0 ){

		fwrite_unlocked(write_buffer, 1, file_size, file);

	}

	fflush_unlocked(file);
	fclose(file);

	printf("Finished %s\n", filename);
	free(args);
}


int main(int argc, char **argv) {

	if( argc != 4 ){
		puts("- Invalid number of parameters");
		return EXIT_FAILURE;
	}

	int amount_of_threads = atoi(argv[1]);

	if( strlen(argv[2]) != amount_of_threads ){
		puts("- Invalid parameter");
	}

	int file_size = atoi(argv[3]);

	pthread_t threads[amount_of_threads];

	int index;

	for (index = 0; index < amount_of_threads; index++) {
		int *args = malloc( sizeof(int) * 3 );
		args[0] = index;
		args[1] = (int)argv[2][index];
		args[2] = file_size;
		pthread_create(&threads[index], NULL, (void*) write_file, (void*)args);
	}

	for (index = 0; index < amount_of_threads; index++) {
		pthread_join(threads[index], NULL);
	}

	return EXIT_SUCCESS;
}
