/*
 * pipe_api.h
 *
 *      Author: ljp
 */

#ifndef PIPE_API_H_
#define PIPE_API_H_

#include "kern_config.h"

#define PIPE_INITIALIZER {0, 0, {0}}
#define DEFINE_PIPE(pipe) pipe_t pipe = PIPE_INITIALIZER

//#define CONFIG_PIPE_SIZE (1 << 8)

typedef struct pipe_t {
	unsigned int nread;     // number of bytes read
	unsigned int nwrite;    // number of bytes written
	char data[CONFIG_PIPE_SIZE];
} pipe_t;

void pipe_init(pipe_t *pipe);
int pipe_write(pipe_t *pi, char *buf, int n, int do_block);
int pipe_read(pipe_t *pi, char* buf, int n, int do_block);

#endif /* PIPE_API_H_ */
