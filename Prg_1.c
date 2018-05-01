  /*
 * Subject Realtime Operating Systems 48450
 * Project: Assignment 2 - Prg_1
 * Name: Phong Au
 * Student Number: 10692820
 * Compile Command: gcc -o Prg_1 Prg_1.c -pthread -lrt
 * Run Command : ./Prg_1
 */

 /* Includes */
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <time.h>

/* Defines */
#define MESSLENGTH 1024
#define DATA_FILE "data.txt"
#define SRC_FILE "src.txt"
#define END_HEADER "end_header"
#define SHM_MESSLENGTH 1024
#define SHM_NAME "shared"

/* ThreadData Struct */
struct ThreadDataA
{
    sem_t *semWait;
    sem_t *semPost;
    int *eof;
    pthread_mutex_t *eofMutex;
	int *pipeWrite;
	FILE *dataFile;
};

struct ThreadDataB
{
    sem_t *semWait;
    sem_t *semPost;
    int *eof;
    pthread_mutex_t *eofMutex;
    pthread_mutex_t *sharedMutex;
    char *shared;
    int *pipeRead;
};

struct ThreadDataC
{
    sem_t *semWait;
    sem_t *semPost;
    int *eof;
    pthread_mutex_t *eofMutex;
    pthread_mutex_t *sharedMutex;
	char *shared;
    FILE *srcFile;
};

/* Function threadA - Reads data from the text file one line at a time and writes it to the pipe. */
void *threadA(struct ThreadDataA *data)
{

    /* Declare local variables */
    char line[MESSLENGTH];

    /* Loop*/
    while(1)
    {
        /* Wait and check eof */
        sem_wait(data->semWait);
        pthread_mutex_lock(data->eofMutex);
        if(*data->eof)
		{
            pthread_mutex_unlock(data->eofMutex);
            break;
		}
		pthread_mutex_unlock(data->eofMutex);

        /* Read line from file and break or write to pipe */
        if(fgets(line,sizeof line, data->dataFile) == NULL)
            break;
        write(*data->pipeWrite, line, strlen(line));

        /* post */
        sem_post(data->semPost);
    }
	return 0;
}

/* Function threadB - Reads data from the pipe and passes it to threadC. */
void *threadB(struct ThreadDataB *data)
{
    /* Declare local variables */
    int n;

	/* Loop*/
	while(1)
	{
        /* Wait and check eof */
	  	sem_wait(data->semWait);
		pthread_mutex_lock(data->eofMutex);
		if(*data->eof)
		{
            pthread_mutex_unlock(data->eofMutex);
            break;
		}
		pthread_mutex_unlock(data->eofMutex);

		/* Read Data from pipe, write to shared */
		pthread_mutex_lock(data->sharedMutex);
		n = read(*data->pipeRead, data->shared, MESSLENGTH);
		data->shared[n] = '\0'; 			// Null Character Terminates string.
		pthread_mutex_unlock(data->sharedMutex);

        /* Post */
		sem_post(data->semPost);
	}
	return 0;
}

/*
 * Function threadC - Reads Data from threadB, justifies whether it is header or content and stores/discards the data.
 */
void *threadC(struct ThreadDataC *data)
{
	/* Declare variables to be used*/
	int endHeaderFound = 0;

	/* Loop while not end of file */
    while(1)
    {
        /* Wait and check eof */
        sem_wait(data->semWait);
        pthread_mutex_lock(data->eofMutex);
        if(*data->eof)
        {
            pthread_mutex_unlock(data->eofMutex);
            break;
        }
        pthread_mutex_unlock(data->eofMutex);

        /* Read data from shared and write to file if end header found */
        pthread_mutex_lock(data->sharedMutex);
        if(endHeaderFound)
        {
            if(fputs(data->shared,data->srcFile) == EOF)
            {
                printf("Failed to write to file.\n");
                pthread_mutex_lock(data->eofMutex);
                *data->eof = 1;
                pthread_mutex_unlock(data->eofMutex);
            }
        }
        else if (strncmp(data->shared, END_HEADER, strlen(END_HEADER)) == 0)
            endHeaderFound = 1;
        pthread_mutex_unlock(data->sharedMutex);

         /* Post */
        sem_post(data->semPost);
    }
	return 0;
}

/*
 * Function writeToSharedMemory - Creates or opens shared memory and writes to it.
 */
int writeToSharedMemory(char sharedMemoryName[], char *data)
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

				/* Copy data to shared memory */
                sprintf(addr, "%s", data);

				/* Remove mappings */
				if(munmap((void *)addr, (size_t)SHM_MESSLENGTH) < 0)
				{
					printf("Could not remove mappings\n");
				}
				close(shm_fd);
				return 1;
			}
		}
		close(shm_fd);
	}

	return 0;
}

/* Main Function*/
int main(int argc, char *argv[])
{

	/* Clock/Timer */
    struct timespec start, end;
    clock_gettime(CLOCK_REALTIME, &start);

    /* Declare variables*/
    int fd[2], eof;
    char *shared;
    FILE *dataFile, *srcFile;
    sem_t write, read, justify;
    pthread_mutex_t eofMutex, sharedMutex;

    /* Create Pipe */
    if (pipe(fd) < 0)
        printf("Failed to create pipe.\n");
    else
    {
        /* Allocate memory for shared data betwen threadB and threadC*/
        shared = malloc(MESSLENGTH * sizeof(int));
        if (shared == NULL)
            printf("Failed to allocate memory for shared.\n");
        else
        {
            /* Initialse eofMutex */
            if (pthread_mutex_init(&eofMutex, NULL) != 0)
                printf("Failed to initialise eofMutex.\n");
            else
            {
                /* Initialse sharedMutex */
                if (pthread_mutex_init(&sharedMutex, NULL) != 0)
                    printf("Failed to initialise sharedMutex.\n");
                else
                {
                    /* Initialise semaphore write */
                    if (sem_init(&write, 0, 0) < 0)
                        printf("Failed to initialise semaphore write.\n");
                    else
                    {
                        /* Initialise semaphore read */
                        if (sem_init(&read, 0, 0) < 0)
                            printf("Failed to initialise semaphore read.\n");
                        else
                        {
                            /* Initialise semaphore justify */
                            if (sem_init(&justify, 0, 0) < 0)
                                printf("Failed to initialise semaphore justify.\n");
                            else
                            {
                                /* Open data file for reading */
                                dataFile = fopen(DATA_FILE, "r");

                                if (dataFile == NULL)
                                    printf("Failed to open %s for reading.\n", DATA_FILE);
                                else
                                {
                                    /* Open src file for writing */
                                    srcFile = fopen(SRC_FILE, "w");
                                    if (srcFile == NULL)
                                        printf("Failed to open %s for writing.\n", SRC_FILE);
                                    else
                                    {

                                        /* Declare threadData*/
                                        struct ThreadDataA dataA = {&write, &read, &eof, &eofMutex, &fd[1], dataFile};
                                        struct ThreadDataB dataB = {&read, &justify, &eof, &eofMutex, &sharedMutex, shared,  &fd[0]};
                                        struct ThreadDataC dataC = {&justify, &write, &eof, &eofMutex, &sharedMutex, shared, srcFile};

                                        /* Declare Thread Id */
                                        pthread_t tidA,tidB,tidC;

                                        eof = 1;

                                        /* Create ThreadA */
                                        if(pthread_create(&tidA, NULL, (void *)threadA, &dataA) != 0)
                                            printf("Failed to create thread A\n");
                                        else
                                        {
                                            /* Create ThreadB */
                                            if(pthread_create(&tidB, NULL, (void *)threadB, &dataB) != 0)
                                            {
                                                printf("Failed to create thread B\n");
                                                sem_post(&write);
                                            }
                                            else
                                            {
                                                /* Create ThreadC */
                                                if(pthread_create(&tidC, NULL, (void *)threadC, &dataC) != 0)
                                                {
                                                    printf("Failed to create thread C\n");
                                                    sem_post(&write);
                                                    sem_post(&read);
                                                }

                                                else
                                                {
                                                    /* Start */
                                                    pthread_mutex_lock(&eofMutex);
                                                    eof = 0;
                                                    pthread_mutex_unlock(&eofMutex);

                                                    sem_post(&write);

                                                    /* Finished */
                                                    pthread_join(tidA,NULL);
                                                    pthread_mutex_lock(&eofMutex);
                                                    eof = 1;
                                                    pthread_mutex_unlock(&eofMutex);
                                                    sem_post(&read);
                                                    sem_post(&justify);
                                                    pthread_join(tidB,NULL);
                                                    pthread_join(tidC,NULL);
                                                }
                                            }
                                        }

                                        /* Close src file */
                                        if(fclose(srcFile) != 0)
                                            printf("Failed to close %s.\n", SRC_FILE);
                                    }

                                    /* Close data file*/
                                    if(fclose(dataFile) != 0)
                                        printf("Failed to close %s.\n", DATA_FILE);
                                }

                                /* Destroy semaphore justify */
                                if(sem_destroy(&justify) < 0)
                                    printf("Failed to destroy semaphore justify.\n");
                            }

                            /* Destroy semaphore read */
                            if(sem_destroy(&read) < 0)
                                printf("Failed to destroy semaphore read.\n");
                        }

                        /* Destroy semaphore write */
                        if(sem_destroy(&write) < 0)
                            printf("Failed to destroy semaphore write.\n");
                    }

                    /* Destroy mutex sharedMutex */
                    if(pthread_mutex_destroy(&sharedMutex) != 0)
                        printf("Failed to destroy sharedMutex.\n");
                }

                /* Destroy mutex eofMutex */
                if(pthread_mutex_destroy(&eofMutex) != 0)
                    printf("Failed to destroy eofMutex.\n");
            }

            /* Free shared memory*/
            free(shared);
        }

        /* Close Pipe */
        if(close(fd[0])<0)
            perror("close(fd[0])");
        if(close(fd[1])<0)
            perror("close(fd[1])");
    }

	/* Calculate Duration */
	clock_gettime(CLOCK_REALTIME, &end);
	int duration = (int)((end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec));
	char durationAsString[SHM_MESSLENGTH];
	sprintf(durationAsString, "Duration: %d Nanoseconds", duration);

	/* Write to shared memory */
	if(!writeToSharedMemory(SHM_NAME, durationAsString))
		printf("Failed to write to shared memory\n");

    /* Test */
    //printf("Duration: %d Nanoseconds\n",duration);
    //printf("%s",durationAsString);

    return 0;
}
