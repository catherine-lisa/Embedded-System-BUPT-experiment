#if defined ( __GNUC__ )

/*
 * syscall.s
 *
 *      Author: ljp
 */
#include "syscall.h"

.text
.align 2
.thumb
.syntax unified
.cpu cortex-m4
.fpu softvfp


#define SYSCALL(func_name) 		\
  .globl func_name; 			\
  func_name: 					\
    push {r12, lr} ;			\
    mov r12, #SYS_##func_name ;	\
    svc 0 ;						\
    nop	;						\
    pop {r12, lr} ;				\
    bx lr;

SYSCALL(system_call)
SYSCALL(fork)
SYSCALL(call_test)
SYSCALL(raise_privilege)

#endif /* __GNUC__ */
