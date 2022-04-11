#ifdef STM32F407xx
#include "stm32f4xx_hal.h"
#endif
#ifdef STM32F10X_MD
#include "stm32f1xx_hal.h"
#endif

#include "kern/task_api.h"
#include "kern/kern_port.h"
#include "syscall.h"

/*
 CONTROL[1]
 Writeable only when the processor is in thread mode and privileged state (CONTROL[0]=0).
 =0 In handler mode - MSP is selected. No alternate stack pointer possible for handler mode.
 =0 In thread mode - Default stack pointer MSP is used.
 =1 In thread mode - Alternate stack pointer PSP is used.

 CONTROL[0] [not writeable for Cortex-M0]
 Writeable only when the processor is in privileged state.
 Can be used to switch the processor to user state (thread mode).
 Once in user state, trigger an interrupt and change the state to privileged in the exception handler (the only way).
 =0 In thread mode and privileged state.
 =1 In thread mode and user state.
 */

extern uint32_t __get_CONTROL(void);
extern void __set_CONTROL(uint32_t control);

//CONTROL_nPRIV_Msk = 0x1
#define THREAD_MODE_PRIVILEGED      0x00   /* Thread mode has privileged access */
#define THREAD_MODE_UNPRIVILEGED    0x01   /* Thread mode has unprivileged access */

static void switch_to_privileged_mode() {
	/* enable privileeged mode */
	__set_CONTROL(__get_CONTROL() & ~CONTROL_nPRIV_Msk);
	/* Execute ISB instruction to flush pipeline as recommended by Arm */
	__asm("isb");
	//__ISB();
}

static void switch_to_user_mode() {
	/* enable user mode */
	__set_CONTROL(__get_CONTROL() | CONTROL_nPRIV_Msk);
	/* Execute ISB instruction to flush pipeline as recommended by Arm */
	__asm("isb");
	//__ISB();
}

/* Check Thread mode privilege status */
#define check_privilege_mode() trace_printf("%d %s\n", __LINE__, (__get_CONTROL() & CONTROL_nPRIV_Msk) ? "UNPRIVILEGED": "PRIVILEGED")

void exec_privilege_action() {
	SysTick->LOAD = SysTick->LOAD;
}

static void priv_test_task(void) {
	task_sleep(200);//wait previous test to be done
	
	check_privilege_mode();

	/* Switch Thread mode from privileged to unprivileged #################*/
	switch_to_user_mode();
	check_privilege_mode();
	/* Unprivileged access mainly affect ability to:
	 - Use or not use certain instructions such as MSR fields
	 - Access System Control Space (SCS) registers such as NVIC and SysTick */

	/* Switch back Thread mode from unprivileged to privileged #################*/
	/* Try to switch back Thread mode to privileged (Not possible, this can be done only in Handler mode) */
	switch_to_privileged_mode();
	check_privilege_mode();

	//uncomment this will cause HardFault
	//exec_privilege_action();

	/* Generate a system call exception, and in the ISR switch back Thread mode to privileged #################*/
	//TODO
//	raise_privilege();
	check_privilege_mode();

	exec_privilege_action();
}

void sys_raise_privilege(void) {
	switch_to_privileged_mode();
}

void fork_test_task(void) {
	int stack_val = 0xa5;
	int pid = fork();
	if(pid < 0) {	/* error occurred */
		trace_printf("Fork Failed!\n");
		return;
	} else if(pid == 0) { /* child process */
		// task_set_priority(task_priority(0) + 1);
		task_lock();
		trace_printf("I'm the child [%p, %s]. stack_val *%p == %x\n", task_self(), task_self()->name, &stack_val, stack_val);
		task_unlock();
	} else {
		task_lock();
		trace_printf("I'm the parent [%p, %s]. stack_val *%p == %x. Child is %p\n", task_self(), task_self()->name, &stack_val, stack_val, (void*)pid);
		task_unlock();
		/* parent will wait for the child to complete */
         task_wait((void *)pid);
       /* When the child is ended, then the parent will continue to execute its code */
         trace_printf("Child Complete \n");
	}
}

void project1_main(void) {
#if 1
	//test svc 0 - system call
	system_call((system_call_func_t) &trace_printf,
			(void*) "\nsyscall trace_printf %d %d\n", (void*) 1, (void*) 2);

	//test svc 1  - task fork
	task_create_simple(fork_test_task, 0);

	//test svc 2 - just a printf test
	int ret = call_test("I am a syscall!");
	trace_printf("ret = %d\n", ret);
	ret = call_test(NULL);
	trace_printf("ret = %d\n", ret);
#endif

	//YOUR syscall
	task_create_simple(priv_test_task, 0);
}
