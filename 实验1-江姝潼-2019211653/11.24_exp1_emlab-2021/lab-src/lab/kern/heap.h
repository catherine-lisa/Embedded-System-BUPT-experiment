/*
 * heap.h
 *
 *      Author: ljp
 */

#ifndef HEAP_H_
#define HEAP_H_


void *kmalloc(unsigned int nbytes);
void kfree(void *ptr);
void heap_init(void);

#endif /* HEAP_H_ */
