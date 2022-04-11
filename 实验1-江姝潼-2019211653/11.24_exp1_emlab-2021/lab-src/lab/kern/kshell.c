/*
 *      Author: ljp
 */

#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "stdarg.h"
#include "limits.h"
#include "kshell.h"

extern int trace_printf(const char *format, ...);
extern void cmd_help(struct uart_shell *shell, int argc, char *argv[]);

#define USING_HISTORY 		1
#define CMD_ARG_MAX    		5
#define MAX_HISTORY_LINES 	10
#define CMD_SIZE			80

#define SHELL_PROMPT     	"#"
#define SHELL_ESCAPSE_CHAR	0x1d	/*Ctrl + ]*/


enum input_stat {
	WAIT_NORMAL, WAIT_SPEC_KEY, WAIT_FUNC_KEY,
};

typedef struct uart_shell {
	unsigned int echo_mode;

	enum input_stat stat;

	char line[CMD_SIZE];
	int line_position;
	int line_curpos;

#ifdef USING_HISTORY
	unsigned int current_history;
	unsigned int history_count;
	char cmd_history[MAX_HISTORY_LINES][CMD_SIZE];
#endif

	/*input and output function for frontend*/
	getc_func_t _getchar;
	putc_func_t _putchar;
} uart_shell_t;


#define PRINTF_BUFFER_SIZE 128
void ksh_printf(struct uart_shell *shell, char *fmt, ...) {
	va_list ap;
	char buffer[PRINTF_BUFFER_SIZE];
	char *chstring = buffer;

	if (!shell->_putchar)
		return;

	va_start(ap, fmt);
	vsnprintf(buffer, PRINTF_BUFFER_SIZE, fmt, ap);
	while (*chstring)
		shell->_putchar(*chstring++);
	va_end(ap);
}

#ifdef USING_HISTORY
static int shell_show_history(struct uart_shell *shell) {
	/*clear command  display*/
	ksh_printf(shell, "\r																			   \r"); //clean line, for windows
	//ksh_printf(shell, "\033[2K\r");/*this works fine */
	ksh_printf(shell, "%s%s", SHELL_PROMPT, shell->line);
	return 0;
}

static void shell_push_history(struct uart_shell *shell) {
	if (shell->line_position != 0) {
		/* push history */
		if (shell->history_count >= MAX_HISTORY_LINES) {
			/* move history */
			int index;
			for (index = 0; index < MAX_HISTORY_LINES - 1; index++) {
				memcpy(&shell->cmd_history[index][0],
						&shell->cmd_history[index + 1][0], CMD_SIZE);
			}
			memset(&shell->cmd_history[index][0], 0, CMD_SIZE);
			memcpy(&shell->cmd_history[index][0], shell->line,
					shell->line_position);

			/* it's the maximum history */
			shell->history_count = MAX_HISTORY_LINES;
		} else {
			memset(&shell->cmd_history[shell->history_count][0], 0, CMD_SIZE);
			memcpy(&shell->cmd_history[shell->history_count][0], shell->line,
					shell->line_position);

			/* increase count and set current history position */
			shell->history_count++;
		}
	}
	shell->current_history = shell->history_count;
}

#endif

static int str_common(const char *str1, const char *str2) {
	const char *str = str1;

	while ((*str != 0) && (*str2 != 0) && (*str == *str2)) {
		str++;
		str2++;
	}

	return (str - str1);
}

void ksh_auto_complete_internal(struct uart_shell *shell, char *prefix) {
	int length, min_length;
	const char *name_ptr, *cmd_name;
	struct ksh_cmd *s_cmd;

	min_length = 0;
	name_ptr = NULL;

	if (*prefix  == '\0') {
		cmd_help(shell, 0, NULL);
		return;
	}

	/* checks in internal command */
	{
		for (s_cmd = &ksh_cmd_table[0]; s_cmd->func != NULL; s_cmd++) {
			cmd_name = (const char*) s_cmd->name;
			if (strncmp(prefix, cmd_name, strlen(prefix)) == 0) {
				if (min_length == 0) {
					/* set name_ptr */
					name_ptr = cmd_name;
					/* set initial length */
					min_length = strlen(name_ptr);
				}

				length = str_common(name_ptr, cmd_name);
				if (length < min_length)
					min_length = length;

				ksh_printf(shell, "%s"CR, cmd_name);
			}
		}
	}

	/* auto complete string */
	if (name_ptr != NULL) {
		strncpy(prefix, name_ptr, min_length);
	}

	return;
}

static void ksh_auto_complete(struct uart_shell *shell, char *prefix) {
	ksh_printf(shell, CR);
	ksh_auto_complete_internal(shell, prefix);
	ksh_printf(shell, "%s%s", SHELL_PROMPT, prefix);
}

static int ksh_split_cmdline(char *cmd, unsigned int length,
		char *argv[CMD_ARG_MAX]) {
	char *ptr;
	unsigned int position;
	unsigned int argc;

	ptr = cmd;
	position = 0;
	argc = 0;

	while (position < length) {
		/* strip blank and tab */
		while ((*ptr == ' ' || *ptr == '\t') && position < length) {
			*ptr = '\0';
			ptr++;
			position++;
		}
		if (position >= length)
			break;

		/* handle string */
		if (*ptr == '"') {
			ptr++;
			position++;
			argv[argc] = ptr;
			argc++;

			/* skip this string */
			while (*ptr != '"' && position < length) {
				if (*ptr == '\\') {
					if (*(ptr + 1) == '"') {
						ptr++;
						position++;
					}
				}
				ptr++;
				position++;
			}
			if (position >= length)
				break;

			/* skip '"' */
			*ptr = '\0';
			ptr++;
			position++;
		} else {
			argv[argc] = ptr;
			argc++;
			while ((*ptr != ' ' && *ptr != '\t') && position < length) {
				ptr++;
				position++;
			}
			if (position >= length)
				break;
		}
	}

	return argc;
}

static int ksh_parse_and_exec_cmd(struct uart_shell *shell, char *cmd,
		unsigned int length, int *retp) {
	int argc;
	unsigned int cmd0_size = 0;
	cmd_function_t cmd_func;
	char *argv[CMD_ARG_MAX];

	if ((!cmd) || (!retp))
		return -1;

	/* find the size of first command */
	while ((cmd[cmd0_size] != ' ' && cmd[cmd0_size] != '\t')
			&& cmd0_size < length)
		cmd0_size++;
	if (cmd0_size == 0)
		return -1;

	cmd_func = kshell_find_cmd(cmd, cmd0_size);
	if (cmd_func == NULL)
		return -1;

	/* split arguments */
	memset(argv, 0x00, sizeof(argv));
	argc = ksh_split_cmdline(cmd, length, argv);
	if (argc == 0)
		return -1;

	/* exec this command */
	/**retp = */cmd_func(shell, argc, argv);
	return 0;
}

static int ksh_execute_internal(struct uart_shell *shell, char *cmd,
		unsigned int length) {
	int cmd_ret;

	/* strim the beginning of command */
	while (*cmd == ' ' || *cmd == '\t') {
		cmd++;
		length--;
	}

	if (length == 0)
		return 0;

	/* Exec command*/
	if (ksh_parse_and_exec_cmd(shell, cmd, length, &cmd_ret) == 0) {
		return cmd_ret;
	}

	/* truncate the cmd at the first space. */
	{
		char *tcmd;
		tcmd = cmd;
		while (*tcmd != ' ' && *tcmd != '\0') {
			tcmd++;
		}
		*tcmd = '\0';
	}
	ksh_printf(shell, "[%s] command not found."CR, cmd);
	return -1;
}

static char* shell_readline(struct uart_shell *shell) {
	char ch0 = 0;
	char ch = 0;

	while (1) {
		/* read one character from device */
		ch0 = ch;
		int ret = shell->_getchar(); /* loop until we get a char */
		if(ret < 0) {
			continue;
		}
		ch = (char)(ret & 0xff);

		if (ch == SHELL_ESCAPSE_CHAR) {
			return NULL;
		}

		if ((ch0 == '\r') && (ch == '\0'))/*telnet client send 0x0d 0x00 for return, but uart send 0x0d, we skip 0x0d 0x00*/
		{
			continue;
		}

		/*
		 * handle control key
		 * up key  : 0x1b 0x5b 0x41
		 * down key: 0x1b 0x5b 0x42
		 * right key:0x1b 0x5b 0x43
		 * left key: 0x1b 0x5b 0x44
		 */
		if (ch == 0x1b) {
			shell->stat = WAIT_SPEC_KEY;
			continue;
		} else if (shell->stat == WAIT_SPEC_KEY) {
			if (ch == 0x5b) {
				shell->stat = WAIT_FUNC_KEY;
				continue;
			}

			shell->stat = WAIT_NORMAL;
		} else if (shell->stat == WAIT_FUNC_KEY) {
			shell->stat = WAIT_NORMAL;

			if (ch == 0x41) /* up key */
			{
#ifdef USING_HISTORY
				/* prev history */
				if (shell->current_history > 0)
					shell->current_history--;
				else {
					shell->current_history = 0;
					continue;
				}

				/* copy the history command */
				memcpy(shell->line,
						&shell->cmd_history[shell->current_history][0],
						CMD_SIZE);
				shell->line_curpos = shell->line_position = strlen(shell->line);
				shell_show_history(shell);
#endif
				continue;
			} else if (ch == 0x42) /* down key */
			{
#ifdef USING_HISTORY
				/* next history */
				if (shell->current_history < shell->history_count - 1)
					shell->current_history++;
				else {
					/* set to the end of history */
					if (shell->history_count != 0)
						shell->current_history = shell->history_count - 1;
					else
						continue;
				}

				memcpy(shell->line,
						&shell->cmd_history[shell->current_history][0],
						CMD_SIZE);
				shell->line_curpos = shell->line_position = strlen(shell->line);
				shell_show_history(shell);
#endif
				continue;
			} else if (ch == 0x44) /* left key */
			{
				if (shell->line_curpos) {
					ksh_printf(shell, "\b");
					shell->line_curpos--;
				}

				continue;
			} else if (ch == 0x43) /* right key */
			{
				if (shell->line_curpos < shell->line_position) {
					ksh_printf(shell, "%c", shell->line[shell->line_curpos]);
					shell->line_curpos++;
				}

				continue;
			}

		}

		if (ch == '\t') {
			int i;
			/* move the cursor to the beginning of line */
			for (i = 0; i < shell->line_curpos; i++)
				ksh_printf(shell, "\b");

			/* auto complete */
			ksh_auto_complete(shell, &shell->line[0]);
			/* re-calculate position */
			shell->line_curpos = shell->line_position = strlen(shell->line);

			continue;
		}
		/* handle backspace key */
		else if (ch == 0x7f || ch == 0x08) {
			/* note that shell->line_curpos >= 0 */
			if (shell->line_curpos == 0)
				continue;

			shell->line_position--;
			shell->line_curpos--;

			if (shell->line_position > shell->line_curpos) {
				int i;

				memmove(&shell->line[shell->line_curpos],
						&shell->line[shell->line_curpos + 1],
						shell->line_position - shell->line_curpos);
				shell->line[shell->line_position] = 0;

				ksh_printf(shell, "\b%s  \b", &shell->line[shell->line_curpos]);

				/* move the cursor to the origin position */
				for (i = shell->line_curpos; i <= shell->line_position; i++)
					ksh_printf(shell, "\b");
			} else {
				ksh_printf(shell, "\b \b");
				shell->line[shell->line_position] = 0;
			}

			continue;
		}

		/* handle end of line, break */
		if (ch == '\r' || ch == '\n') {
#ifdef USING_HISTORY
			shell_push_history(shell);
#endif

			/*exec command*/
			return shell->line;
		}

		/* it's a large line, discard it */
		if (shell->line_position >= CMD_SIZE)
			shell->line_position = 0;

		/* normal character */
		if (shell->line_curpos < shell->line_position) {
			int i;

			memmove(&shell->line[shell->line_curpos + 1],
					&shell->line[shell->line_curpos],
					shell->line_position - shell->line_curpos);
			shell->line[shell->line_curpos] = ch;
			if (shell->echo_mode)
				ksh_printf(shell, "%s", &shell->line[shell->line_curpos]);

			/* move the cursor to new position */
			for (i = shell->line_curpos; i < shell->line_position; i++)
				ksh_printf(shell, "\b");
		} else {
			shell->line[shell->line_position] = ch;
			if (shell->echo_mode)
				ksh_printf(shell, "%c", ch);
		}

		ch = 0;
		shell->line_position++;
		shell->line_curpos++;
		if (shell->line_position >= 80) {
			/* clear command line */
			shell->line_position = 0;
			shell->line_curpos = 0;
		}
	} /* end of device read */
}

//execute a command in shell->line
void shell_execute(struct uart_shell *shell) {

	ksh_printf(shell, CR);

	ksh_execute_internal(shell, shell->line, shell->line_position);

	ksh_printf(shell, SHELL_PROMPT);
	memset(shell->line, 0, sizeof(shell->line));
	shell->line_curpos = shell->line_position = 0;
}

//Main loop, get input and execute it.
void kshell_loop(struct uart_shell *shell) {

	ksh_printf(shell, SHELL_PROMPT);

	//loop until ctrl+] received
	while (shell_readline(shell)) {
		shell_execute(shell);
	}

	ksh_printf(shell, CR"Bye Bye"CR);
}

static struct uart_shell _kshell;
/*return struct uart_shell*, keep it for ksh_teardown*/
void* kshell_init(getc_func_t _getchar, putc_func_t _putchar) {
	struct uart_shell *shell = &_kshell;

	memset(shell, 0, sizeof(struct uart_shell));

	/* normal is echo mode */
	shell->echo_mode = 1;
	shell->stat = WAIT_NORMAL;
	shell->_getchar = _getchar;
	shell->_putchar = _putchar;

	return (shell);
}

void kshell_register(getc_func_t _getchar, putc_func_t _putchar) {
	struct uart_shell *shell = &_kshell;
	shell->_getchar = _getchar;
	shell->_putchar = _putchar;
}

//=======main entry
#include "board.h"
#include "task_api.h"

static int getc_default(void) {
	while(!board_getc_ready()) {
		task_sleep(1);
	}
	return board_getc();
}

static int putc_default(char ch) {
	board_putc(ch);
	return ch;
}

//main entry
static void kshell_task(void) {
	kshell_loop(&_kshell);
}

void kshell_lib_init(void) {
	kshell_init(getc_default, putc_default);
	task_create((task_func_t)kshell_task, NULL, PRIO_LOWEST + 1, "kshell_task");
}
