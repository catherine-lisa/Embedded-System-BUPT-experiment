/*
 * syscall.c
 *
 *      Author: ljp
 */
#include <stdint.h>
#include <assert.h>
#include "kern/debug.h"
#include "kern/kern_port.h"
#include "syscall.h"
#include "syscall.h"

/*
 * How to add a system call function foo(): foo() -> SYS_foo -> sys_foo()
 * 1.declare foo() in syscall.h
 * 2.define SYS_foo num in syscall.h. 
 * 3.add SYS_foo and SYSCALL foo in syscall_ccarm.S
 * 4.add __SYSCALL_ENTRY(foo) in sys_call_table, declare sys_foo() in syscall.c. SYS_foo num is the index of sys_call_table
 * 5.define sys_foo() somewhere!
 */

int sys_system_call(int (*call_func)(void*, void*, void*), void *arg1, void *arg2, void *arg3) {
	return (*call_func)(arg1, arg2, arg3);
}

int sys_call_test(char *str) {
	if (str) {
		trace_printf("Hello, %s\n", str);
		return 0;
	} else {
		trace_printf("Hello, are u OK?\n");
		return -1;
	}
}

int sys_fork(void) {
	return task_fork();
}

__attribute__((weak)) void sys_raise_privilege(void) {

}

//static int sys_ni_syscall(void) {
//	return -1;
//}

/*
 * system call handler table
 */
//System call handler function type
typedef uintptr_t (*syscall_handler_t)(uintptr_t arg1, uintptr_t arg2,
		uintptr_t arg3, uintptr_t arg4, void *stack_frame);

#define __NR_SYSCALL_MAX 256
#define __SYSCALL_ENTRY(sym) [SYS_##sym] = (syscall_handler_t)(void*)&sys_##sym,

//syscall table
const syscall_handler_t sys_call_table[] = {
		__SYSCALL_ENTRY(system_call)
		__SYSCALL_ENTRY(fork)
		__SYSCALL_ENTRY(call_test)
};

const int sys_call_count = NELEM(sys_call_table);
STATIC_ASSERT(NELEM(sys_call_table) <= 257);

/*
 * system call handler entry
 */

//registers saved by hardware
#if 0
typedef struct hw_stack_frame_t {
	//args r0 - r3
	uintptr_t R0;	//hardware saved R0~R3,R12,LR,PC,xPSR
	uintptr_t R1;
	uintptr_t R2;
	uintptr_t R3;
	uintptr_t R12;   //svc call function
	uintptr_t LR;	//LR=r14
	uintptr_t PC_RETURN;	//return address, PC=r15
	uintptr_t xPSR;
} hw_stack_frame_t;
#endif

uintptr_t SVC_Handler_dispatch(task_stack_frame_t *sf) {
	unsigned int svc_number;
	// hw_stack_frame_t *sf = (hw_stack_frame_t*) svc_args;
	uintptr_t retVal = (uintptr_t)-1;

	/*
	 * Stack contains:
	 * r0, r1, r2, r3, r12, r14, the return address(pc) and xPSR
	 * First argument (r0) is svc_args[0]
	 */
	//svc_number = ((char*) svc_args[6])[-2];	//pc-2 == svc
	svc_number = sf->R12;
	
	if (svc_number >= sys_call_count) {
		trace_printf("invalid svc number %d\n", svc_number);
		sf->R0 = (uintptr_t)-1;
		return (uintptr_t)-1;
	}

	syscall_handler_t sc_handler = sys_call_table[svc_number];
	if (sc_handler) {
		retVal = (*sc_handler)(sf->R0, sf->R1, sf->R2, sf->R3, sf);	//call syscall implemention
	} else {
		trace_printf("syscall not installed for svc number %d\n", svc_number);
	}

	sf->R0 = retVal;	//the return value of user function is svc_args[0]
	return retVal;
}

//https://developer.arm.com/documentation/ka004005/latest
#if defined (__CC_ARM)          /* ARM Compiler */

__asm void SVC_Handler(void)
{
  IMPORT SVC_Handler_dispatch
  IMPORT current_task
  PRESERVE8
  
  TST 	lr, #4
  ITE 	EQ
  MRSEQ r0, MSP
  MRSNE r0, PSP

    //hareware saved xpsr pc lr r12 r3-r0
    stmdb r0!, {r4-r11}    //software save r11-r4

    mrs r2, control
    mov r3, lr
    stmdb r0!, {r2 - r3}    //software save lr & control

    tst lr, #4    //are we on the same stack? adjust sp if yes
    it eq
    moveq sp, r0

    mov32 r1, current_task
	ldr	r2, [r1]
	cmp	r2, #0
    beq _svc_dispatch
	str r0, [r2]	//save sp to current_task->context->sp if current_task != NULL

_svc_dispatch
  push {r0, lr}
  ;PUSH  {R4, LR}                 ; Save Registers
  ;LDM   R0, {R0-R3, R12}         ; Read R0-R3,R12 from stack
  BL SVC_Handler_dispatch         ; Call SVC Function
  ;POP   {R4, PC}                 ; RETI
  pop {r0 ,lr}

    //RESTORE CPU CONTEXT
    ldmia r0!, {r2, r3, r4-r11}    //new stack is in R0, r2=new control, r3=new exc_return
    mov lr, r3
    ISB

    tst lr, #4
    ite eq
    msreq msp, r0    //return stack = MSP or
    msrne psp, r0    //return stack = PSP

	bx LR
}

#elif defined (__GNUC__)        /* GNU Compiler */

void SVC_Handler(void) {
	__asm volatile (
			".global SVC_Handler_dispatch \n"
			"TST lr, #4 \n"
			"ITE EQ \n"
			"MRSEQ r0, MSP \n"
			"MRSNE r0, PSP \n"
			"PUSH  {R4, LR} \n"      		//Save Registers
			"LDM   R0, {R0-R3, R12} \n"		//Read R0-R3,R12 from stack
			"BL SVC_Handler_dispatch\n"		//R0 = sp
			"POP   {R4, PC} \n"				//RETI
	);
}
#endif	/* __CC_ARM/__GNUC__ */
