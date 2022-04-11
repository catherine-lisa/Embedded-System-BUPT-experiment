/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 */

#ifndef MY_BUTTON_H
#define MY_BUTTON_H

#include "kern/kern_api.h"

#define MY_BUTTON_DOWN_MS        50
#define MY_BUTTON_HOLD_MS        700

#define MY_BUTTON_SCAN_SPACE_MS  20
#define MY_BUTTON_LIST_MAX       10

typedef void (*my_button_callback)(void*);

enum my_button_event
{
    BUTTON_EVENT_CLICK_DOWN,
    BUTTON_EVENT_CLICK_UP,
    BUTTON_EVENT_HOLD,
    BUTTON_EVENT_HOLD_CYC,
    BUTTON_EVENT_HOLD_UP,
    BUTTON_EVENT_NONE
};

struct my_button
{
    uint8_t  press_logic_level;
    uint16_t cnt;
    uint16_t hold_cyc_period;
    uint16_t pin;
    const char *pin_name;

    enum my_button_event event;

    my_button_callback cb;
};

int my_button_register(struct my_button *button);
int my_button_start(void);

#endif
