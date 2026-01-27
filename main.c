#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h> 
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <mqueue.h>
#include <sys/mman.h>
#include <fcntl.h> 
#include <semaphore.h>
#include "process_1.h"
#include "process_2.h"
#include "process_3.h"
#include "constants.h"

SharedData *shmPtr = NULL;
sem_t *sem_parent_to_p3 = NULL;
sem_t *sem_p3_to_p2 = NULL;
sem_t *sem_p2_to_p1 = NULL;

static void P2SigHandler(int signum) {
    printf("PM(%i): Received from p2\n", getpid());
    //shmPtr->signum = shmPtr->signum;
    sem_post(sem_parent_to_p3);
    // //exit(1);
}

static void SetupSignals() {
    sigset_t signals;
    sigfillset(&signals);
    sigdelset(&signals, SIGUSR1);
    sigprocmask(SIG_BLOCK, &signals, NULL);

    signal(SIGUSR1, P2SigHandler);
}

mqd_t CreateMessageQueue() {
    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = MAX_TEXT_SIZE;
    attr.mq_curmsgs = 0;
    return mq_open(MQ_NAME, O_RDWR | O_CREAT, 0644, &attr);
}

int main() {
    SetupSignals();

    pid_t p3_pid = -1;
    pid_t p2_pid = -1;
    pid_t p1_pid = -1;

    int shmId = -1;
    shmId = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shmId < 0) {
        printf("Failed to create shared memory\n");
        exit(1);
    }
    if (ftruncate(shmId, sizeof(SharedData)) < 0) {
        printf("Failed to allocate memory for shared data\n");
        exit(1);
    }

    shmPtr = mmap(0, sizeof(SharedData), PROT_WRITE, MAP_SHARED, shmId, 0);
    if (shmPtr == MAP_FAILED) {
        printf("Failed to create shmPtr\n");
        exit(1);
    }

    sem_parent_to_p3 = sem_open("/sem_parent_to_p3", O_CREAT | O_EXCL, 0666, 0);
    if (sem_parent_to_p3 == SEM_FAILED) {
        printf("Failed to create sem_parent_to_p3\n");
        exit(1);
    }
    sem_p3_to_p2 = sem_open("/sem_p3_to_p2", O_CREAT | O_EXCL, 0666, 0);
    if (sem_p3_to_p2 == SEM_FAILED) {
        printf("Failed to create semaphore sem_p3_to_p2\n");
        exit(1);
    }
    
    sem_p2_to_p1 = sem_open("/sem_p2_to_p1", O_CREAT | O_EXCL, 0666, 0);
    if (sem_p2_to_p1 == SEM_FAILED) {
        printf("Failed to create semaphore sem_p2_to_p1\n");
        exit(1);
    }

    int pipe_p3_p2[2];
    if (pipe(pipe_p3_p2) < 0) {
        printf("Failed to create p3-p2 pipe\n");
        exit(1);
    }

    p3_pid = fork();
    if (p3_pid < 0) {
        printf("Error forking process 3\n");
        exit(1);
    }
    if (p3_pid == 0) {
        close(pipe_p3_p2[R]);
        process3(pipe_p3_p2[W], sem_parent_to_p3, sem_p3_to_p2, shmId);
        exit(0);
    }

    mqd_t mq_p2_p1 = -1;
    mq_p2_p1 = CreateMessageQueue();
    if (mq_p2_p1 < 0) {
        printf("Failed to create message queue\n");
        exit(1);
    }

    p2_pid = fork();
    if (p2_pid < 0) {
        printf("Error forking process 2\n");
        exit(1);
    }
    if (p2_pid == 0) {
        close(pipe_p3_p2[W]);
        process2(pipe_p3_p2[R], mq_p2_p1, shmId, sem_p3_to_p2, sem_p2_to_p1);
        mq_close(mq_p2_p1);
        exit(0);
    }

    close(pipe_p3_p2[R]);
    close(pipe_p3_p2[W]);

    p1_pid = fork();
    if (p1_pid < 0) {
        printf("Error forking process 1\n");
        exit(1);
    }
    if (p1_pid == 0) {
        process1(mq_p2_p1, sem_p2_to_p1, shmId);
        mq_close(mq_p2_p1);
        exit(0);
    }
  
    wait(NULL);
    wait(NULL);
    wait(NULL);
    mq_close(mq_p2_p1);
    shm_unlink(SHM_NAME);
    mq_unlink(MQ_NAME);
    sem_close(sem_parent_to_p3);
    sem_close(sem_p3_to_p2);
    sem_close(sem_p2_to_p1);
    sem_unlink("/sem_parent_to_p3");
    sem_unlink("/sem_p3_to_p2");
    sem_unlink("/sem_p2_to_p1");

    printf("end");
}