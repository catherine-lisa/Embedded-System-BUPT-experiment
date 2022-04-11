/*
 * semaphore_api.h
 *
 *      Author: ljp
 */

#ifndef SEMAPHORE_API_H_
#define SEMAPHORE_API_H_

#define MAGIC_SEMAPHORE (0x53454d41) // "SEMA"

#define SEM_INITIALIZER {MAGIC_SEMAPHORE, 0}
#define DEFINE_SEM(sem) sem_t sem = SEM_INITIALIZER

typedef struct semaphore {
	unsigned int magic;
	int count;
} sem_t;

void sem_init(sem_t *sem, int count);
int sem_take(sem_t *sem, tick_t timeout);
void sem_give(sem_t *sem);

#endif /* SEMAPHORE_API_H_ */
