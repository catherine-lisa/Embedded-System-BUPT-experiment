/*
 * timer.c
 *
 *      Author: ljp
 */


#include <stdlib.h>
#include <string.h>
#include "kern_api.h"
#include "task_api.h"
#include "kern_port.h"


#define LOCAL_TRACE 0

#define MAGIC_TIMER (0x54494d52)  //'TIMR'

#define TIMER_ON_QUEUE_FLAG 0x01
#define TIMER_IS_ON_QUEUE(tm)	(((tm)->flag & TIMER_ON_QUEUE_FLAG) == TIMER_ON_QUEUE_FLAG)
#define TIMER_ON_QUEUE(tm)		(tm)->flag |= TIMER_ON_QUEUE_FLAG
#define TIMER_OFF_QUEUE(tm)		(tm)->flag &= ~TIMER_ON_QUEUE_FLAG

static volatile int tick_task_started = 0;
static volatile tick_t time_ticks;
static TAILQ_HEAD(timer_queue_t, timer_t) timer_queue = TAILQ_HEAD_INITIALIZER(timer_queue);
/*static*/ int _timer_count = 0;    //for debug

static void timer_tick_task(void);

tick_t timer_ticks(void) {
	return time_ticks;
}

timer_t* timer_init(timer_t *tm) {
	tm->magic = MAGIC_TIMER;
	tm->expire_time = 0;
	tm->period_time = 0;
	tm->callback = NULL;
	tm->arg = NULL;
	tm->flag = 0;
    return tm;
}

static void timer_insert(timer_t *new_tm) {
    timer_t *tm;
    timer_t *temp;

    TIMER_ON_QUEUE(new_tm);
    TAILQ_FOREACH_MUTABLE(tm, &timer_queue, timer_node, temp) {
        if (TIME_GT(tm->expire_time, new_tm->expire_time)) {//first one
        	TAILQ_INSERT_BEFORE(tm, new_tm, timer_node);
            _timer_count++;
            return;
        }
    }

    TAILQ_INSERT_TAIL(&timer_queue, new_tm, timer_node);
    _timer_count++;
}

static void timer_remove(timer_t *tm) {
	TIMER_OFF_QUEUE(tm);
	TAILQ_REMOVE(&timer_queue, tm, timer_node);
    _timer_count--;
}

static void timer_add(timer_t *timer, tick_t delay, tick_t period, timer_func_t callback, void *arg) {
    tick_t now;

    TRACEK("timer %p, delay %u, period %u, callback %p, arg %p\n", timer, delay, period, callback, arg);

    ASSERT(callback != NULL);
    ASSERT(timer->magic == MAGIC_TIMER);

    if (TIMER_IS_ON_QUEUE(timer)) {
    	TRACEK("timer %p already set\n", timer);
    	timer_cancel(timer);
    }

    now = timer_ticks();
    timer->expire_time = now + delay;
    timer->period_time = period;
    timer->callback = callback;
    timer->arg = arg;

    TRACEK("scheduled time %u\n", timer->expire_time);

    WITHIN_CRITICAL() {
		timer_insert(timer);
    }
}

void timer_add_oneshot(timer_t *timer, tick_t delay, timer_func_t callback, void *arg) {
    if (delay <= 0) {
        delay = 1;
    }
    timer_add(timer, delay, 0, callback, arg);
}

void timer_add_period(timer_t *timer, tick_t period, timer_func_t callback, void *arg) {
    if (period <= 0) {
        period = 1;
    }
    timer_add(timer, period, period, callback, arg);
}

void timer_cancel(timer_t *timer) {
    ASSERT(timer->magic == MAGIC_TIMER);

    WITHIN_CRITICAL() {

		if (TIMER_IS_ON_QUEUE(timer)) {
			timer_remove(timer);
		}

		timer->period_time = 0;
		timer->callback = NULL;
		timer->arg = NULL;
	}
}

//called in interrupt
int timer_tick_isr(void) {
	int expired = 0;

	if(!tick_task_started) {
		return 0;
	}

    time_ticks++;

    WITHIN_CRITICAL() {
    	timer_t *timer;
        timer = TAILQ_FIRST(&timer_queue);
        if ((timer != 0) && (TIME_GE(time_ticks, timer->expire_time))) {
        	expired = 1;
        }
    }

    if(expired) {
    	task_wakeup(timer_tick_task);
    }

    //round-robin schedule
    task_timer_tick();

    return expired;
}

//called in task
static void timer_tick_call(void) {
    timer_t *timer;

    int __irqsr = cpu_irq_save();

    while ((timer = TAILQ_FIRST(&timer_queue)) != NULL) {
        ASSERT(timer && timer->magic == MAGIC_TIMER);

        if (TIME_LT(time_ticks, timer->expire_time))
            break;

        timer_remove(timer);

        cpu_irq_restore(__irqsr);

        int periodic = timer->period_time > 0;
        if(timer->callback) {
            TRACEK("timer %p firing callback %p, arg %p\n", timer, timer->callback, timer->arg);
            timer->callback(timer->arg);
        }

        __irqsr = cpu_irq_save();

        if (periodic && !TIMER_IS_ON_QUEUE(timer) && timer->period_time > 0) {
            timer->expire_time = time_ticks + timer->period_time;
            timer_insert(timer);
        }
    }

    cpu_irq_restore(__irqsr);
}

static void timer_tick_task(void) {
	tick_task_started = 1;
	while(1) {
		task_wait(timer_tick_task);
		timer_tick_call();
	}
}

//for task.c
static void wait_timeout_handler(void *task) {
	ASSERT(task);
	task_wakeup_the(task);
}

//if timer is enabled, timeout wait can be implemented
void task_wait_timeout(void *event, tick_t timeout) {
	if(timeout == WAIT_NONE) {
		return;
	}
	if(timeout == WAIT_FOREVER) {
		task_wait(event);
		return;
	}

	timer_t timer;
	timer_init(&timer);
	timer_add_oneshot(&timer, timeout, wait_timeout_handler, (void*)task_self());
    int time1 = timer_ticks();
	task_wait(event);
    int time2 = timer_ticks();
    //ASSERT(TIME_GE(time2, time1 + timeout));
	timer_cancel(&timer);

}

void task_sleep(tick_t ticks) {
	task_wait_timeout(task_self(), ticks);
}

tick_t ms_to_tick(int ms) {
    tick_t tick;

    if (ms < 0) {
        tick = (tick_t)WAIT_FOREVER;
    } else {
        tick = TICK_PER_SECOND * (ms / 1000);
        tick += (TICK_PER_SECOND * (ms % 1000) + 999) / 1000;
    }

    return tick;
}

void task_msleep(int ms) {
    tick_t tick;

    tick = ms_to_tick(ms);

    task_sleep(tick);
}

void timer_lib_init(void) {
	TAILQ_INIT(&timer_queue);
	tick_task_started = 0;
	task_create((task_func_t)timer_tick_task, NULL, PRIO_HIGHEST, "timer_task");
}
