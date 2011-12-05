/*
 ============================================================================
 Name        : write_file.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#define BUFFER_SIZE 1024


int main(int argc, char **argv) {

	if( argc != 4 ){
		puts("- Invalid number of parameters");
		return EXIT_FAILURE;
	}

	char write_buffer[BUFFER_SIZE];
	char character = argv[1][0];

	memset(write_buffer, character, BUFFER_SIZE);

	FILE *file = fopen(argv[2], "r+");
	if( file == NULL ){
		puts("- Creating file ...");
		file = fopen(argv[2], "wb");
	}

	int file_size = atoi(argv[3]);

	while( file_size - BUFFER_SIZE >= 0 ){
		fwrite_unlocked(write_buffer, 1, BUFFER_SIZE, file);
		file_size = file_size - BUFFER_SIZE;
	}

	if( file_size > 0 ){

		fwrite_unlocked(write_buffer, 1, file_size, file);

	}

	fflush_unlocked(file);
	fclose(file);

	puts("Finished");

	return EXIT_SUCCESS;
}
