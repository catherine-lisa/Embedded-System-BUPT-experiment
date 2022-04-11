/*
 * mutex.c
 *
 *      Author: ljp
 */

#include "kern_api.h"

void mutex_init(mutex_t *mutex) {
	mutex->magic = MAGIC_MUTEX;
	mutex->owner = NULL;
	mutex->take_count = 0;
	mutex->original_prio = 0;
}

//P
int mutex_take(mutex_t *mutex) {
	ASSERT(mutex->magic == MAGIC_MUTEX);
	ASSERT(!cpu_in_irq());

	while (1) {
		CPU_CRITICAL_ENTER();

		//resource available
		if (mutex->owner == NULL) {
			mutex->owner = task_self(); //mark mutex taken
			ASSERT(mutex->take_count == 0);
			mutex->take_count = 1;
			mutex->original_prio = task_priority(mutex->owner);//1
			CPU_CRITICAL_EXIT();
			return 0;
		}

		//resource already taken by me
		if (mutex->owner == task_self()) {
			ASSERT(mutex->take_count > 0);
			mutex->take_count++;
			CPU_CRITICAL_EXIT();
			return 0;
		}

		//resource unavailable
		//处理优先级反转-拥有者继承当前高优先级任务的优先级
		int myprio = task_priority(task_self());
		if(task_priority(mutex->owner) < myprio) {
			task_set_priority(mutex->owner, myprio);//2
		}
		task_wait(mutex);
		CPU_CRITICAL_EXIT();

		continue;
	}

//	ASSERT(0);
//	return 0;
}

//V
void mutex_give(mutex_t *mutex) {
	ASSERT(mutex->magic == MAGIC_MUTEX);
	ASSERT(mutex->take_count > 0);
	ASSERT(mutex->owner == task_self());

	CPU_CRITICAL_ENTER();
	mutex->take_count--;
	if (mutex->take_count == 0) {
		mutex->owner = NULL; //mutex available
		task_set_priority(mutex->owner, mutex->original_prio);//3
		mutex->original_prio = 0;
		task_wakeup_all(mutex); //may no task is waiting...
	}
	CPU_CRITICAL_EXIT();
}
