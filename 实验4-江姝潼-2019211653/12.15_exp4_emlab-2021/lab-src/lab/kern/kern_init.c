/*
 * kern_init.c
 *
 *      Author: ljp
 */
#include "kern_api.h"

void kern_test_init(void);

void kern_init(void) {
	trace_printf("\nkern init...\n");
  /* Initialize PendSV */
  cpu_task_init();
  /* Initialize task lib */
  task_lib_init();
  /* Initialize timer lib */
  timer_lib_init();
  /* Initialize Heap */
  heap_init();
  /* Initialize I/O subsystem lib */
  ios_device_init();
  ios_fd_init();
  /* Initialize shell */
  kshell_lib_init();
/* Initialize kernel unit tests */
  kern_test_init();
}

void kern_start(void) {
  /* Start scheduler */
	trace_printf("kern start...\n");
  task_yield();
}
