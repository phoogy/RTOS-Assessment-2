  /*
 * Subject Realtime Operating Systems 48450
 * Project: Assignment 2 - Prg_1
 * Name: Phong Au
 * Student Number: 10692820
 * Compile Command: gcc -o Prg_1 Prg_1.c -lpthread -lrt -Wall
 * Run Command : ./Prg_1
 */

 /* Includes */
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

/* Defines */
#define MESSLENGTH 127

/* ThreadData Struct */
struct ThreadData
{
	char *collectFromPipe;					// Place to pass string from thread 2 to thread 3
	int eof;								// End Of File Flag
	int fd[2];								// The File Descriptor for the pipe
	pthread_mutex_t eof_mutex, cfp_mutex;	// The mutex locks
	sem_t write, read, justify;				// The semaphores
	int debug;
};

/* Threads & Functions */
void *threadA(void *args);			        // Thread one
void *threadB(void *args);					// Thread two
void *threadC(void *args);					// Thread three
void initialiseData(void *args);			// Function to initialise data
void freeResources(void *args);				// Function to initialise data
int eof(struct ThreadData *threadData);		// Get shared EOF flag in threadData
int writeToSharedMemory(char sharedMemoryName[], char *data); // Get shared EOF flag in threadData
int printSrc(void);							// Print src.txt to console

/*
 * Function main
 * <summary>The main program.</summary>
 * <params></params>
 * <returns></returns>
 */
int main(int argc, char *argv[])
{
	/* Clock/Timer */
	int startTime = clock() * 1000000 / CLOCKS_PER_SEC; // Start Clock Time

    /* Declare threadData*/
	struct ThreadData threadData;

	/* Turn off debug */
    threadData.debug = 0;

	/* Process args */
	if(argc > 1)
	{
        if (strcmp(argv[1],"-debug") == 0)
            threadData.debug = 1;
        else
        {
            printf("Commmands\n[-debug] Outputs process to console.\n");
            exit(EXIT_FAILURE);
        }
	}

    /* Initialise threadData */
    initialiseData(&threadData);	// Call InitialiseData
    if(threadData.debug)
        printf("Initialised Data.\n");

	/* Thread Id Declarations */
	pthread_t tidA,tidB,tidC;	   // Thread IDs

	/* Thread Creation */
	if(pthread_create(&tidA, NULL, threadA, &threadData) != 0)
    {
        perror("pthread_create A");
        freeResources(&threadData);
    	exit(3);
    }
	if(pthread_create(&tidB, NULL, threadB, &threadData) != 0)
    {
        perror("pthread_create B");
        freeResources(&threadData);
    	exit(4);
    }
	if(pthread_create(&tidC, NULL, threadC, &threadData) != 0)
    {
        perror("pthread_create C");
        freeResources(&threadData);
    	exit(5);
    }
    if(threadData.debug)
        printf("Created threads.\n");

	/* NOTE Passing NULL to the thread atttribute will make pthread_create use
	 * the default attributes which is the same as if you were to use
	 * pthread_attr_init() for a pthread_attr_t object
	 */

	/* Thread Wait */
	if(threadData.debug)
        printf("Waiting for threads.\n");

	pthread_join(tidA,NULL);	// Wait for Thread one to finish
	pthread_join(tidB,NULL);	// Wait for Thread two to finish
	pthread_join(tidC,NULL);	// Wait for Thread three to finish

    if(threadData.debug)
        printf("Waiting for threads finished.\n");

	/* Free Resources */
	freeResources(&threadData);
	if(threadData.debug)
        printf("Resources freed.\n");

	/* Clock/Timer */
	char time[1024];	// Declare time variable with 1024 because thats the created size of the shared memory
	sprintf(time, "%d", (int)((clock() * 1000000 / CLOCKS_PER_SEC) - startTime));	// Calculate Time
    if(threadData.debug)
        printf("Time Calculated.\n");

	/* Shared memory */
	if(!writeToSharedMemory("shared",time))
		printf("Failed to wrote to shared memory\n");
    else if(threadData.debug)
        printf("Written time to shared memory.\n");

    /* Print time and src*/
    if(threadData.debug)
    {
        printf("Duration: %s Microseconds\n",time);	// Print Duration to terminal to compare with Prg2
        if(!printSrc())
            printf("Failed to print src file");
    }
    return 0;
}

/*
 * Function threadA
 * <summary>Reads data from the text file one line at a time and writes it to the pipe.</summary>
 * <params>void *args = reference to the threadData</params>
 * <returns></returns>
 */
void *threadA(void *args)
{
	/* Declare variables to be used*/
	struct ThreadData *threadData = args;
	FILE *dataFile;

	/* Open File */
	if((dataFile = fopen("data.txt", "r")))
	{
		/* Declare char array to store line from text file */
		char line[MESSLENGTH];

		/* Loop while not end of file */
		while(fgets(line,sizeof line, dataFile) != NULL)
		{
			sem_wait(&(threadData->write));					// Wait for semaphore
			write(threadData->fd[1], line, strlen(line));	// Write line to pipe
			sem_post(&(threadData->read));					// Signal threadB
		}

		/* Close dataFile */
		fclose(dataFile);

		/* Set eof to 1 to signal other threads that dataFile has finished */
		pthread_mutex_lock(&(threadData->eof_mutex));		// Mutex lock
		threadData->eof = 1;								// Set eof to 1
		pthread_mutex_unlock(&(threadData->eof_mutex));		// Mutex unlock
	}else
	{
		/* Error opening file setting eof to 2 so other threads don't wait for data */
		perror("data.txt");                             // Print error
		pthread_mutex_lock(&(threadData->eof_mutex));	// Mutex lock
		threadData->eof = 2;							// Set eof to 2
		pthread_mutex_unlock(&(threadData->eof_mutex));	// Mutex unlock
	}

	/* Signal threadB one last time so threadB can end */
	sem_post(&(threadData->read));
	return 0;
}

/*
 * Function threadB
 * <summary>Reads data from the pipe and passes it to threadC.</summary>
 * <params>void *args = reference to the threadData</params>
 * <returns></returns>
 */
void *threadB(void *args)
{
	/* Declare variables to be used*/
	struct ThreadData *threadData = args;
	int n;

	/* Loop while not end of file */
	while(eof(threadData) == 0)
	{
		/* Wait for signal*/
	  	sem_wait(&(threadData->read));

		/* If dataFile opening in threadA failed*/
		if(eof(threadData) == 2)
		{
			sem_post(&(threadData->justify));	// Signal threadC
			pthread_exit(NULL);					// Exit Thread
		}

		/* Read Data from pipe */
		pthread_mutex_lock(&(threadData->cfp_mutex));
		n = read(threadData->fd[0], threadData->collectFromPipe, MESSLENGTH);
		threadData->collectFromPipe[n] = '\0'; 			// Null Character Terminates string.
		pthread_mutex_unlock(&(threadData->cfp_mutex));

		/* If end of file */
		if(eof(threadData) == 1)
		{
			sem_post(&(threadData->justify));	// Signal threadC
			pthread_exit(NULL);					// Exit Thread
		}

		/* Signal threadC */
		sem_post(&(threadData->justify));
	}
	return 0;
}

/*
 * Function threadC
 * <summary>Reads Data from threadB, justifies whether it is header or content and stores/discards the data.</summary>
 * <params>void *args = reference to the threadData</params>
 * <returns></returns>
 */
void *threadC(void *args)
{
	/* Declare variables to be used*/
	struct ThreadData *threadData = args;
	FILE *src; // File Variable declaration
	int end_header = 0;

	/* Open/Create File to write to*/
	src = fopen("src.txt", "w");

	/* Loop while not end of file */
	while(eof(threadData) == 0)
	{
		/* Wait for signal*/
		sem_wait(&(threadData->justify));

		/* If dataFile opening in threadA failed*/
		if(eof(threadData) == 2)
		{
			fclose(src);			// Close src file
			pthread_exit(NULL);		// Exit Thread
		}

		/* Process Data from thread B */
		if(end_header)								// If end header has been found
			fputs(threadData->collectFromPipe,src);	// Write data to src file
		else if (strcmp(threadData->collectFromPipe,"end_header\n") == 0) // Look for end header
			end_header = 1;							// Set end header to found/true/1

		/* If end of file */
		if(eof(threadData) == 1)
		{
			fclose(src);			// Close src file
			pthread_exit(NULL);		// Exit Thread
		}

		/* Signal threadA */
	  	sem_post(&(threadData->write));
	}

	/* Close src file */
	fclose(src);
	return 0;
}

/*
 * Function initialiseData
 * <summary>Initialises resources, mutexes and sempahores.</summary>
 * <params>void *args = reference to the threadData</params>
 * <returns></returns>
 */
void initialiseData(void *args) {

	/* Declare variables to be used*/
	struct ThreadData *threadData = args;

	/* Allocate memory for collectFromPipe */
	threadData->collectFromPipe = malloc(MESSLENGTH*sizeof(int));
	if (threadData->collectFromPipe == NULL)
	{
        perror("collectFromPipe");
        exit(1);
	}

	/* Initialise mutexes */
	pthread_mutex_init(&(threadData->eof_mutex), NULL); // End Of File mutex.
	pthread_mutex_init(&(threadData->cfp_mutex), NULL); // Collect From File mutex.

	/* Set end of file flag to false/0 */
	pthread_mutex_lock(&(threadData->eof_mutex));
	threadData->eof = 0;
	pthread_mutex_unlock(&(threadData->eof_mutex));

	/* Initialise the semaphores */
	sem_init(&(threadData->write), 0, 0);   // Write/Thread A semaphore
	sem_init(&(threadData->read), 0, 0);	// Read/Thread B semaphore
	sem_init(&(threadData->justify), 0, 0); // Justify/Thread C semaphore
	sem_post(&(threadData->write));			// Add a token to write semaphore

	/* Create Pipe */
	if(pipe(threadData->fd)<0)
    {
        perror("fd");
        exit(2);
	}
}

/*
 * Function freeResources
 * <summary>Frees up allocated resources and destroys mutexes and sempahores.</summary>
 * <params>void *args = reference to the threadData</params>
 * <returns></returns>
 */
void freeResources(void *args)
{
	/* Declare variables to be used*/
	struct ThreadData *threadData = args;

	/* Free Resources */
	free(threadData->collectFromPipe);					// Free pipe
	pthread_mutex_destroy(&(threadData->eof_mutex));	// Destroy eof mutex
	pthread_mutex_destroy(&(threadData->cfp_mutex));	// Destroy cfp mutex
	sem_destroy(&(threadData->write));					// Destroy write semaphore
	sem_destroy(&(threadData->read));					// Destroy read semaphore
	sem_destroy(&(threadData->justify));				// Destroy justify semaphore
}

/*
 * Function eof
 * <summary>Allows safe access to the eof flag as this function implements mutex.</summary>
 * <params>struct ThreadData *threadData = reference to the threadData object</params>
 * <returns>int 0 = not end of file, 1 = end of file, 2 = error opening file, -1 = unknown error.</returns>
 */
int eof(struct ThreadData *threadData)
{
	/* Mutex Lock then Unlock */
	pthread_mutex_lock(&(threadData->eof_mutex));
	if(threadData->eof == 0)
	{
		pthread_mutex_unlock(&(threadData->eof_mutex));
		return 0;
	}else if (threadData->eof == 1)
	{
		pthread_mutex_unlock(&(threadData->eof_mutex));
		return 1;
	}else if(threadData->eof == 2)
	{
		pthread_mutex_unlock(&(threadData->eof_mutex));
		return 2;
	}else
	{
		pthread_mutex_unlock(&(threadData->eof_mutex));
		return -1;
	}
}

/*
 * Function writeToSharedMemory
 * <summary>Creates or opens shared memory and writes to it.</summary>
 * <params>char sharedMemoryName[] = the name of the shared memory, void *data = a reference to the data to be copied to shared memory</params>
 * <returns>int 1 = success, 0 = failed.</returns>
 */
int writeToSharedMemory(char sharedMemoryName[], char *data)
{
	/* Declare variables to be used */
	int shm_fd;		// Shared memory file descriptor
	char *addr;		// Shared memory address

	/* Create shared memory region */
	if ((shm_fd = shm_open(sharedMemoryName, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR))< 0)
		printf("Could not create shared memory region\n");
	else
	{
		/* Make the shared memory region */ 
    	if (ftruncate(shm_fd, (size_t)1024) < 0) 
      		printf("Could not create shared memory size\n");
		else
		{
			/* Map the shared memory */
			addr = (char*)mmap(NULL, (size_t)1024, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
			if (addr == MAP_FAILED)
				printf("Could not create shared memory map\n");
			else
			{
				/* Copy data to shared memory */
				memcpy(addr,data,sizeof(&data));
			
				/* Remove mappings */
				if(munmap((void *)addr, (size_t)1024) < 0)
				{
					perror("munmap");
				}
				//close(shm_fd);
				return 1;
			}			
		}
	}
	return 0;
}

/*
 * Function printSrc
 * <summary>Attempts to print the contents the src.txt file.</summary>
 * <params></params>
 * <returns>int 1 = success, 0 = failed.</return>
 */
int printSrc(void)
{
	/* Declare variables to be used */
	FILE *srctest;
	char line[MESSLENGTH];	// Char array to store line from text file

	/* Open File */
	if((srctest = fopen("src.txt", "r")))
	{
		/* Print src.txt */
		printf("\nsrc.txt\n");

		/* Write all lines from file to console */
		while(fgets(line,sizeof line, srctest) != NULL)	// While not end of file
			printf("%s",line);

		/* Close file */
		fclose(srctest);
		return 1;
	}
	return 0;
}
