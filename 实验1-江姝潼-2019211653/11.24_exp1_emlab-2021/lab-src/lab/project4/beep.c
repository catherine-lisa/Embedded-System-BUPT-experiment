/*
* Copyright (c) 2006-2018, RT-Thread Development Team
*
* SPDX-License-Identifier: Apache-2.0
*
* Change Logs:
* Date           Author       Notes
* 2018-10-17     flybreak      the first version
*/

#include "kern/ios_device.h"

#include "beep.h"
#include "pwm.h"

int beep_init(void)
{
    /* 初始化PWM设备 */
    pwm_init(BEEP_PWM_NAME);
    
    return 0;
}

int beep_on(void)
{
    pwm_enable(BEEP_PWM_NAME); //使能蜂鸣器对应的 PWM 通道

    return 0;
}

int beep_off(void)
{
    pwm_disable(BEEP_PWM_NAME); //失能蜂鸣器对应的 PWM 通道

    return 0;
}

int beep_set(uint16_t freq, uint8_t volume)
{
    uint32_t period, pulse;

    /* 将频率转化为周期 周期单位:ns 频率单位:HZ */
    period = 1000000000 / freq;  //unit:ns 1/HZ*10^9 = ns

    /* 根据声音大小计算占空比 蜂鸣器高电平触发 */
    // pulse = period - period / 100 * volume;//低电平触发
    pulse = period / 100 * volume;

    /* 利用 PWM API 设定 周期和占空比 */
    pwm_set(BEEP_PWM_NAME, period, pulse);//channel,period,pulse

    return 0;
}
