/*
* Copyright (c) 2006-2018, RT-Thread Development Team
*
* SPDX-License-Identifier: Apache-2.0
*
* Change Logs:
* Date           Author       Notes
* 2018-10-17     flybreak      the first version
*/

#ifndef BEEP_KEY_H
#define BEEP_KEY_H

#include "button.h"
#include "player.h"
#include "pin.h"

#define KEY_PLAY_PIN     2
#define KEY_LAST_PIN     1
#define KEY_NEXT_PIN     3

#define KEY_PLAY_PIN_NAME     PIN_DEV_PATH"/PI.9"   //K3
#define KEY_NEXT_PIN_NAME     PIN_DEV_PATH"/PF.11"  //K4
#define KEY_LAST_PIN_NAME     PIN_DEV_PATH"/PC.13"  //K5
#define KEY_LIST_PIN_NAME     PIN_DEV_PATH"/PA.0"   //K6

#define KEY_PRESS_LEVEL  0

int key_init(void);                     //按键初始化

#endif
