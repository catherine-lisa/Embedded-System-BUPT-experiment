/*
 * kern_config.h
 *
 *      Author: ljp
 */

#ifndef KERN_CONFIG_H_
#define KERN_CONFIG_H_

#define CONFIG_TASK_STACK_SIZE 		400 	/* Size of task stacks in words */
#define CONFIG_STATIC_TASK_LIMIT 	8  		/* Max number of static tasks in task pool */
#define CONFIG_TASK_TLS_SIZE 		1 	    /* Size of task local storage entries */

#define CONFIG_HEAP_SIZE 			1024		/* Size of heap in words */

#define CONFIG_PIPE_SIZE   			128 	/* Size of pipe size in bytes */

#define CONFIG_MAX_CMD              16      /* Max shell commands in cmd table */

#define TICK_PER_SECOND 			1000	/* Kernel heartbeat HZ */

#define CONFIG_DEBUG                1       /* Enable kernel debug */

#endif /* KERN_CONFIG_H_ */
