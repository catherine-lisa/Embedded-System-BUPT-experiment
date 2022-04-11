/*
 * kshell_cmd.c
 *
 *      Author: ljp
 */

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "kern_config.h"
#include "kern_api.h"
#include "kshell.h"

/***************************************************************
 * ush commands
 ***************************************************************/

void cmd_help(struct uart_shell *shell, int argc, char *argv[]) {
	(void) argc;
	(void) argv;

	ksh_printf(shell, "shell commands:"CR);
	{
		struct ksh_cmd *s_cmd;

		for (s_cmd = &ksh_cmd_table[0]; s_cmd->func != NULL; s_cmd++) {
			ksh_printf(shell, "%-16s - %s"CR, s_cmd->name, s_cmd->desc);
		}
	}
	ksh_printf(shell, CR);

	return;
}

static void display_mem(struct uart_shell *shell, unsigned char *pBfr, int bfrLen) {
	int i, j;

	for (i = 0; i < bfrLen; i += 16) {
		ksh_printf(shell, "%08x:", (unsigned int) (pBfr + i));
		for (j = i; j < bfrLen && j < i + 16; j++) {
			ksh_printf(shell, "%2.2x ", pBfr[j]);
			if (j % 16 == 7) {
				ksh_printf(shell, " ");
			}
		}
		ksh_printf(shell, " ");
		for (j = i; j < bfrLen && j < i + 16; j++) {
			ksh_printf(shell, "%c",
					(pBfr[j] >= 32 && pBfr[j] <= 127) ? pBfr[j] : '.');
		}
		ksh_printf(shell, "\r\n");
	}
}

static unsigned int shell_md_address, shell_md_size = 128;
static void cmd_d(struct uart_shell *shell, int argc, char *argv[]) {
	if (argc > 1) {
		if (argv[1] != NULL) {
			char *endptr;
			shell_md_address = strtoul(argv[1], &endptr, 0);
			if (shell_md_address == ULONG_MAX || *endptr != '\0') {
				ksh_printf(shell, "wrong address:%s!\r\n", argv[1]);
				return;
			}
		}
		if (argv[2] != NULL) {
			char *endptr;
			shell_md_size = strtoul(argv[2], &endptr, 0);
			if (shell_md_size == ULONG_MAX || *endptr != '\0') {
				ksh_printf(shell, "wrong size:%s!\r\n", argv[2]);
				return;
			}
		}
	}
	display_mem(shell, (unsigned char*) shell_md_address, (int) shell_md_size);
	shell_md_address += shell_md_size;
}

static void cmd_sleep(struct uart_shell *shell, int argc, char *argv[]) {
	unsigned int ticks = 1;
	if (argc > 1) {
		if (argv[1] != NULL) {
			char *endptr;
			ticks = strtoul(argv[1], &endptr, 0);
			if (ticks == ULONG_MAX || *endptr != '\0') {
				ksh_printf(shell, "wrong params:%s!\r\n", argv[1]);
				return;
			}
		}
	}
	tick_t t1, t2;
	t1 = timer_ticks();
	task_sleep(ticks);
	t2 = timer_ticks();
	ksh_printf(shell, "sleep %u ticks(%d -> %d)\n", ticks, t1, t2);
}

extern void task_info_all(void);
extern int _timer_count;
static void cmd_i(struct uart_shell *shell, int argc, char *argv[]) {
	(void)argc;(void)argv;
	ksh_printf(shell, "time %u ticks, %d timers\n", timer_ticks(), _timer_count);
	task_info_all();
}

void HAL_NVIC_SystemReset(void);
void cmd_reboot(struct uart_shell *shell, int argc, char *argv[]) {
	(void)argc;(void)argv;
	ksh_printf(shell, "reboot...\n");
	HAL_NVIC_SystemReset();
}

//void cmd_help(struct uart_shell *shell, int argc, char *argv[]);
//void cmd_led(struct uart_shell *shell, int argc, char *argv[]);
//void cmd_d(struct uart_shell *shell, int argc, char *argv[]);

struct ksh_cmd ksh_cmd_table[CONFIG_MAX_CMD] = { 
		{ "?", "print this help", cmd_help }, //help
		{ "help", "print this help", cmd_help }, 		//help
		{ "d", "d [address size], memory display", cmd_d }, //dispaly memory
		{ "i", "i, show system info", cmd_i }, //dispaly systemn info
		{ "sleep", "sleep [n], shell sleep n ticks", cmd_sleep },
		{ "reboot", "reboot, system reboot", cmd_reboot },
		{NULL, NULL, NULL}
		};

//struct ksh_cmd *ksh_cmd_table_begin = &ksh_cmd_table[0];
//struct ksh_cmd *ksh_cmd_table_end = &ksh_cmd_table[sizeof(ksh_cmd_table)
//		/ sizeof(ksh_cmd_table[0])];

struct ksh_cmd* kshell_cmd_add(void *func, const char *name, const char *desc) {
	struct ksh_cmd *s_cmd;
	for (s_cmd = &ksh_cmd_table[0]; s_cmd < &ksh_cmd_table[CONFIG_MAX_CMD]; s_cmd++) {
		if (!s_cmd->func) {
			s_cmd->name = name;
			s_cmd->desc = desc;
			s_cmd->func = (cmd_function_t) func;
			return s_cmd;
		}
	}
	return NULL;
}

cmd_function_t kshell_find_cmd(char *cmd, int size) {
	struct ksh_cmd *s_cmd;
	cmd_function_t cmd_func = NULL;

	for (s_cmd = &ksh_cmd_table[0]; s_cmd->func != NULL; s_cmd++) {
		if (strncmp(s_cmd->name, cmd, size) == 0 && (s_cmd->name[size] == '\0' || s_cmd->name[size] == ' ')) {
			cmd_func = (cmd_function_t) s_cmd->func;
			break;
		}
	}

	return cmd_func;
}
