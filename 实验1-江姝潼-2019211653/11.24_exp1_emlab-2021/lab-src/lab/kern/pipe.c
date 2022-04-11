/*
 * pipe.c
 *
 *      Author: ljp
 */
#include "kern_api.h"

void pipe_init(pipe_t *pipe) {
	pipe->nread = 0;
	pipe->nwrite = 0;
}

//buf[0..n] -> pipe, wait if do_block and pipe full
int pipe_write(pipe_t *pi, char *buf, int n, int do_block) {
	int i = 0;
	ASSERT(!(do_block && cpu_in_irq()));

	CPU_CRITICAL_ENTER();

	while (i < n) {
		if (pi->nwrite == pi->nread + CONFIG_PIPE_SIZE) { //pipe_write-full
			task_wakeup(&pi->nread);
			if (do_block) {
				task_wait(&pi->nwrite);
				CPU_CRITICAL_EXIT();
				CPU_CRITICAL_ENTER();
			} else {
				break;
			}
		} else {
			pi->data[pi->nwrite++ % CONFIG_PIPE_SIZE] = buf[i];
			i++;
		}
	}

	task_wakeup(&pi->nread);

	CPU_CRITICAL_EXIT();

	return i;
}

//pipe -> buf[0..n], wait if do_block and pipe empty
int pipe_read(pipe_t *pi, char* buf, int n, int do_block) {
	int i;
	ASSERT(!(do_block && cpu_in_irq()));

	CPU_CRITICAL_ENTER();

	while (pi->nread == pi->nwrite) {  //pipe-empty
		if (do_block) {
			task_wait(&pi->nread); //pipe_read-task_wait
			CPU_CRITICAL_EXIT();
			CPU_CRITICAL_ENTER();
		} else  {
			break;
		}
	}
	for (i = 0; i < n; i++) {  //pipe_read-copy
		if (pi->nread == pi->nwrite)
			break;
		buf[i] = pi->data[pi->nread++ % CONFIG_PIPE_SIZE];
	}

	task_wakeup(&pi->nwrite);  //pipe_read-task_wakeup

	CPU_CRITICAL_EXIT();

	return i;
}
