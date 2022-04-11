/*
 *      Author: ljp
 */

#ifndef BOARD_H_
#define BOARD_H_


#if   defined ( __CC_ARM )
#define CONFIG_USE_BOARD_FSM4	1
#endif

#if   defined ( __GNUC__ )
#define CONFIG_USE_BOARD_QEMU	1
#endif

void board_init(void);
extern char board_getc(void);
extern void board_putc(unsigned char c);
extern int board_getc_ready(void);
extern int board_putc_ready(void);

int trace_printf(const char* format, ...);

#ifndef __IO
extern void HAL_Delay(volatile  unsigned int Delay);;
#endif


#endif /* BOARD_H_ */
