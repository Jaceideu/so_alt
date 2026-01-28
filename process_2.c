#include "process_2.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "constants.h"
#include <signal.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <semaphore.h>

extern SharedData *shmPtr;

static void Handler(int signum)
{
    shmPtr->signum = signum;
    kill(getppid(), SIGUSR1);
}

static void SetupSignals()
{
    sigset_t signals;
    sigemptyset(&signals);
    sigaddset(&signals, SIGTERM);
    sigaddset(&signals, SIGTSTP);
    sigaddset(&signals, SIGCONT);
    sigprocmask(SIG_UNBLOCK, &signals, NULL);
    signal(SIGTERM, Handler);
    signal(SIGTSTP, Handler);
    signal(SIGCONT, Handler);
}

void process2(int pipe_p3_read, mqd_t mq_p2_p1, int shmId, sem_t *sem_p3_to_p2, sem_t *sem_p2_to_p1)
{
    SetupSignals();

    shmPtr = mmap(0, sizeof(SharedData), PROT_READ | PROT_WRITE, MAP_SHARED, shmId, 0);
    if (!shmPtr)
    {
        printf("Failed to create shmPtr\n");
        exit(1);
    }

    int shouldSkip = 0;
    char buf[MAX_TEXT_SIZE];
    ssize_t n;
    while (shouldSkip || (n = read(pipe_p3_read, buf, sizeof(buf) - 1)) > 0)
    {
        if (!shouldSkip)
        {
            printf("P2(%d): Read data via pipe from P3\n", getpid());
            fflush(stdout);
            snprintf(buf, MAX_TEXT_SIZE, "%li", n);

            if (mq_send(mq_p2_p1, buf, strlen(buf) + 1, 1) < 0)
            {
                printf("Failed to send queue message\n");
                exit(1);
            }
            printf("P2(%d): Send message via message queue to P1\n", getpid());
        }

        if (sem_trywait(sem_p3_to_p2) == 0)
        {
            sem_post(sem_p2_to_p1);
            if (shmPtr->signum == SIGTERM)
            {
                printf("P2(%i): received signal %i\n", getpid(), shmPtr->signum);
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