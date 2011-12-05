/*
 ============================================================================
 Name        : make_starvation.c
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

#define FILENAME_HEAD "concurrent_file"
#define FILENAME_EXT ".txt"
#define MAX_FILENAME sizeof(FILENAME_HEAD) + 5 + sizeof(FILENAME_EXT)
#define BUFFER_SIZE 1024


void read_file(void *arg){
	char read_buffer[BUFFER_SIZE];
	char *path = (char*)arg;

	FILE *file = fopen(path, "rb");
	printf("- Opening %s ...\n", path);

	while(1){
		if( fread_unlocked(read_buffer, 1, BUFFER_SIZE, file) < BUFFER_SIZE ) {
			fseek(file, 0, SEEK_SET);
		}
	}

	fclose(file);
}


int main(int argc, char **argv) {

	if( argc != 3 ){
		puts("- Invalid number of parameters");
		return EXIT_FAILURE;
	}

	int amount_of_threads = atoi(argv[1]);

	pthread_t threads[amount_of_threads];

	int index;

	for (index = 0; index < amount_of_threads; index++) {
		pthread_create(&threads[index], NULL, (void*) read_file, (void*)argv[2]);
	}

	for (index = 0; index < amount_of_threads; index++) {
		pthread_join(threads[index], NULL);
	}

	return EXIT_SUCCESS;
}
