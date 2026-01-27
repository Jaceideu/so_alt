#include <mqueue.h>
#include <semaphore.h>
void process1(mqd_t mq_p2_p1, sem_t *sem_p2_to_p1, int shmId);