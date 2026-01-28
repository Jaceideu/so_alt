#include <semaphore.h>
void process3(int pipe_p2_write, int pipe_p1_read, sem_t *sem_parent_to_p3, sem_t *sem_p3_to_p2, int shmId);