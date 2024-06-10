// Required libraries
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <dirent.h>
#include <unistd.h>

#define MAX_FILES 100  // Maximum amount of files that are manageable
#define MAX_THREADS 10 // Maximum number of threads

sem_t sem;                  // Control resource access with a semaphore
pthread_mutex_t fileMutex;  // Using a mutex to synchronize file index access
char *fileQueue[MAX_FILES]; // File names are stored in an array.
int fileCount = 0;          // How many files are waiting in line
int currentIndex = 0;       // The following file's index after processing

// A function to determine if a given number is prime
int checkPrime(int num)
{
    if (num <= 1)
        return 0; // 1 and less are not prime numbers
    for (int i = 2; i * i <= num; i++)
    {
        if (num % i == 0)
            return 0; // If divisible by any number other than 1 and itself, it's not prime
    }
    return 1;
}

// Prime number counting in files using a thread function
void *isNumberPrime(void *arg)
{
    while (1)
    {
        pthread_mutex_lock(&fileMutex); // To safely access currentIndex, lock the mutex.
        if (currentIndex >= fileCount)
        { // Verify that every file has been processed.
            pthread_mutex_unlock(&fileMutex);
            break;
        }
        char *filename = fileQueue[currentIndex++]; // Obtain the file name next.
        pthread_mutex_unlock(&fileMutex);           // After accessing currentIndex, unlock the mutex.

        int prime_count = 0;
        FILE *file = fopen(filename, "r"); // Read the file by opening it.
        int number;

        while (fscanf(file, "%d", &number) != EOF)
        { // Take numbers out of the file.
            if (checkPrime(number))
            {
                prime_count++; // If the number is prime, increase the prime count.
            }
        }

        fclose(file); // Close the file
        printf("Thread %ld has found %d primes in %s\n", pthread_self(), prime_count, filename);
    }
    sem_post(&sem); // When the thread is done, signal the semaphore.
    return NULL;
}

// Thread setup and execution as the primary function
int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        printf("Usage: %s <directory> <threadNumber>\n", argv[0]); // Verify that the usage is correct.
        return 1;
    }

    int threadNumber = atoi(argv[2]);     // Count of threads to be utilized
    sem_init(&sem, 0, threadNumber);      // Initialize semaphore
    pthread_mutex_init(&fileMutex, NULL); // Initialize mutex

    DIR *dir = opendir(argv[1]); // Launch the designated directory.
    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL)
    { // Examine every entry in the directory.
        if (entry->d_type == DT_REG)
        {                                                       // Verify whether the entry is a standard file.
            char *filepath = malloc(1024);                      // Make memory available for the file path.
            sprintf(filepath, "%s/%s", argv[1], entry->d_name); // Using the entire file path
            fileQueue[fileCount++] = filepath;                  // Add to file queue
        }
    }

    closedir(dir); // Close the directory

    pthread_t threads[threadNumber]; // Thread identifier array
    for (int i = 0; i < threadNumber; i++)
    {
        sem_wait(&sem);                                         // Prior to starting each thread, await the semaphore.
        pthread_create(&threads[i], NULL, isNumberPrime, NULL); // Create a new thread
    }

    for (int i = 0; i < threadNumber; i++)
    {
        pthread_join(threads[i], NULL); // Await the completion of every thread.
    }

    sem_destroy(&sem);                 // Destroy semaphore
    pthread_mutex_destroy(&fileMutex); // Destroy mutex

    // No longer assigned, free file paths
    for (int i = 0; i < fileCount; i++)
    {
        free(fileQueue[i]);
    }

    return 0;
}