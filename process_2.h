#include <mqueue.h>
#include <semaphore.h>
void process2(int pipe_p3_read, mqd_t mq_p2_p1, int shmId, sem_t *sem_p3_to_p2, sem_t *sem_p2_to_p1);