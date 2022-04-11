/*
 *      Author: ljp
 */

#include <stdio.h>
#include <stdarg.h>

#include "board_cm.h"
#include "task_api.h"
#include "kern_port.h"

//#define SVC_CALL() __asm volatile ("svc 0")

#define MMIO32(addr)		(*(volatile unsigned int *)(addr))
/** ICSR: Interrupt Control State Register */
#define SCB_ICSR				MMIO32(0xE000ED04)
//System Handler Priority Register 3 (PendSV).
#define SHPR3_PRI_14			MMIO32(0xE000ED22)
/** PENDSVSET: PendSV set-pending bit */
#ifndef SCB_ICSR_PENDSVSET
#define SCB_ICSR_PENDSVSET		(1 << 28)
#endif
//PendSV priority level (lowest).
#define PRI_LVL_PENDSV		(0xFF)

static void task_error(void)
{
	cpu_irq_save();
	trace_printf("task %s stack overfow? die...\r\n", current_task->name);
	while(1);
}

//fill task's init stack
task_stack_frame_t* cpu_task_stack_prepair(stack_t *stack_top, task_func_t entry,
		void *arg) {
	task_stack_frame_t *frame = (task_stack_frame_t*) ((unsigned int) stack_top
			& ~(8 - 1));					//align to double word
	frame--;

	//context saved & restore by hardware exception
	frame->xPSR = 0x01000000u;	//initial xPSR: EPSR.T = 1, Thumb mode
	frame->PC = ((reg_t) entry & ~(2 - 1));	//Entry Point. UNPREDICTABLE if the new PC not halfword aligned
	frame->LR = (reg_t)task_error;//(unsigned int)-1;		//invalid caller
	frame->R12 = 0x12121212u;
	frame->R3 = 0x03030303u;
	frame->R2 = 0x02020202u;
	frame->R1 = 0x01010101u;
	frame->R0 = (reg_t) arg;

	//context saved & restore by software (pendsv handler)
	frame->R11 = 0x11111111u;
	frame->R10 = 0x10101010u;
	frame->R9 = 0x09090909u;
	frame->R8 = 0x08080808u;
	frame->R7 = 0x07070707u;
	frame->R6 = 0x06060606u;
	frame->R5 = 0x05050505u;
	frame->R4 = 0x04040404u;

	frame->exc_return = 0xFFFFFFFDu; // initial EXC_RETURN begin state: Thread mode +  non-floating-point + PSP
	frame->control = 0x2;//0x3;		// initial CONTROL : privileged, PSP, no FP

	return frame;
}

void syscall_svc(void) {
	__asm volatile ("svc 0");
}

void syscall_pendsv(void) {
	SCB_ICSR = SCB_ICSR_PENDSVSET;
}

void cpu_schedule_pend(void) {
	syscall_pendsv();
}

void cpu_task_init(void)
{
	HAL_NVIC_SetPriority(PendSV_IRQn, 15, 15);
//	SHPR3_PRI_14 = PRI_LVL_PENDSV;
}

void HAL_SYSTICK_Callback(void)
{
//	HAL_IncTick();
	int timer_tick_isr(void);
	timer_tick_isr();
}

