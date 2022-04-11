/*
 * mutex_api.h
 *
 *      Author: ljp
 */

#ifndef MUTEX_API_H_
#define MUTEX_API_H_

#define MAGIC_MUTEX (0x4d555458) // "MUTX"

#define MUTEX_INITIALIZER {MAGIC_MUTEX, NULL, 0}
#define DEFINE_MUTEX(mtx) mutex_t mtx = MUTEX_INITIALIZER

struct task_t;
typedef struct mutex {
	unsigned int magic;
	struct task_t *owner;	//taken by owner
	int take_count;	//nest take allowed
	int original_prio;	//original priority of mutex owner
} mutex_t;

void mutex_init(mutex_t *mutex);
int mutex_take(mutex_t *mutex);
void mutex_give(mutex_t *mutex);

#endif /* MUTEX_API_H_ */
