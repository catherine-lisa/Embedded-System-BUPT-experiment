/*
 * syscall.h
 *
 *      Author: ljp
 */

#ifndef SYSCALL_H_
#define SYSCALL_H_

#define SVC_CALL(nr) __asm volatile ("svc "#nr)

// System call numbers
#define SYS_system_call         			0
#define SYS_fork           			        1
#define SYS_call_test           			2

#ifndef __ASSEMBLER__
//System call User side functions
//user functions defined in syscall.S. foo(...) will call sys_foo(...) in kernel mode
typedef int (*system_call_func_t)(void*, void*, void*);
extern int system_call(system_call_func_t func, void *arg1, void *arg2, void *arg3);
extern int fork(void);
extern int call_test(void*);
#endif

#endif /* SYSCALL_H_ */
