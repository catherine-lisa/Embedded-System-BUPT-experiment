/*
 * timer.h
 *
 *      Author: ljp
 */

#ifndef TIMER_API_H_
#define TIMER_API_H_

#include "kern_def.h"
#include "queue.h"

#define TIME_SUB(a, b) ((long)((a) - (b)))
#define TIME_GT(a, b) ((long)((a) - (b)) > 0)   //(b,a), a after b
#define TIME_LT(a, b) ((long)((a) - (b)) < 0)   //(a,b), a before b
#define TIME_GE(a, b) ((long)((a) - (b)) >= 0)  //[b,a)
#define TIME_LE(a, b) ((long)((a) - (b)) <= 0)  //(a, b]
#define TIME_IN_RANGE(a,b,c) (TIME_GE(a,b) &&  TIME_LE(a,c))    //[b, a, c]
#define TIME_IN_RANGE_OPEN(a,b,c) (TIME_GE(a,b) &&  TIME_LT(a,c))    //[b, a, c)


// typedef long tick_t;

typedef void (*timer_func_t)(void *arg);

typedef struct timer_t {
	unsigned int magic;
    TAILQ_ENTRY(timer_t) timer_node;

    tick_t expire_time;
    tick_t period_time;

    timer_func_t callback;
    void *arg;

    int flag;
} timer_t;



timer_t* timer_init(timer_t *timer);
void timer_add_oneshot(timer_t *timer, tick_t delay, timer_func_t callback, void *arg);
void timer_add_period(timer_t *timer, tick_t period, timer_func_t callback, void *arg);
void timer_cancel(timer_t *timer);

tick_t timer_ticks(void);

tick_t ms_to_tick(int ms);

void timer_lib_init(void);


#endif /* TIMER_API_H_ */
