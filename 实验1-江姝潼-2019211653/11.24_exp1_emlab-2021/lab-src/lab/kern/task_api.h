/*
 * task_api.h
 *
 *      Author: ljp
 */

#ifndef TASK_API_H_
#define TASK_API_H_

#include "kern_config.h"
#include "kern_def.h"
#include "queue.h"
#include "debug.h"

#define PRIO_LOWEST		0
#define PRIO_NORMAL		15
#define PRIO_HIGHEST	31

//#define CONFIG_TASK_STACK_SIZE 1024		//1024*sizeof(stack_t)

typedef void (* task_func_t)(void*);

typedef void *event_t;

typedef enum state_t {
    TASK_DEAD = 0,
    TASK_READY,
    TASK_RUNNING,
    TASK_BLOCKED,
} state_t;

typedef struct task_t {
	struct task_stack_frame_t *context; 		//pointer to current top of stack
//	stack_t *context;

	task_func_t entry;			//user function entry
	void *arg;				//user function argument

	int prio;			//priority. normal is zero. for interrupt backend task

	enum state_t state;
	TAILQ_ENTRY(task_t) stateq_node;	//ready or waiting

	TAILQ_ENTRY(task_t) link;	//link all task

	void *wait_event;		//wait something : mutex. sem. another task, timer,...
	const char *wait_at; 	//for debug, where waited at

	unsigned int magic;
	const char *name;		//name for debug
	unsigned int switch_count;

    void* tls[CONFIG_TASK_TLS_SIZE];	// thread local storage

#define REDZONE_FILL 'X'
	char redzone[8];		//redzone for task stack overflow

	stack_t stack[CONFIG_TASK_STACK_SIZE];	//we use static stack
} task_t;

extern task_t *current_task;

#define task_create_simple(func, arg)	task_create((task_func_t)(void*)(func), (void *)(arg), PRIO_NORMAL, #func)
task_t* task_create(task_func_t func, void *arg, int prio, const char *name);
int task_fork(void);
void task_delete(task_t *t);

task_t *task_self(void);
char *task_name(task_t *t);
void task_set_name(const char *name);
int task_priority(task_t *t);
void task_set_priority(task_t *t, int priority);
void* task_tls_get(int entry);
void* task_tls_set(int entry, void* val);

int task_lock(void);
int task_unlock(void);

//void task_wait(void *event);
void __task_wait(void *event DEBUG_CONTEXT_ARGS);
#define task_wait(e) __task_wait((e) DEBUG_CONTEXT)
void task_join(task_t *t);

void task_wakeup(void *event);
void task_wakeup_all(void *event);
void task_wakeup_the(task_t *t);

void task_suspend(task_t *t);
void task_resume(task_t *t);

void task_yield(void);

void task_wait_timeout(void *event, tick_t timeout);
void task_sleep(tick_t ticks);
void task_msleep(int ms);

stack_t* task_switch_sp(stack_t *sp);
int task_timer_tick(void);

void task_lib_init(void);

#endif /* TASK_API_H_ */
