/*
 *      Author: ljp
 */

#include <stdlib.h>
#include <string.h>
#include "kern_api.h"
#include "task_api.h"
#include "kern_port.h"
#include "bitops.h"

#define LOCAL_TRACE 0

#define MAGIC_TASK	0x5441534b //"TASK"

#define ASSERT_TASK(t) ASSERT((t) && (t)->magic == MAGIC_TASK)

// static TAILQ_HEAD(ready_queue_t, task_t) ready_queue;
// static TAILQ_HEAD(wait_queue_t, task_t) wait_queue;

static unsigned int ready_queue_bitmap;
static TAILQ_HEAD(ready_queue_t, task_t) ready_queue[PRIO_HIGHEST + 1];

STATIC_ASSERT(PRIO_HIGHEST < sizeof(ready_queue_bitmap) * 8);

 /* may be a better hash? */
#define WQ_HASH_SIZE	32	//128
#define LOOKUP(x)	(((long)(x) >> 8) & (WQ_HASH_SIZE - 1))
#define WHICH_WQ(x)	&wait_queue[LOOKUP(x)]
static TAILQ_HEAD(wait_queue_t, task_t) wait_queue[WQ_HASH_SIZE];


task_t *current_task;	//who is running now
static task_t *idle_task;
static int _scheduler_lock_count = 0;

int _rq_task_count = 0;//for debug
int _wq_task_count = 0;
static int _task_count;
/*static*/ task_t _task_pool[CONFIG_STATIC_TASK_LIMIT];
static TAILQ_HEAD(task_deathpool_t, task_t) _task_deathpool;


//////////////////////////////////////////////////////
//local functions
static void task_schedule(void);

/* task pool manipulation */
static void task_pool_init(void) {
	TAILQ_INIT(&_task_deathpool);
	FOR_EACH(i, _task_pool) {
        task_t *task = &_task_pool[i];
        memset(task, 0, sizeof(task_t));
        task->state = TASK_DEAD;
        TAILQ_INSERT_TAIL(&_task_deathpool, task, link);
    }
	_task_count = 0;
}

static task_t* task_pool_get() {
	task_t *t;
	WITHIN_CRITICAL() {
		t = TAILQ_FIRST(&_task_deathpool);
		if(t) {
			TAILQ_REMOVE(&_task_deathpool, t, link);
			_task_count++;
		}
	}
	return t;
}

static void task_pool_put(task_t *t) {
	WITHIN_CRITICAL() {
		if(&_task_pool[0] <= t && t < &_task_pool[CONFIG_STATIC_TASK_LIMIT])  {
			TAILQ_INSERT_TAIL(&_task_deathpool, t, link);
			_task_count--;
		}
	}
}

/* ready queue manipulation */
static void ready_queue_insert_tail(task_t *t) {
	ASSERT(t->prio < sizeof(ready_queue_bitmap) * 8);
	TAILQ_INSERT_TAIL(&ready_queue[t->prio], t, stateq_node);
	ready_queue_bitmap |= (1 << t->prio);
	_rq_task_count++;
}

static void ready_queue_remove(task_t *t) {
	ASSERT(t->prio < sizeof(ready_queue_bitmap) * 8);
	TAILQ_REMOVE(&ready_queue[t->prio], t, stateq_node);
	if(TAILQ_EMPTY(&ready_queue[t->prio])) {
		ready_queue_bitmap &= ~(1 << t->prio);
	}
	_rq_task_count--;
}

static task_t* ready_queue_get_highest(void) {
	if(ready_queue_bitmap == 0) { //all are blocked but current, it must be idle_task
		ASSERT(current_task == idle_task);
		return current_task;
	}

	int highest_prio = fls32(ready_queue_bitmap) - 1;
	task_t *highest_task = TAILQ_FIRST(&ready_queue[highest_prio]);
	ASSERT(highest_task != NULL);
	return highest_task;
}

static void ready_queue_init() {
	FOR_EACH(i, ready_queue) {
        TAILQ_INIT(&ready_queue[i]);}
}


static void wait_queue_insert_tail(task_t *t) {
	// ASSERT(t->wait_event != NULL);
	TAILQ_INSERT_TAIL(WHICH_WQ(t->wait_event), t, stateq_node);
	_wq_task_count++;
}

/* wait queue manipulation */
static void wait_queue_remove(task_t *t) {
	// ASSERT(t->wait_event != NULL);
	TAILQ_REMOVE(WHICH_WQ(t->wait_event), t, stateq_node);
	_wq_task_count--;
}

static void wait_queue_init(void) {
	FOR_EACH(i, wait_queue) {
		TAILQ_INIT(&wait_queue[i]);
	}
}


/* redzone manipulation */
static void redzone_fill(task_t *t) {
	FOR_EACH(i, t->redzone) {
		t->redzone[i] = REDZONE_FILL;
	}
}

static void redzone_check(task_t *t) {
	FOR_EACH(i, t->redzone) {
		if(REDZONE_FILL != t->redzone[i]){
			PANIC("task %s stack overflow!\n", t->name);
		}
	}
}

//////////////////////////////////////////////////////
//task getter/setter

task_t *task_self(void) {
	return current_task;
}

//task name getter/setter
char *task_name(task_t *t) {
	if(t == NULL) {
		t = task_self();
	}
	return (char*)t->name;
}

void task_set_name(const char *name) {
	current_task->name = name;
}

//task priority getter/setter
int task_priority(task_t *t) {
	if(t == NULL) {
		t = task_self();
	}
	return t->prio;
}

void task_set_priority(task_t *t, int priority) {
    if (priority < PRIO_LOWEST)  priority = PRIO_LOWEST;
	if (priority > PRIO_HIGHEST) priority = PRIO_HIGHEST;	
	if(t == NULL) {
		t = task_self();
	}

	WITHIN_CRITICAL() {
		if(t->state == TASK_READY) {
			ready_queue_remove(t);
		   	t->prio = priority;
			ready_queue_insert_tail(t);
			task_schedule();
		} else {
			t->prio = priority;
		}
	}
}

// thread local storage getter/setter
void* task_tls_get(int entry) {
	if(entry >= 0 && (unsigned int)entry < NELEM(current_task->tls)) {
	    return current_task->tls[entry];
	}
	return NULL;
}

void* task_tls_set(int entry, void* val) {
	if(entry >= 0 && (unsigned int)entry < NELEM(current_task->tls)) {
		void* oldval = current_task->tls[entry];
		current_task->tls[entry] = val;
	    return oldval;
	}
	return NULL;
}

static void tls_copy(task_t *to, task_t *from) {
    int i;
	if(to == NULL || from == NULL) {
		return;
	}
    for (i=0; i < CONFIG_TASK_TLS_SIZE; i++) {
        to->tls[i] = from->tls[i];
	}
}

//////////////////////////////////////////////////////
//schedule service

//lock the thread scheduler 
int task_lock(void) {
	ASSERT(_scheduler_lock_count >= 0);
	WITHIN_CRITICAL() {
		_scheduler_lock_count++;
	}
	return _scheduler_lock_count;
}

//unlock the thread scheduler
int task_unlock(void) {
	ASSERT(_scheduler_lock_count > 0);
	WITHIN_CRITICAL() {
		_scheduler_lock_count--;
		if(_scheduler_lock_count <= 0) {
			_scheduler_lock_count = 0;
			if(current_task) { //if scheduler is started, do a schedule
				task_schedule();
			}
		}
	}
	return _scheduler_lock_count;
}


static task_t* task_change_state_to(task_t* t, state_t newstate) {
	ASSERT(cpu_irq_disabled());

	switch (t->state) {
		case TASK_DEAD:
			if(newstate == TASK_READY) { //created
				//add task to ready list
				ready_queue_insert_tail(t);
			} else {
				ASSERT(0 && "TASK_DEAD state error");
				return t;
			}
			break;
		case TASK_READY:
			if(newstate == TASK_RUNNING) { //run
				ready_queue_remove(t);
			} else if(newstate == TASK_BLOCKED) { //suspended
				ready_queue_remove(t);
				wait_queue_insert_tail(t);
			} else if(newstate == TASK_DEAD) { //deleted
				ready_queue_remove(t);
			} else {
				ASSERT(0 && "TASK_READY state error");
				return t;
			}
			break;
		case TASK_RUNNING:
			if(newstate == TASK_READY) { //preempted
				ready_queue_insert_tail(t);
			} else if(newstate == TASK_BLOCKED) { //blocked
				wait_queue_insert_tail(t);
			} else if(newstate == TASK_DEAD) { //suicide
			} else {
				ASSERT(0 && "TASK_RUNNING state error");
				return t;
			}
			break;
		case TASK_BLOCKED:
			if(newstate == TASK_READY) { //unblocked
				wait_queue_remove(t);
				ready_queue_insert_tail(t);
			} else if(newstate == TASK_DEAD) { //deledted
				wait_queue_remove(t);
			} else {
				ASSERT(0 && "TASK_BLOCKED state error");
				return t;
			}
			break;
		default:
				ASSERT(0 && "task unknown state error");
				return t;
			//break;
	}

	t->state = newstate;
	return t;
}

//parent function for monitor
static void task_guard(task_t* th) {
	if (!th) {
		return;
	}
	if (th->entry) {
		(*th->entry)(th->arg);
	}
	task_delete(current_task);//should be current_task for task_fork
}

/*
static task_t *pick_next_fifo(void) {
	return TAILQ_FIRST(&ready_queue);
}

static task_t *pick_next_rr(void) {
	task_t* t = TAILQ_FIRST(&ready_queue);
	ready_queue_remove(t);
	ready_queue_insert_tail(t);
	return t;
}
*/

static task_t *pick_next_rt(void) {
	task_t* highest = ready_queue_get_highest();
	
	if(highest != NULL && current_task != NULL) {//current_task is not in ready queue
		if(current_task->state == TASK_RUNNING && current_task->prio > highest->prio)
		highest = current_task;
	}
	return highest;
}

__weak void task_switch_context_hook(task_t *from, task_t *to) {
	TRACE_D("[%d] %s(pri %d sp %08x) -> %s(pri %d sp %08x)", 
		timer_ticks(), from->name, from->prio, from->context, to->name, to->prio, to->context);
}

stack_t* task_switch_sp(stack_t *sp) {
	if(current_task) {
		current_task->context = (void*) sp;
		redzone_check(current_task);
	}

	task_t *next_task = pick_next_rt();
	ASSERT(next_task != NULL);
	if(current_task != next_task) {
		task_change_state_to(next_task, TASK_RUNNING);
		if(current_task->state == TASK_RUNNING) {
			task_change_state_to(current_task, TASK_READY);
		}
	}
	ASSERT(_wq_task_count + _rq_task_count + 1 == _task_count);

	redzone_check(next_task);
	next_task->switch_count++;

	task_switch_context_hook(current_task, next_task);

	current_task = next_task;
	return (void*)next_task->context;
}

static void task_schedule(void) {
	ASSERT(cpu_irq_disabled());

	if (_scheduler_lock_count > 0) { //if scheduler is disabled/locked
		return;
	}

	//request to do a schedule
	cpu_schedule_pend();
}

int task_timer_tick(void) {
	WITHIN_CRITICAL() {
		task_schedule();
	}
	return 1;
}

//////////////////////////////////////////////////////
//task block <-> unblock

void __task_wait(void *event DEBUG_CONTEXT_ARGS) {
#if CONFIG_DEBUG
	current_task->wait_at = where;
#endif
	ASSERT(!cpu_in_irq());

	WITHIN_CRITICAL() {
		current_task->wait_event = event;
		task_change_state_to(current_task, TASK_BLOCKED);
		task_schedule();
	}
}

void task_join(task_t *t) {
	WITHIN_CRITICAL() {
		if(t->state != TASK_DEAD) { //ensure task is alive
			task_wait(t);
		}
	}
}

void task_wakeup_n(void *event, int n) {
	task_t *t;
	struct wait_queue_t * wq = WHICH_WQ(event);
	ASSERT(event != NULL);

	WITHIN_CRITICAL() {
		int need_resched = 0;
		TAILQ_FOREACH(t, wq, stateq_node) {
			ASSERT(t->wait_event != NULL);
			if(t->wait_event == event) {
				if(n-- == 0) {
					break;
				}
				task_change_state_to(t, TASK_READY);
				t->wait_event = NULL;
				need_resched = 1;
			}
		}
		if(need_resched) {
			task_schedule();
		}
	}
}

void task_wakeup(void *event) {
	task_wakeup_n(event, 1);
}

void task_wakeup_all(void *event) {
	task_wakeup_n(event, -1);
}

void task_wakeup_the(task_t *t) {
	ASSERT_TASK(t);

	WITHIN_CRITICAL() {
		task_change_state_to(t, TASK_READY);
		task_schedule();
	}
}

void task_suspend(task_t *t) {
	WITHIN_CRITICAL() {
		if(t == NULL) {
			t = task_self();
		}
		task_change_state_to(t, TASK_BLOCKED);
		task_schedule();
	}
}

void task_resume(task_t *t) {
	ASSERT_TASK(t);

	task_wakeup_the(t);
}


void task_yield(void) {
	ASSERT(!cpu_in_irq());

	WITHIN_CRITICAL() {
		task_schedule();
	}
}

//////////////////////////////////////////////////////
//task crate <-> delete

void task_delete(task_t *t) {
	ASSERT_TASK(t);

	WITHIN_CRITICAL() {
		if(t == NULL) {
			t = task_self();
		}
		task_change_state_to(t, TASK_DEAD);
		task_pool_put(t);
		t->magic = 0;
		task_wakeup_all(t);//wake all joining task
		task_schedule();
	}
}

static task_t * task_new(task_func_t func, void *arg, int prio, const char *name) {
	task_t *t;
	t = task_pool_get();
	if(t == NULL) {
		return NULL;
	}
	ASSERT(t->magic == 0);
	ASSERT(t->state == 0 || t->state ==TASK_DEAD);
	
	//param
	t->entry = func;
	t->arg = arg;
	t->prio = prio;
	t->name = name;
	//default
	t->magic = MAGIC_TASK;
	t->state = TASK_DEAD;
	t->wait_event = NULL;
	t->wait_at = NULL;
	t->switch_count = 0;
	tls_copy(t, current_task); //inherit thread local storage from the parent

	//Init stack
	redzone_fill(t);
	memset(t->stack, 'E', sizeof(t->stack));

	return t;
}

task_t * task_create(task_func_t func, void *arg, int prio, const char *name) {
	task_t *t = task_new(func, arg, prio, name);
	ASSERT(t != NULL);
	if(t == NULL) {
		return NULL;
	}

	//create an init stack frame for function to become task
	stack_t *stack_top = (stack_t*) ((char*) t->stack + sizeof(t->stack));
	t->context = (void*)cpu_task_stack_prepair(stack_top, (task_func_t) task_guard, t);

	WITHIN_CRITICAL() {
		task_change_state_to(t, TASK_READY);
	}
	return t;
}

int task_fork(void) {
	task_t *t = task_new(current_task->entry, current_task->arg, current_task->prio, current_task->name);
	ASSERT(t != NULL);
	if(t == NULL) {
		return -1;
	}

	//Compute how much of the stack is used
	int used = current_task->stack + CONFIG_TASK_STACK_SIZE - (stack_t*)current_task->context;
	//New stack is END - used
	t->context = (void *)(t->stack + CONFIG_TASK_STACK_SIZE - used);
	//Copy only the used part of the stack
	memcpy(t->context, current_task->context, used * sizeof(stack_t));

	/* Set return values in child task */
	t->context->R0 = 0;
	/* Set return values in parent task */
	current_task->context->R0 = (int)t;

	WITHIN_CRITICAL() {
		task_change_state_to(t, TASK_READY);
	}
	//return to parent task
	return (int)t;
}

//////////////////////////////////////////////////////
//for debug
static const char *task_state_string[] = {"DEAD", "READY", "RUNNING", "BLOCK"};

void task_info(task_t *t) {
	if(t == NULL) {
		t = task_self();
	}
	trace_printf("%-16s(%p): pri %2u state %7s wait %p stack %p|%p|%p switch-ctr %u\n",
		t->name, t, t->prio, task_state_string[t->state], t->wait_event, 
		t->stack, t->context,  t->stack + CONFIG_TASK_STACK_SIZE, t->switch_count);
}

void task_info_all(void) {
	task_t *t;
	trace_printf("\n%d task alive. READY %d BLOCK %d Current %p.\n", _task_count, _rq_task_count, _wq_task_count, current_task);
	for(t = _task_pool; t < _task_pool + CONFIG_STATIC_TASK_LIMIT; t++) {
		if(t->magic == MAGIC_TASK) {
			task_info(t);
		}
	}
}

//////////////////////////////////////////////////////
//task lib init

void task_init(){
	ready_queue_init();
	wait_queue_init();
	task_pool_init();

	_scheduler_lock_count = 0;
	current_task = NULL;
}

static int _idle_count;

static void idle_task_entry(void){
	_idle_count = 0;
	while(1) {
		ASSERT(!cpu_irq_disabled());
		//task_yield();
		_idle_count++;
//		HAL_CPU_Sleep();
//		trace_printf("#");
//		HAL_Delay(100);
	}
}

void task_lib_init(void) {
	task_init();
//	set_cpu_context_switch_call(&syscall_pendsv);
	idle_task = task_create((task_func_t)idle_task_entry, NULL, PRIO_LOWEST, "idle_task");
}

