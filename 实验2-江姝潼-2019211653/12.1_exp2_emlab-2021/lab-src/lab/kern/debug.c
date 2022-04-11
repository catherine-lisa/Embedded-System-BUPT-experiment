/*
 * debug.c
 *
 *      Author: ljp
 */


#include "debug.h"

void assert_fail(const char *assertion, const char *file, unsigned int line, const char *function)
{
	trace_printf("Failed Assertion '%s' at %s:%u in function %s\r\n", assertion, file, line, function);
	while(1);
}


// static const char *last_irq_save_file;
// static int last_irq_save_line;
// static const char * last_irq_disable_file;
// static int last_irq_disable_line;

// static const char *last_irq_restore_file;
// static int last_irq_restore_line;
// static const char * last_irq_enable_file;
// static int last_irq_enable_line;

static int irq_save_count;
static int irq_restore_count;

void debug_irq_save(int PRIMASK, const char *file, int line) {
	irq_save_count++;
	// last_irq_save_file = file;
	// last_irq_save_line = line;
	// if(PRIMASK == 0) {
	// 	last_irq_disable_file = file;
	// 	last_irq_disable_line = line;
	// }
}

void debug_irq_restore(int PRIMASK, const char *file, int line) {
	irq_restore_count++;
	// last_irq_restore_file = file;
	// last_irq_restore_line = line;
	// if(PRIMASK == 0) {
	// 	last_irq_enable_file = file;
	// 	last_irq_enable_line = line;
	// }
}
