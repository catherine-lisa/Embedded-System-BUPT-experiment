/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 */

#include "kern/kern_api.h"
#include "pin.h"
#include "button.h"

/****    Debug     ****/
#define LOCAL_TRACE TRACE_LVL_DEBUG

#define MY_BUTTON_CALL(func, argv) \
    do { if ((func) != NULL) func argv; } while (0)

struct my_button_manage
{
    uint8_t num;
    timer_t timer;
    struct my_button *button_list[MY_BUTTON_LIST_MAX];
};

static struct my_button_manage button_manage;

int my_button_register(struct my_button *button)
{
    if (button->press_logic_level == 0)
    {
        pin_mode(button->pin_name, PIN_MODE_INPUT_PULLUP);
    }
    else
    {
        pin_mode(button->pin_name, PIN_MODE_INPUT_PULLDOWN);
    }
    
    button->cnt = 0;
    button->event = BUTTON_EVENT_NONE;
    button_manage.button_list[button_manage.num++] = button;
    
    return 0;
}

static void my_button_scan(void *param)
{
    uint8_t i;
    uint16_t cnt_old;

    (void)param;
    for (i = 0; i < button_manage.num; i++)
    {
        cnt_old = button_manage.button_list[i]->cnt;

        if (pin_read(button_manage.button_list[i]->pin_name) == button_manage.button_list[i]->press_logic_level)
        {
            button_manage.button_list[i]->cnt ++;

            if (button_manage.button_list[i]->cnt == MY_BUTTON_DOWN_MS / MY_BUTTON_SCAN_SPACE_MS) /* BUTTON_DOWN */
            {
                TRACE_D("BUTTON_DOWN");
                button_manage.button_list[i]->event = BUTTON_EVENT_CLICK_DOWN;
                MY_BUTTON_CALL(button_manage.button_list[i]->cb, (button_manage.button_list[i]));
            }
            else if (button_manage.button_list[i]->cnt == MY_BUTTON_HOLD_MS / MY_BUTTON_SCAN_SPACE_MS) /* BUTTON_HOLD */
            {
                TRACE_D("BUTTON_HOLD");
                button_manage.button_list[i]->event = BUTTON_EVENT_HOLD;
                MY_BUTTON_CALL(button_manage.button_list[i]->cb, (button_manage.button_list[i]));
            }
            else if (button_manage.button_list[i]->cnt > MY_BUTTON_HOLD_MS / MY_BUTTON_SCAN_SPACE_MS) /* BUTTON_HOLD_CYC */
            {
                TRACE_D("BUTTON_HOLD_CYC");
                button_manage.button_list[i]->event = BUTTON_EVENT_HOLD_CYC;
                if (button_manage.button_list[i]->hold_cyc_period && button_manage.button_list[i]->cnt % (button_manage.button_list[i]->hold_cyc_period / MY_BUTTON_SCAN_SPACE_MS) == 0)
                    MY_BUTTON_CALL(button_manage.button_list[i]->cb, (button_manage.button_list[i]));
            }
        }
        else
        {
            button_manage.button_list[i]->cnt = 0;
            if (cnt_old >= MY_BUTTON_DOWN_MS / MY_BUTTON_SCAN_SPACE_MS && cnt_old < MY_BUTTON_HOLD_MS / MY_BUTTON_SCAN_SPACE_MS) /* BUTTON_CLICK_UP */
            {
                TRACE_D("BUTTON_CLICK_UP");
                button_manage.button_list[i]->event = BUTTON_EVENT_CLICK_UP;
                MY_BUTTON_CALL(button_manage.button_list[i]->cb, (button_manage.button_list[i]));
            }
            else if (cnt_old >= MY_BUTTON_HOLD_MS / MY_BUTTON_SCAN_SPACE_MS) /* BUTTON_HOLD_UP */
            {
                TRACE_D("BUTTON_HOLD_UP");
                button_manage.button_list[i]->event = BUTTON_EVENT_HOLD_UP; 
                MY_BUTTON_CALL(button_manage.button_list[i]->cb, (button_manage.button_list[i]));
            }
        }
    }
}

int my_button_start()
{
	timer_init(&button_manage.timer);
    /* 启动定时器 */
    timer_add_period(&button_manage.timer, /* 周期性定时器 */
    		TICK_PER_SECOND * MY_BUTTON_SCAN_SPACE_MS / 1000, /* 定时长度，以OS Tick为单位，即20个OS Tick */
			my_button_scan, /* 超时时回调的处理函数 */
			NULL); /* 超时函数的入口参数 */
    return 0;
}
