#include "process_1.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <mqueue.h>
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
    sigprocmask(SIG_BLOCK, &signals, NULL);
}

void process1(mqd_t mq_p2_p1, sem_t *sem_p2_to_p1, int shmId, int pipe_p1_p3_write)
{
    SetupSignals();

    shmPtr = mmap(0, sizeof(SharedData), PROT_READ, MAP_SHARED, shmId, 0);
    if (!shmPtr)
    {
        printf("Failed to create shmPtr\n");
        exit(1);
    }

    char buf[MAX_TEXT_SIZE];
    int shouldSkip = 0;
    while (1)
    {
        if (!shouldSkip)
        {
            size_t bytesRead = mq_receive(mq_p2_p1, buf, MAX_TEXT_SIZE, NULL);
            if (bytesRead < 0)
            {
                printf("Failed to receive message from queue\n");
                exit(1);
            }
            buf[bytesRead] = '\0';
            printf("P1(%d): message received from P2: %s\n", getpid(), buf);
            write(pipe_p1_p3_write, "ACK", 3);
        }

        if (sem_trywait(sem_p2_to_p1) == 0)
        {
            if (shmPtr->signum == SIGTERM)
            {
                printf("P1(%i): received signal %i\n", getpid(), shmPtr->signum);
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