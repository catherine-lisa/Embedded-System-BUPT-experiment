/*
 * kshell.h
 *
 *      Author: ljp
 */

#ifndef PROJECT3_KSHELL_H_
#define PROJECT3_KSHELL_H_

struct uart_shell;
typedef int (*getc_func_t)(void);
typedef int (*putc_func_t)(char);

void* kshell_init(getc_func_t, putc_func_t);
void kshell_register(getc_func_t, putc_func_t);
void kshell_loop(struct uart_shell *shell);

void kshell_lib_init(void);

#define CR "\r\n"

void ksh_printf(struct uart_shell *shell, char *fmt, ...);



typedef void (*cmd_function_t)(struct uart_shell *shell, int argc, char *argv[]);

/* kshell cmd table */
struct ksh_cmd {
	const char *name; /* the name of system call */
	const char *desc; /* description of system call */
	cmd_function_t func;/* the function address */
};
extern struct ksh_cmd ksh_cmd_table[];

struct ksh_cmd* kshell_cmd_add(void *func, const char *name, const char *desc);
cmd_function_t kshell_find_cmd(char *cmd, int size);

#endif /* PROJECT3_KSHELL_H_ */
