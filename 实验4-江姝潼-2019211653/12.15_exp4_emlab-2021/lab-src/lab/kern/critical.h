/*
 * critical.h
 *
 *      Author: ljp
 */

#ifndef CRITICAL_H_
#define CRITICAL_H_


extern int cpu_irq_save(void);
extern void cpu_irq_restore(int state);


#define CPU_CRITICAL_ENTER()    int  __irqstate__ = cpu_irq_save();debug_irq_save(__irqstate__, __FILE__,  __LINE__)
#define CPU_CRITICAL_EXIT()     debug_irq_restore(__irqstate__, __FILE__, __LINE__); cpu_irq_restore(__irqstate__)

#define WITHIN_CRITICAL() for ( \
 	 	 	 int __irqsr = cpu_irq_save(), __ret = 0;	\
			!__ret;					\
			cpu_irq_restore(__irqsr), __ret = 1)


#endif /* CRITICAL_H_ */
