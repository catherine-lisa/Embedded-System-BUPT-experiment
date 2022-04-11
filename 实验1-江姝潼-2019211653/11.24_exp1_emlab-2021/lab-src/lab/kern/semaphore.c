/*
 * semaphore.c
 *
 *      Author: ljp
 */

#include "kern_api.h"

void sem_init(sem_t *sem, int count) {
	sem->count = count;
	sem->magic = MAGIC_SEMAPHORE;
}

//P, 0:try take, -1:wait forever, else: wait 'timeout' ticks
int sem_take(sem_t *sem, tick_t timeout) {
	ASSERT(sem->magic == MAGIC_SEMAPHORE);
	ASSERT(!((timeout != WAIT_NONE) && cpu_in_irq()));

	tick_t expire_time = timer_ticks();
	if (timeout != WAIT_FOREVER) {
		expire_time += timeout;
	}

	while (1) {
		CPU_CRITICAL_ENTER();

		if (sem->count > 0) {	//have resource, take it
			--sem->count;
			CPU_CRITICAL_EXIT();
			return 0;
		}

		//no resource, no wait
		if (timeout == WAIT_NONE) {
			CPU_CRITICAL_EXIT();
			return -1;	//try failed
		}

		//no resource, wait resource until available
		if (timeout == WAIT_FOREVER) {
			task_wait(sem);
			CPU_CRITICAL_EXIT();
			continue; //waken up, try again
		}

		ASSERT(sem->count <= 0);
		//no resource, wait resource until available or timeout
		tick_t now  = timer_ticks();
		if (TIME_GE(now, expire_time)) {
			CPU_CRITICAL_EXIT();
			return -1;	//if timeout and no resource, return error
		} else { //no resource, no timeout, wait to wakeup
			task_wait_timeout(sem, expire_time - now);
			CPU_CRITICAL_EXIT();//ensure task_wait_timeout
			continue;   //timeout or resource available? just try again
		}

//		ASSERT(0);
//		CPU_CRITICAL_EXIT();
	}

//	ASSERT(0);
//	return 0;
}

//V
void sem_give(sem_t *sem) {
	ASSERT(sem->magic == MAGIC_SEMAPHORE);
//	ASSERT(sem->count >= 0);
	CPU_CRITICAL_ENTER();
	sem->count++;
	if(sem->count > 0) {
		task_wakeup_all(sem); //may no task is waiting...
	}
	CPU_CRITICAL_EXIT();
}
