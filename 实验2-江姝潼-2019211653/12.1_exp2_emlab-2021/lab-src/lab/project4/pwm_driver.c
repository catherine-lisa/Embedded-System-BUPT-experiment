/*
 * driver_pwm.c
 *
 *      Author: ljp
 */
#include <string.h>
#include "stm32f4xx_hal.h"
#include "pwm.h"


static TIM_TypeDef * timmap[] = {
    TIM1, TIM2, TIM3, TIM4, TIM5, TIM6, TIM7, TIM8, TIM9, TIM10, TIM11, TIM12, TIM13, TIM14, 0
};

static struct pwm_device _pwm_dev;
// /10.1 -> 10.1
static const char * get_file_name(const char *pathname) {
    int i;
    const char *filename = pathname;
    //skip path name
    for (i = 0; pathname[i] != 0; i++) {
        if(pathname[i] == '/') {
            filename = pathname + i + 1;
        }
    }
    return filename;
}

//name "10.1" -> pwm_num = 10, channel = 1
static int stm32_pwm_get(const char *name)
{
    int pwmcode = 0;
    int pwm_num, channel;
    int name_len;

    name = get_file_name(name);
    name_len = strlen(name);

    if ((name_len < 3) || (name_len > 4)) {
        return -1;
    }
    if(name[1] == '.') {
        pwm_num = (int)(name[0] - '0');
        channel = (int)name[2] - '0';
        pwmcode = PWM_CODE(pwm_num, channel);
        return pwmcode;
    } else if(name[2] == '.') {
        pwm_num = (int)((name[0] - '0') * 10 + (name[1] - '0'));
        channel = (int)name[3] - '0';
        pwmcode = PWM_CODE(pwm_num, channel);
        return pwmcode;
    } else {
        return -1;
    }
}

/* APBx timer clocks frequency doubler state related to APB1CLKDivider value */
static void pclkx_doubler_get(uint32_t *pclk1_doubler, uint32_t *pclk2_doubler)
{
    uint32_t flatency = 0;
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    ASSERT(pclk1_doubler != NULL);
    ASSERT(pclk1_doubler != NULL);

    HAL_RCC_GetClockConfig(&RCC_ClkInitStruct, &flatency);

    *pclk1_doubler = 1;
    *pclk2_doubler = 1;

#if defined(SOC_SERIES_STM32MP1)
    if (RCC_ClkInitStruct.APB1_Div != RCC_APB1_DIV1)
    {
        *pclk1_doubler = 2;
    }
    if (RCC_ClkInitStruct.APB2_Div != RCC_APB2_DIV1)
    {
       *pclk2_doubler = 2;
    }
#else
    if (RCC_ClkInitStruct.APB1CLKDivider != RCC_HCLK_DIV1)
    {
         *pclk1_doubler = 2;
    }
#if !defined(SOC_SERIES_STM32F0) && !defined(SOC_SERIES_STM32G0)
    if (RCC_ClkInitStruct.APB2CLKDivider != RCC_HCLK_DIV1)
    {
         *pclk2_doubler = 2;
    }
#endif
#endif
}

//dev name as "/pwm", filename as "10.1" -> 0xA1
static int stm32_pwm_open(struct device_t *dev, const char *filename, int flags, int mode) {
    (void)dev;
    (void)flags;
    (void)mode;
    //get pwmcode
    int pwmcode = -1;
    //enable TIM
    //TODO:从filename提取定时器号和通道号，编码到pwmcode中，return会传回上层的fd->file_data中

    return pwmcode;
}

static int stm32_pwm_close(struct device_t *dev, void* file_data) {
    (void)dev;
    (void)file_data;
    //no action
    return 0;
}

static int stm32_pwm_write(struct device_t *dev, void* file_data, char *buffer, int size) {
    (void)dev;
    ASSERT(size == 1);
    //TODO:从file_data解码出定时器号和通道号，从buffer中读出数据，据此启动或者关闭pwm通道

    return 1;
}

#define MAX_PERIOD 65535
#define MIN_PERIOD 3
#define MIN_PULSE 2

static int stm32_pwm_set(TIM_HandleTypeDef *htim, int channel, uint32_t period, uint32_t pulse)
{
    uint64_t tim_clock, psc;
    uint32_t pclk1_doubler, pclk2_doubler;
    /* Converts the channel number to the channel number of Hal library */
    channel = 0x04 * (channel - 1);

    pclkx_doubler_get(&pclk1_doubler, &pclk2_doubler);

    if (1)
    {
        tim_clock = (uint32_t)(HAL_RCC_GetPCLK2Freq() * pclk2_doubler);
    }
    else
    {
        tim_clock = (uint32_t)(HAL_RCC_GetPCLK1Freq() * pclk1_doubler);
    }

    /* Convert nanosecond to frequency and duty cycle. 1s = 1 * 1000 * 1000 * 1000 ns */
    tim_clock /= 1000000UL;
    period = (unsigned long long)period * tim_clock / 1000ULL ;
    psc = period / MAX_PERIOD + 1;
    period = period / psc;
    __HAL_TIM_SET_PRESCALER(htim, psc - 1);

    if (period < MIN_PERIOD)
    {
        period = MIN_PERIOD;
    }
    __HAL_TIM_SET_AUTORELOAD(htim, period - 1);

    pulse = (unsigned long long)pulse * tim_clock / psc / 1000ULL;
    if (pulse < MIN_PULSE)
    {
        pulse = MIN_PULSE;
    }
    else if (pulse > period)
    {
        pulse = period;
    }
    __HAL_TIM_SET_COMPARE(htim, channel, pulse - 1);
    __HAL_TIM_SET_COUNTER(htim, 0);

    /* Update frequency value */
    HAL_TIM_GenerateEvent(htim, TIM_EVENTSOURCE_UPDATE);

    return 0;
}

static int stm32_pwm_ioctl(struct device_t *dev, void* file_data, int cmd, void *args) {
    //TODO:从file_data解码出定时器号和通道号，从args中得到控制消息，根据cmd设置pwm定时器TIM10

    (void)dev;

    return 0;
}

void driver_pwm_init(void) {
    _pwm_dev.parent.dev_ops.open = stm32_pwm_open;
    _pwm_dev.parent.dev_ops.write = stm32_pwm_write;
    _pwm_dev.parent.dev_ops.close = stm32_pwm_close;
    _pwm_dev.parent.dev_ops.ioctl = stm32_pwm_ioctl;

    device_register(&_pwm_dev.parent, PWM_DEV_PATH);
}

