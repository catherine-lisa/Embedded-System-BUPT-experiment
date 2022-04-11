/*
 * msgq.c
 *
 *      Author: ljp
 */
#include "kern_api.h"

void msgq_init(struct msgq *msgq, char *buffer, int msg_size, unsigned int max_msgs) {
    msgq->magic = MAGIC_MSGQ;
	msgq->msg_size = msg_size;
	msgq->max_msgs = max_msgs;
	msgq->free_msgs = max_msgs;
	msgq->buffer_start = buffer;
	msgq->buffer_end = buffer + (msg_size * max_msgs);
	msgq->read_ptr = buffer;
	msgq->write_ptr = buffer;
}


// static int msgq_is_empty(struct msgq *msgq) {
//     return (msgq->free_msgs >= msgq->max_msgs);
// }

// static int msgq_is_full(struct msgq *msgq) {
//     return (msgq->free_msgs <= 0);
// }

//put a message into msgq, wait 'timeout' ticks if msgq is full
int msgq_put_wait(struct msgq *msgq, const void *msg, tick_t timeout) {
	ASSERT(msgq->magic == MAGIC_MSGQ);
	ASSERT(!((timeout != WAIT_NONE) && cpu_in_irq()));

	tick_t expire_time = timer_ticks();
	if (timeout != WAIT_FOREVER) {
		expire_time += timeout;
	}

    //try to put a msg until success or timeout
	while (1) {
		CPU_CRITICAL_ENTER();

		if (msgq->free_msgs > 0) {	//have space, put msg
			/* put message in queue */
			(void)memcpy(msgq->write_ptr, msg, msgq->msg_size);
			msgq->write_ptr += msgq->msg_size;
			if (msgq->write_ptr == msgq->buffer_end) {
				msgq->write_ptr = msgq->buffer_start;
			}
			--msgq->free_msgs;
            if((msgq->free_msgs + 1) == msgq->max_msgs) {
                task_wakeup_all(&msgq->read_ptr);
            }
			CPU_CRITICAL_EXIT();
			return 0;
		}

		//no space, no wait
		if (timeout == WAIT_NONE) {
			CPU_CRITICAL_EXIT();
			return -1;	//try failed
		}

		//no space, wait space until available
		if (timeout == WAIT_FOREVER) {
			task_wait(&msgq->write_ptr);
			CPU_CRITICAL_EXIT();
			continue; //waken up, try again
		}

		ASSERT(msgq->free_msgs <= 0);
		//no space, wait until space available or timeout
		tick_t now  = timer_ticks();
		if (TIME_GE(now, expire_time)) {
			CPU_CRITICAL_EXIT();
			return -1;	//if timeout and no space, return error
		} else { //no space, no timeout, wait to wakeup
			task_wait_timeout(&msgq->write_ptr, expire_time - now);
			CPU_CRITICAL_EXIT();//ensure task_wait_timeout
			continue;   //timeout or space available? just try again
		}
	}
}

//get a message from msgq, wait 'timeout' ticks if msgq is empty
int msgq_get(struct msgq *msgq, void *msg, tick_t timeout) {
    ASSERT(msg != NULL);
	ASSERT(msgq->magic == MAGIC_MSGQ);
	ASSERT(!((timeout != WAIT_NONE) && cpu_in_irq()));

	tick_t expire_time = timer_ticks();
	if (timeout != WAIT_FOREVER) {
		expire_time += timeout;
	}

    //try to put a msg until success or timeout
	while (1) {
		CPU_CRITICAL_ENTER();

		if (msgq->free_msgs < msgq->max_msgs) {	//have msg, get it
            /* take first available message from queue */
            (void)memcpy(msg, msgq->read_ptr, msgq->msg_size);
            msgq->read_ptr += msgq->msg_size;
            if (msgq->read_ptr == msgq->buffer_end) {
                msgq->read_ptr = msgq->buffer_start;
            }
			++msgq->free_msgs;
            if((msgq->free_msgs - 1) == 0) {
                task_wakeup_all(&msgq->write_ptr);
            }
			CPU_CRITICAL_EXIT();
			return 0;
		}

		//no msg, no wait
		if (timeout == WAIT_NONE) {
			CPU_CRITICAL_EXIT();
			return -1;	//try failed
		}

		//no msg, wait until msg available
		if (timeout == WAIT_FOREVER) {
			task_wait(&msgq->read_ptr);
			CPU_CRITICAL_EXIT();
			continue; //waken up, try again
		}

		ASSERT(msgq->free_msgs >= msgq->max_msgs);
		//no msg, wait until msg available or timeout
		tick_t now  = timer_ticks();
		if (TIME_GE(now, expire_time)) {
			CPU_CRITICAL_EXIT();
			return -1;	//if timeout and no msg, return error
		} else { //no msg, no timeout, wait to wakeup
			task_wait_timeout(&msgq->read_ptr, expire_time - now);
			CPU_CRITICAL_EXIT();//ensure task_wait_timeout
			continue;   //timeout or msg available? just try again
		}
	}
}

