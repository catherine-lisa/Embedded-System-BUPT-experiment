/*
 * kern_init.c
 *
 *      Author: ljp
 */
#ifdef STM32F407xx

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "board_cm.h"
#include "kern_api.h"

#define PROJECT_ITEM(func) {&func, #func}

extern void project1_main(void);
extern void project2_main(void);
extern void project3_main(void);
extern void project4_main(void);
static void project_list(void);

struct project_info {
    void (*prj_entry)(void);
    const char * desc;
} prj_table[] = {
    PROJECT_ITEM(project_list),
    PROJECT_ITEM(project1_main),
    PROJECT_ITEM(project2_main),
    PROJECT_ITEM(project3_main),
    PROJECT_ITEM(project4_main),
};

static void project_list(void) {
    trace_printf("project entry list:\n");
    for (size_t i = 0; i < NELEM(prj_table); i++)
    {
        trace_printf("%d: %s\n", i, prj_table[i].desc);
    }
}

static void cmd_prj(struct uart_shell *shell, int argc, char *argv[]) {
    if (argc == 1) {
        project_list();
    } else {
        char *endptr;
        int idx = strtoul(argv[1], &endptr, 0);
        if (idx == ULONG_MAX || *endptr != '\0') {
            ksh_printf(shell, "wrong params:%s!\r\n", argv[1]);
            return;
        }
        if (idx >= 0 && idx < NELEM(prj_table)) {
            task_create_simple(prj_table[idx].prj_entry, 0);
            // (*prj_table[idx].prj_entry)();
        } else {
            ksh_printf(shell, "para %s out of range[0..%d]!\r\n", argv[1], NELEM(prj_table));
        }
    }
}

void lab_init(void) {
    kshell_cmd_add(cmd_prj, "prj", "prj n. run project n");
}




static void check_uart(USART_TypeDef *uart) {
    char ch;
    //echo
    if (uart->SR & UART_FLAG_RXNE) {
        ch = (char)(uart->DR & (unsigned char)0x00FFU);
        while (!(uart->SR & UART_FLAG_TXE));
        uart->DR = ch;
    }
}

static int gpio_read(GPIO_TypeDef *port, int pin) {
    int one = (port->IDR >> pin) & 1;
    return one;
}
static void gpio_set(GPIO_TypeDef *port, int pin) {
    port->ODR |= (1 << pin);
}
static void gpio_clr(GPIO_TypeDef *port, int pin) {
    port->ODR &= ~(1 << pin);
}
static void gpio_write(GPIO_TypeDef *port, int pin, int one) {
    if(one) {
        gpio_set(port, pin);
    } else {
        gpio_clr(port, pin);
    }
}

static void check_gpio(void) {
    gpio_write(GPIOF, 7, gpio_read(GPIOI, 9));
    gpio_write(GPIOF, 8, gpio_read(GPIOF, 11));
    gpio_write(GPIOF, 9, gpio_read(GPIOC, 13));
    gpio_write(GPIOF, 10, gpio_read(GPIOA, 0));
}

static uint32_t freq = 0;
static const uint16_t freq_table[12]  = {262, 277, 294, 311, 330, 349, 369, 392, 415, 440, 466, 494}; 

static void tim_pwm_start(TIM_TypeDef *tim, uint32_t Channel) {
    TIM_HandleTypeDef htim;
    htim.Instance = tim;

    if (freq >= NELEM(freq_table)) {
        freq = 0;
    } else {
        freq++;
    }

    HAL_TIM_PWM_Start(&htim, Channel);
    tim->ARR = 1000000 / freq_table[freq];
    tim->CCR1 = 1000000 / freq_table[freq] * 0.8;
}

static void tim_pwm_stop(TIM_TypeDef *tim, uint32_t Channel) {
    TIM_HandleTypeDef htim;
    htim.Instance = tim;

    HAL_TIM_PWM_Stop(&htim, Channel);
}

static void check_tim(TIM_TypeDef *tim, uint32_t Channel) {
    if(!gpio_read(GPIOI, 9)) {
        tim_pwm_start(tim, Channel);
    } else {
        tim_pwm_stop(tim, Channel);
    }
}

static volatile int smoking = 0;
static timer_t stop_timer;

void stop_smoke(void) {
    smoking = 0;
}

extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart3;

static void blow(void) {
	const char *str1 = "THIS is UART1\n";
	const char *str3 = "THIS is UART3\n";
	HAL_UART_Transmit(&huart1, (unsigned char*)str1, strlen(str1), 100);//say hello
	HAL_UART_Transmit(&huart3, (unsigned char*)str3, strlen(str3), 100);
    gpio_clr(GPIOF, 7);     //led on
    gpio_clr(GPIOF, 8);
    gpio_clr(GPIOF, 9);
    gpio_clr(GPIOF, 10);
    tim_pwm_start(TIM10, 0);   //beep
    task_sleep(500);
    tim_pwm_stop(TIM10, 0);
}

static void smoke_task(void) {
    trace_printf("\nsmoke start\n");
    trace_printf("Check: LED6-9 lighting? Beeping? Push K3-K6 to light LED6-9.\nHold K3 + K4 to stop test.\nUart in echo mode now.\n");
    smoking = 1;
    blow();
    while(smoking) {
        check_uart(USART1);
        check_uart(USART3);
        check_gpio();
        check_tim(TIM10, 0);

        task_sleep(1);
        if(!gpio_read(GPIOI, 9) && !gpio_read(GPIOF, 11))  { //k3+k4
            smoking = 0;
            timer_cancel(&stop_timer);
        }
    }
    tim_pwm_stop(TIM10, 0);
    trace_printf("\nsmoke done\n");
}

//verify board configuration is right
static void cmd_smoke(struct uart_shell *shell, int argc, char *argv[]) {
    if(!smoking) {
        timer_add_oneshot(timer_init(&stop_timer), ms_to_tick(1000 * 30), (timer_func_t)stop_smoke, 0);
        task_create_simple(smoke_task, 0);
    }
}

void app_main(void) {
    lab_init();
    kshell_cmd_add(cmd_smoke, "smoke", "smoke. run board smoke testing");
}

#endif //STM32F407xx
