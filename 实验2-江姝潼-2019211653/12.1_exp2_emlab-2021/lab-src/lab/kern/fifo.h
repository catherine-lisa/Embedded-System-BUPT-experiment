/*
 *      Author: ljp
 */

#ifndef FIFO_H_
#define FIFO_H_

#define MOD(x, y) (x) >= (y) ?  (x) - (y): (x)

#define DEFINE_FIFO(fifo_type_name, fifo_name, data_type, data_num) \
		struct fifo_type_name { \
			int capacity; \
			volatile int count; \
			int head; \
			data_type data[data_num]; \
		} fifo_name = {data_num, 0, 0, {0}};


#define FIFO_COUNT(fifo) ((fifo)->count)
#define FIFO_CAPACITY(fifo) ((fifo)->capacity)
#define FIFO_EMPTY(fifo) (FIFO_COUNT(fifo) <= 0)
#define FIFO_FULL(fifo) (FIFO_COUNT(fifo) >= FIFO_CAPACITY(fifo))

#define FIFO_RESET(fifo) do {(fifo)->count = 0; (fifo)->head = 0;}while(0)

#define FIFO_PUSH(fifo, data_ptr) 	do { \
		if (!FIFO_FULL(fifo)) { \
			(fifo)->data[MOD(((fifo)->head + (fifo)->count), (fifo)->capacity)] = *(data_ptr); \
			(fifo)->count++; \
		} else { \
			trace_printf("fifo full!"); \
		} \
} while(0)

#define FIFO_POP(fifo, data_ptr) 	do { \
		if (!FIFO_EMPTY((fifo))) { \
			*(data_ptr) = (fifo)->data[(fifo)->head]; \
			(fifo)->head = MOD((fifo)->head + 1, (fifo)->capacity); \
			(fifo)->count--; \
		} else { \
			trace_printf("(fifo) empty!"); \
		} \
} while(0)

#define FIFO_PUSH_VAR(fifo, var) 	do { \
		if (!FIFO_FULL(fifo)) { \
			(fifo)->data[MOD(((fifo)->head + (fifo)->count), (fifo)->capacity)] = (var); \
			(fifo)->count++; \
		} else { \
			trace_printf("fifo full!"); \
		} \
} while(0)

#define FIFO_POP_VAR(fifo, var) 	do { \
		if (!FIFO_EMPTY((fifo))) { \
			(var) = (fifo)->data[(fifo)->head]; \
			(fifo)->head = MOD((fifo)->head + 1, (fifo)->capacity); \
			(fifo)->count--; \
		} else { \
			trace_printf("(fifo) empty!"); \
		} \
} while(0)




#endif /* FIFO_H_ */
