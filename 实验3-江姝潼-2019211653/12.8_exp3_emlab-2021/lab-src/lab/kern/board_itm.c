/*
 * board_itm.c
 *
 *      Author: ljp
 */

#if   defined ( __CC_ARM )
#ifdef STM32F10X_MD


#include "board_cm.h"
#include "board.h"


//#define ITM_RXBUFFER_EMPTY    0x5AA55AA5

/*!< Variable to receive characters. */
extern
volatile int32_t ITM_RxBuffer;
volatile int32_t ITM_RxBuffer = ITM_RXBUFFER_EMPTY;


__weak void in_poll_loop() {
	//task_yield();
}


int board_getc_ready(void) {
	return 1;
}

char board_getc(void) {
	int32_t ch;
	do {
	    ch = ITM_ReceiveChar();
	  } while (ch == -1);
	return ch;
}

int board_putc_ready(void) {
	return 1;
}

void board_putc(unsigned char ch) {
	ITM_SendChar(ch);
}

__weak void board_putc_debug(unsigned char ch) {
	ITM_SendChar(ch);
}

#include <stdio.h>
#include <stdarg.h>
int fputc(int ch, FILE *f)
{
	board_putc_debug((uint8_t)ch);
	return ch;
}

static int trace_write(const char *s, int n) {
	for (int i = 0; i < n; ++i) {
		board_putc_debug(s[i]);
	}
	return n;
}

__weak int trace_printf(const char *format, ...) {
	int ret;
	va_list ap;

	va_start(ap, format);

	// TODO: rewrite it to no longer use newlib, it is way too heavy

	static char buf[128];

	// Print to the local buffer
	ret = vsnprintf(buf, sizeof(buf), format, ap);
	if (ret > 0) {
		// Transfer the buffer to the device
		ret = trace_write(buf, (size_t) ret);
	}

	va_end(ap);
	return ret;
}


#endif //STM32F10X_MD
#endif //__CC_ARM
