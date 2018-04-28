/*
 * Subject Realtime Operating Systems 48450
 * Project: Assignment 2 - Prg_2
 * Name: Phong Au
 * Student Number: 10692820
 * Compile Command: gcc -o Prg_2 Prg_2.c -lpthread -lrt -Wall
 * Run Command : ./Prg_2
 */

 /* Includes */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h> /* For mode constants */
#include <fcntl.h> /* For O_* constants */

/* Defines */
#define MESSLENGTH 1024

/*
 * Function main
 * <summary>Reads from shared memory and writes to console.</summary>
 * <params></params>
 * <returns></returns>
 */
int main(int argc, char *argv[])
{
    /* Process args */
    int debug = 0;
	if(argc > 1)
	{
        if (strcmp(argv[1],"-debug") == 0)
            debug = 1;
        else
        {
            printf("Commmands\n[-debug] Outputs process to console.\n");
            exit(EXIT_FAILURE);
        }
	}

	/* Declare variables to be used */
	if(debug)
        printf("Declaring Variables\n");

    int shm_fd; // Shared memory file descriptor
    char *addr;  // Shaared memory address

	/* Create shared memory region */
	if(debug)
        printf("Creating shared memory region\n");

    if ((shm_fd = shm_open("shared", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR))< 0)
        printf("Could not create shared memory region\n");
    else
    {
		if (ftruncate(shm_fd, (size_t)1024) < 0) 
      		printf("Could not create shared memory size\n");
		else
		{
			/* Map the shared memory */
			if(debug)
				printf("Mapping the shared memory region\n");

			addr = (char*)mmap(NULL, (size_t)1024, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
			if (addr == MAP_FAILED)
				printf("Could not create shared memory map\n");
			else
			{
				/* Print data from shared memory to Console */
				if(debug)
					printf("Printing Duration\n");

				printf("Duration: %s Microseconds\n", addr);
				
				/* Remove mappings */
				if(munmap((void *)addr, (size_t)1024) < 0)
				{
					perror("munmap");
				}
				close(shm_fd);
				return 1;
			}
		}
    }
    return 0;
}
