/*
 * kern_def.h
 *
 *      Author: ljp
 */

#ifndef KERN_DEF_H_
#define KERN_DEF_H_

#include <stddef.h>

#define K_TRUE                          1
#define K_FALSE                         0

#define K_OK                            (0)
#define K_ERROR                         (-1)
#define K_ETIMEOUT                      (-2)
#define K_EINVAL                        (-3)
#define K_ENOMEM                        (-4)

typedef long                            k_base_t;
typedef unsigned long                   k_ubase_t;

typedef k_base_t                 		status_t;
typedef unsigned int                    stack_t;
typedef unsigned int                    reg_t;
typedef k_ubase_t                   	tick_t;


#define WAIT_FOREVER (tick_t)(-1)
#define WAIT_NONE (tick_t)(0)

// number of elements in fixed-size array
#define NELEM(x) (sizeof(x)/sizeof((x)[0]))
#define FOR_EACH(i, array) for (size_t (i) = 0; (i) < NELEM(array); (i)++)

#define ARG_UNUSED(x) (void)(x)

#endif /* KERN_DEF_H_ */
