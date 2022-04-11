/*
 * task_port_cm_keil.c
 *
 *      Author: ljp
 */


#if   defined ( __CC_ARM )
#include "board_cm.h"
#include "task_api.h"
#include "kern_port.h"

void cpu_irq_enable(void) {
	__enable_irq();
}

void cpu_irq_disable(void) {
	__disable_irq();
}

__asm int cpu_irq_save(void) {
    MRS     R0, PRIMASK
    CPSID   I
    BX      LR
}

__asm void  cpu_irq_restore(int cpsr) {
    MSR     PRIMASK, R0
    BX      LR
}

int cpu_irq_disabled(void) {
    unsigned int state = __get_PRIMASK();

    state &= 0x1;

    return !!state;
}

#ifndef IPSR_ISR_Msk
#define IPSR_ISR_Msk                       (0x1FFUL)                  /*!< IPSR: ISR Mask */
#endif

int cpu_in_irq(void) {
    uint32_t ipsr = __get_IPSR();
    return (ipsr & IPSR_ISR_Msk);
}


__asm void PendSV_Handler(void) {
	extern current_task
    PRESERVE8
    //SAVE CPU CONTEXT
    CPSID   I    //disable interrupt
    tst lr, #4    //which stack we were using?
    ite eq
    mrseq r0, msp
    mrsne r0, psp

    mov32 r1, current_task
	ldr		r1, [r1]
	cmp		r1, #0
    beq 	_pendsv_handler_nosave

    //hareware saved xpsr pc lr r12 r3-r0
    stmdb r0!, {r4-r11}    //software save r11-r4

    mrs r2, control
    mov r3, lr
    stmdb r0!, {r2 - r3}    //software save lr & control

    tst lr, #4    //are we on the same stack? adjust sp if yes
    it eq
    moveq sp, r0


    //call C to help
_pendsv_handler_nosave
	bl __cpp(task_switch_sp)    //stack is fine, let's call into c, return r0 == new task's sp

    //RESTORE CPU CONTEXT
    ldmia r0!, {r2, r3, r4-r11}    //new stack is in R0, r2=new control, r3=new exc_return
    mov lr, r3
    msr control, r2
    ISB

    tst lr, #4
    ite eq
    msreq msp, r0    //return stack = MSP or
    msrne psp, r0    //return stack = PSP

    CPSIE   I    //enable interrupt

    bx LR    //return to new task from trap. PC & PSR popped out by bx lr
}

/*
void syscall_svc(void) {
	__asm
	{
		svc 0
	}
}
*/

#endif /* __CC_ARM */
