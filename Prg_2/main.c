/*
 * Subject Realtime Operating Systems 48450
 * Project: Assignment 2 - Prg_2
 * Name: Phong Au
 * Student Number: 10692820
 * Compile Command: gcc -o Prg_2 Prg_2.c -lrt
 * Run Command : ./Prg_2
 */

 /* Includes */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <fcntl.h>


/* Defines */
#define SHM_MESSLENGTH 1024
#define SHM_NAME "shared"

int readFromSharedMemory(char *dataPtr, char sharedMemoryName[])
{
	/* Declare variables to be used */
	int shm_fd;
	char *addr;

	/* Create shared memory region */
	if ((shm_fd = shm_open(sharedMemoryName, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR))< 0)
		printf("Could not create shared memory region\n");
	else
	{

		/* Make the shared memory region */
    	if (ftruncate(shm_fd, (size_t)SHM_MESSLENGTH) < 0)
      		printf("Could not create shared memory size\n");
		else
		{

			/* Map the shared memory */
			addr = (char*)mmap(NULL, (size_t)SHM_MESSLENGTH, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
			if (addr == MAP_FAILED)
				printf("Could not create shared memory map\n");
			else
			{

				/* Copy shared memory data to dataPtr */
                sprintf(dataPtr, "%s", addr);

				/* Remove mappings */
				if(munmap((void *)addr, (size_t)SHM_MESSLENGTH) < 0)
				{
					printf("Could not remove mappings\n");
				}
				return 1;
			}
		}
	}
	return 0;
}
/*
 * Function main - Reads from shared memory and writes to console.
 */
int main(int argc, char *argv[])
{
    char data[SHM_MESSLENGTH];
    if(readFromSharedMemory(&data[0], SHM_NAME))
        printf("%s\n",data);
    else
        printf("Failed to read from shared memory.\n");
    return 0;
}
