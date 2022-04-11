/*
 * kern_test.c
 *
 *      Author: ljp
 */


#include "ktest.h"
#include "kern_api.h"

#define MN 4
static int add_count[MN];
static task_t *add_taskid[MN];
static int add_total;
static volatile int stop = 0;
static mutex_t mtx;

static void adder_task(int n) {
    while(!stop) {
        add_count[n]++;
        mutex_take(&mtx);
        add_total++;
        mutex_give(&mtx);
    }
}

static void stop_task(void) {
    stop = 1;
}

void mutex_test_task(void) {
    add_total = 0;
    stop = 0;
    mutex_init(&mtx);

    task_lock();
    for (int i = 0; i < MN; i++) {
        add_count[i] = 0;
        add_taskid[i] = task_create_simple(adder_task, i);
    }
    timer_t timer;
    timer_init(&timer);
    timer_add_oneshot(&timer, TICK_PER_SECOND * 5, (timer_func_t)stop_task, 0);
    task_unlock();
    for (int i = 0; i < MN; i++) {
        task_join(add_taskid[i]);
    }
    int all = 0;
    for (int i = 0; i < MN; i++) {
        all += add_count[i];
    }
    KASSERT_INT_EQUAL(all, add_total);
}

KTEST_DEFINE_UNIT(test_mutex) {
    task_create_simple(mutex_test_task, 0);
}


////////////////////////////////////////////////
static sem_t sem;
static const int sem_total_its = 10000;
static const int sem_task_max_its = 1000;
static const int sem_start_value = 10;
static int sem_remaining_its = 0;
static int sem_tasks = 0;
static mutex_t sem_test_mutex;

static int rand(void) {
    static unsigned int randseed = 12345;
    return (randseed = randseed * 1664525 + 1013904223);
}

static inline int atomic_add(volatile int *ptr, int val) {
    WITHIN_CRITICAL() {
        *ptr = *ptr + val;
    }
    return *ptr;
}

static int sem_producer(void *unused) {
	(void)unused;
    trace_printf("sem producer %p starting up, running for %d iterations\n", task_self(), sem_total_its);

    for (int x = 0; x < sem_total_its; x++) {
        sem_give(&sem);
    }

    return 0;
}

static int sem_consumer(void *unused) {
	(void)unused;
    unsigned int iterations = 0;

    mutex_take(&sem_test_mutex);
    if (sem_remaining_its >= sem_task_max_its) {
        iterations = rand();
        iterations %= sem_task_max_its;
    } else {
        iterations = sem_remaining_its;
    }
    sem_remaining_its -= iterations;
    mutex_give(&sem_test_mutex);

    trace_printf("sem consumer %p starting up, running for %u iterations\n", task_self(), iterations);
    for (unsigned int x = 0; x < iterations; x++)
        sem_take(&sem, WAIT_FOREVER);
    trace_printf("sem consumer %p done\n", task_self());
    atomic_add(&sem_tasks, -1);
    return 0;
}

static int sem_test(void) {
    sem_init(&sem, sem_start_value);
    mutex_init(&sem_test_mutex);

    sem_remaining_its = sem_total_its;
    while (1) {
        mutex_take(&sem_test_mutex);
        if (sem_remaining_its) {
            task_create_simple(&sem_consumer, NULL);
            atomic_add(&sem_tasks, 1);
        } else {
            mutex_give(&sem_test_mutex);
            break;
        }
        mutex_give(&sem_test_mutex);
    }

    task_create_simple(&sem_producer, NULL);

    while (sem_tasks)
        task_yield();

    if (sem.count == sem_start_value)
        trace_printf("sem tests successfully complete\n");
    else
        trace_printf("sem tests failed: %d != %d\n", sem.count, sem_start_value);

    return 0;
}

KTEST_DEFINE_UNIT(test_sem) {
    sem_test();
}

static void kern_testcase(void)
{
    KTEST_RUN_UNIT(test_mutex);
    KTEST_RUN_UNIT(test_sem);
}


void kern_test(void)
{
	KTEST_DEFINE_SUITE(kern_testsuite,
		KTEST_DEFINE_CASE_SIMPLE_BODY(kern_testcase)
	);

	KTEST_RUN_SUITE(kern_testsuite);
}


//typedef void (*sketch_func_t)(void);
//
//typedef struct sketch_data {
//    sketch_func_t setup;
//    sketch_func_t loop;
//} sketch_data_t;
//
//static void sketch_task(sketch_data_t *data) {
//    ASSERT(data);
//    if(data->setup) {
//        (*data->setup)();
//    }
//    while(1) {
//        (*data->loop)();
//    }
//}
//
//void start_sketch(sketch_func_t setup, sketch_func_t loop) {
//     sketch_data_t data = {
//         setup,
//         loop
//     };
//     task_create_simple(sketch_task, &data);
//}

void kern_test_init(void) {
    kshell_cmd_add(kern_test, "ktest", "kern unit test");
}

