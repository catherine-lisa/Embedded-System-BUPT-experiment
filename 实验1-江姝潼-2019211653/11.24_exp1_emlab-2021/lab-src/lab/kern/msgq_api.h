/*
 * msgq.h
 *
 *      Author: ljp
 */

#ifndef MSGQ_API_H_
#define MSGQ_API_H_

#include "kern_def.h"

#define MAGIC_MSGQ (0x4d534751) // "MSGQ"

struct task_t;
typedef struct msgq {
	unsigned int magic;
	/* Message size */
	int msg_size;
	/* Maximal number of messages */
	unsigned int max_msgs;
	/* Number of free messages */
	unsigned int free_msgs;
	/* Start of message buffer */
	char *buffer_start;
	/* End of message buffer */
	char *buffer_end;
	/* Read pointer */
	char *read_ptr;
	/* Write pointer */
	char *write_ptr;
} msgq_t;

#define MSGQ_INITIALIZER(q_buffer, q_msg_size, q_max_msgs) \
	{ \
	.magic = MAGIC_MSGQ, \
	.msg_size = q_msg_size, \
	.max_msgs = q_max_msgs, \
	.free_msgs = q_max_msgs, \
	.buffer_start = q_buffer, \
	.buffer_end = q_buffer + ((q_max_msgs) * (q_msg_size)), \
	.read_ptr = q_buffer, \
	.write_ptr = q_buffer, \
	}

#define DEFINE_MSGQ(q_name, q_msg_size, q_max_msgs)		\
	static char __msgq_ringbuf_##q_name[(q_msg_size) * (q_max_msgs)];	\
	q_name = MSGQ_INITIALIZER(__msgq_ringbuf_##q_name, (q_msg_size), (q_max_msgs))

void msgq_init(struct msgq *msgq, char *buffer, int msg_size, unsigned int max_msgs);
int msgq_put_wait(struct msgq *msgq, const void *msg, tick_t timeout);
static inline int msgq_put(struct msgq *msgq, const void *msg) {
	return msgq_put_wait(msgq, msg, WAIT_NONE);
}
int msgq_get(struct msgq *msgq, void *msg, tick_t timeout);
static inline int msgq_free_msgs(struct msgq *msgq) {
	return msgq->free_msgs;
}

#endif /* MSGQ_API_H_ */
