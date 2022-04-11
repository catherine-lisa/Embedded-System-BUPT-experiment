/*
 * kern_port.h
 *
 *      Author: ljp
 */

#ifndef TASK_PORT_H_
#define TASK_PORT_H_

#include "kern_def.h"
#include "task_api.h"
#include "critical.h"

//context save/restore related
typedef struct task_stack_frame_t {
	//extra registers saved on task stack
	reg_t control;
	reg_t exc_return;	//=>r14

	reg_t R4;	//Remaining registers saved on task stack
	reg_t R5;
	reg_t R6;
	reg_t R7;
	reg_t R8;
	reg_t R9;
	reg_t R10;
	reg_t R11;

	reg_t R0;	//hardware saved R0~R3,R12,LR,PC,xPSR
	reg_t R1;
	reg_t R2;
	reg_t R3;
	reg_t R12;
	reg_t LR;	//LR=r14
	reg_t PC;	//PC=r15
	reg_t xPSR;
} task_stack_frame_t;

struct task_stack_frame_t;

extern void cpu_task_init(void);
extern void cpu_schedule_pend(void);
extern struct task_stack_frame_t* cpu_task_stack_prepair(stack_t *stack_top, task_func_t entry, void *arg);

//extern struct cpu_context* cpu_task_stack_init(void *stack_base, int stack_size, void* entry, void *arg);

//extern void cpu_idle(void);


//irq enable/disable related
extern void cpu_irq_enable(void);
extern void cpu_irq_disable(void);

extern int cpu_irq_save(void);
extern void cpu_irq_restore(int state);

extern int cpu_irq_disabled(void);

extern int cpu_in_irq(void);


#endif /* TASK_PORT_H_ */
