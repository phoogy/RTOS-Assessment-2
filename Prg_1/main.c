/*
 * Phong Au - 10692820
 * Last Modified 08-04-2018
 * Compile by using gcc -Wall -lpthread -o Prg_1 main.c
 *
 */

// Includes
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <semaphore.h>
#include <time.h>

// Pipe file descriptors
int fd[2];

// Thread Ids
pthread_t tidA, tidB, tidC;

// Shared Char
char sharedChar;

// Microsecond_timer
int microsecondTimer;

// Mutex
pthread_mutex_t sharedCharMutex;

// Semaphores
sem_t sem_threadA, sem_threadB;

struct ThreadData{
    int finished;
    int dataCount;
    char character;
} threadData;

// Thread A
void *threadA(ThreadData *threadData)
{
    // Declare Variables
    char ch;                        // Char variable used for handling data.
    FILE *file;                     // File to read.

    // Initialise
    file = fopen("data.txt", "r");  // Open file.

    // Loop while not end of file
    while (!feof(file))
    {
        sem_wait(&sem_threadA)
        ch = fgetc(file);           // Get next char.
        write(fd[1],&ch,1);         // Write to pipe.
        sem_post(&sem_threadB);
    }
    return 0;
}

// Thread B
void *threadB(ThreadData *threadData)
{
    // Declare Variables
    char ch;

    // Loop
    while(1){                                   // Loop Forever.
        sem_wait(&sem_threadB);

        read(fd[0],&ch,1);                      // Read from pipe.
        pthread_mutex_lock(&sharedCharMutex);   // Mutex lock the shared char.
        sharedChar = ch;                        // Set sharedChar.
        pthread_mutex_unlock(&sharedCharMutex); // Mutex unlock the shared char.
        sem_post(&sem_threadC);                   // Post/Signal semaphore dataReady.

        pthread_mutex_lock(&sharedThreadData);
        if (threadData.finished == 1 && threadData.dataCount == 0)
        {
            break;
        }


    }
}

// Thread C
void *threadC(ThreadData *threadData)
{
    // Declare Variables
    char endHeader[] = "end_header";    // String to look for to find end_header.
    char data[sizeof(endHeader)];       // Sequence of char from the data same size as end_header.
    FILE *src;                          // File to write to.
    int endHeaderFound = 0;             // Flag for end_header found.

    // Initialise
    // Empty src file.txt
    // src = fopen("src.txt","w");
    // fclose(src);

    // Loop
    while (1)
    {
        // Move data in the array to prepare for new data to pushed in from the end.
        for (int i = 0; i < sizeof(data); i++)
        {
            data[i] = data[i+1];
        }

        // Get data from shared resource
        sem_wait(&dataReady);                   // Wait for thread b to put data in shared resource.
        pthread_mutex_lock(&sharedCharMutex);   // Lock the shared resource.
        data[sizeof(data)] = sharedChar;        // Get data from shared resource.
        pthread_mutex_unlock(&sharedCharMutex); // Unlock the shared resource.
        sem_post(&dataProcessed);               // Signal finished getting data from shared resource.

        if (endHeaderFound == 1)
        {
            // Write data to file
            src = fopen("src.txt","a");         // Open File.
            fputc(data[sizeof(data)], src);     // Append data to src.txt.
            fclose(src);                        // Close File.

        }else if (strncmp(data,endHeader,sizeof(endHeader)-1) == 0) // If sequence of data == endHeader.
        {
            endHeaderFound = 1;                 // End header found = true.
        }
    }
}

// Main
int main(void)
{

    threadData.dataCount = 0;
    threadData.finished = 0;



    // Initialise
    int startTime = clock() * 1000000 / CLOCKS_PER_SEC; // Initialise Timer
    pthread_mutex_init(&sharedCharMutex,NULL);  // Init Mutex sharedCharMutex.
    sem_init(&dataReady, 0, 0);                 // Init Semaphore dataReady.
    sem_init(&dataProcessed, 0, 1);             // Init Semaphore dataProcessed with +1 vaue.
    pipe(fd);                                   // Init Pipe.

    // Create Threads
    pthread_create(&tidA,NULL,threadA,NULL);    // Create Thread A.
    pthread_create(&tidB,NULL,threadB,NULL);    // Create Thread B.
    pthread_create(&tidC,NULL,threadC,NULL);    // Create Thread C.

    //Wait for Threads to finish.
    pthread_join(tidA,NULL);        // Thread A finishes when file being read reaches end of file.
    // pthread_join(tidB,NULL);
    // pthread_join(tidC,NULL);

	usleep(100000);

	// Time
	microsecondTimer = (clock() * 1000000 / CLOCKS_PER_SEC) - startTime;   // Calculate total time
    printf("Duration: %d Microseconds",microsecondTimer);         // Print Duration to terminal
    return 0;
}
