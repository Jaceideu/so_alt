#include "process_3.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "constants.h"
#include <signal.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/mman.h>

extern SharedData *shmPtr;

static void SetupSignals()
{
    sigset_t signals;
    sigfillset(&signals);
    sigdelset(&signals, SIGINT);
    sigprocmask(SIG_BLOCK, &signals, NULL);
}

void process3(int pipe_p2_write, sem_t *sem_parent_to_p3, sem_t *sem_p3_to_p2, int shmId)
{

    shmPtr = mmap(0, sizeof(SharedData), PROT_READ, MAP_SHARED, shmId, 0);
    if (!shmPtr)
    {
        printf("Failed to create shmPtr\n");
        exit(1);
    }
    printf("MENU\n1. stdin\n2. input.txt\nEnter choice: ");
    int choice;
    int shouldSkip = 0;
    while (1)
    {
        if (!shouldSkip)
        {
            scanf("%d", &choice);
            while (getchar() != '\n')
                ;
            if (choice == 1)
            {
                fflush(stdout);
                char buf[MAX_TEXT_SIZE];
                fgets(buf, MAX_TEXT_SIZE, stdin);
                if (write(pipe_p2_write, buf, strlen(buf)) < 0)
                {
                    printf("Failed to write data using pipe to p2\n");
                    exit(1);
                }
                printf("P3(%d): Sent data via pipe to p2\n", getpid());
                fflush(stdout);
            }
        }

        if (sem_trywait(sem_parent_to_p3) == 0)
        {
            sem_post(sem_p3_to_p2);
            if (shmPtr->signum == SIGTERM)
            {
                printf("P3(%i): received signal %i\n", getpid(), shmPtr->signum);
                break;
            }
            else if (shmPtr->signum == SIGTSTP)
            {
                shouldSkip = 1;
            }
            else if (shmPtr->signum == SIGCONT)
            {
                shouldSkip = 0;
            }
        }
    }
}