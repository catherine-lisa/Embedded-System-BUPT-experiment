/*
 * task_port_cm_gnu.c
 *
 *      Author: ljp
 */

#if defined ( __GNUC__ )
#include <stdint.h>

// override of some routines
void cpu_irq_enable(void) {
    __asm__ volatile("cpsie i" : : : "memory");
}

void cpu_irq_disable(void) {
    __asm__ volatile("cpsid i" : : : "memory");
}

int cpu_irq_save(void) {
    unsigned int priMask;
	__asm volatile (
    "MRS     R0, PRIMASK \n"
    "CPSID   I \n"
    "BX      LR \n"
	:"=r"(priMask) : : "memory");
	return priMask;
}

void  cpu_irq_restore(int priMask) {
	__asm volatile (
    "MSR     PRIMASK, R0 \n"
    "BX      LR \n"
	: :"r" (priMask) : "memory");
}

int cpu_irq_disabled(void) {
    unsigned int state;

    __asm__ volatile("mrs %0, primask" : "=r"(state));
    state &= 0x1;

    return !!state;
}

#define IPSR_ISR_Msk                       (0x1FFUL)                  /*!< IPSR: ISR Mask */
int cpu_in_irq(void) {
	unsigned int ipsr;
    __asm volatile ("MRS %0, ipsr" : "=r" (ipsr) );
    return (ipsr & IPSR_ISR_Msk);
}

uint32_t __get_PRIMASK(void)
{
    uint32_t result=0;

    asm volatile ("MRS %0, primask" : "=r" (result) );
    return(result);
}
void __set_PRIMASK(uint32_t priMask)
{
    asm volatile ("MSR primask, %0" : : "r" (priMask) );
}

uint32_t __get_CONTROL(void)
{
    uint32_t result=0;

    asm volatile ("MRS %0, control" : "=r" (result) );
    return(result);
}
void __set_CONTROL(uint32_t control)
{
    asm volatile ("MSR control, %0" : : "r" (control) );
}

void PendSV_Handler(void) {
	__asm volatile (
//SAVE CPU CONTEXT
			" CPSID   I \n"//disable interrupt
			" tst lr, #4 \n"//which stack we were using?
			" ite eq \n"
			" mrseq r0, msp \n"
			" mrsne r0, psp \n"

			" ldr   r1, =current_task \n"
			" cmp	r1, #0 \n"
			" beq 	_pendsv_handler_nosave \n"

			//hareware saved xpsr pc lr r12 r3-r0
			" stmdb r0!, {r4-r11} \n"//software save r11-r4

			" mrs r2, control \n"
			" mov r3, lr \n"
			" stmdb r0!, {r2 - r3} \n"//software save lr & control

			" tst lr, #4 \n"//are we on the same stack? ajust sp if yes
			" it eq \n"
			" moveq sp, r0 \n"

			" _pendsv_handler_nosave: \n"
//call C to help
			" bl task_switch_sp \n"//stack is fine, let's call into c

//RESTORE CPU CONTEXT
			" ldmia r0!, {r2, r3, r4-r11} \n"//new stack is in R0, r2=new control, r3=new exc_return
			" mov lr, r3 \n"
			" msr control, r2 \n"
			" ISB \n"

			" tst lr, #4 \n"
			" ite eq \n"
			" msreq msp, r0 \n"//return stack = MSP or
			" msrne psp, r0 \n"//return stack = PSP

			" CPSIE   I \n"//enable interrupt

			" bx LR \n"//return to new task from trap. PC & PSR popped out by bx lr
	);
}

#endif /* __GNUC__ */
