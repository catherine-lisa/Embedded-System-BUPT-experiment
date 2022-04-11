/*
* Copyright (c) 2006-2018, RT-Thread Development Team
*
* SPDX-License-Identifier: Apache-2.0
*
* Change Logs:
* Date           Author       Notes
* 2018-10-17     flybreak      the first version
*/

#ifndef LED_H
#define LED_H

#include "pin.h"

#define LED_PIN     PIN_DEV_PATH"/PF.10"    //D9

int led_init(void);                     //LED 灯初始化
int led_on(void);                       //LED 灯亮
int led_off(void);                      //LED 灯灭
int led_toggle(void);                   //LED 灯亮灭状态翻转

#endif
