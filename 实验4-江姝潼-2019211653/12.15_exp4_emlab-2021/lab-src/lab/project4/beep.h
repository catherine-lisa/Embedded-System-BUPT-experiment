/*
* Copyright (c) 2006-2018, RT-Thread Development Team
*
* SPDX-License-Identifier: Apache-2.0
*
* Change Logs:
* Date           Author       Notes
* 2018-10-17     flybreak      the first version
*/

#ifndef BEEP_H
#define BEEP_H

#include "kern/kern_api.h"
#include "pwm.h"

// #define BEEP_PWM_DEVICE  "pwm10"
// #define BEEP_PWM_NUM  10
// #define BEEP_PWM_CH      1

//蜂鸣器的设备文件路径。pwm10，通道1
#define BEEP_PWM_NAME   PWM_DEV_PATH"/10.1"

int beep_init(void);                         //蜂鸣器初始化
int beep_on(void);                           //蜂鸣器开
int beep_off(void);                          //蜂鸣器关
int beep_set(uint16_t freq, uint8_t volume); //蜂鸣器设定

#endif
