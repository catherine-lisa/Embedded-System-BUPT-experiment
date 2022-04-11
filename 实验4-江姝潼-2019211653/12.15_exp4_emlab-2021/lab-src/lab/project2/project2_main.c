/*
 *      Author: ljp
 */
#include "kern/kern_api.h"
#include "button.h"

void leds_init(void);
void led_set(unsigned int val);
void keys_init(void);
int k3_read(void);
int k4_read(void);
int k5_read(void);
int k6_read(void);

typedef enum {
	LED_ROLL_CLOCKWISE = 0,
	LED_ROLL_ANTI_CLOCKWISE,
	LED_STOP,
	LED_START,
	LED_DIMMING
} led_state_t;

static volatile led_state_t led_state = LED_ROLL_CLOCKWISE;
static int led_period_ticks = 250;

static uint8_t dim_curve[] = { 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2,
		2, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 6, 7, 7, 8, 8, 9, 9,
		10, 10, 11, 11, 12, 13, 14, 14, 15, 16, 16, 15, 14, 14, 13, 12, 11, 11,
		10, 10, 9, 9, 8, 8, 7, 7, 6, 6, 6, 5, 5, 5, 5, 4, 4, 4, 4, 3, 3, 3, 3,
		3, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1 };

static uint16_t dim_curve_len = sizeof(dim_curve) / sizeof(dim_curve[0]);

static void led_dimming(void) {
	int i = 0;
	int max = 17;
	//TODO：根据dim_curve曲线的占空比值控制LED的亮灭时长
}


static void led_blink_task(void) {
	int light_bits[4] = { 0xE, 0xD, 0x7, 0xB }; //[D9..D6], 0 => on, 1 => off
	int light_curr = 0;
	led_state_t light_state = LED_ROLL_CLOCKWISE;

	leds_init();

	while (1) {
		switch (led_state) {
		case LED_ROLL_CLOCKWISE:
			light_state = led_state;
			led_set(light_bits[light_curr]);
			if (light_curr >= 3) {
				light_curr = 0;
			} else {
				light_curr++;
			}
			task_sleep(led_period_ticks);
			break;

		case LED_ROLL_ANTI_CLOCKWISE:
			light_state = led_state;
			led_set(light_bits[light_curr]);
			if (light_curr <= 0) {
				light_curr = 3;
			} else {
				light_curr--;
			}
			task_sleep(led_period_ticks);
			break;

		case LED_STOP:
			break;

		case LED_START:
			led_state = light_state;
			break;

		case LED_DIMMING:
			light_state = led_state;
			led_dimming();
			break;

		default:
			ASSERT(0);
			break;
		}
		task_sleep(1);
	}
}

static button_t K3, K4, K5, K6;

static int read_key(button_t *btn) {
	if (btn == &K3) {
		return k3_read();
	} else if (btn == &K4) {
		return k4_read();
	} else if (btn == &K5) {
		return k5_read();
	} else if (btn == &K6) {
		return k6_read();
	}
	ASSERT(0);
	return 0;
}

static int process_event(button_t *btn) {
	static unsigned int k3_press_count;
	if (btn == &K3) {
		if (button_event_read(btn) == BTN_EVT_CLICK) {
			if (k3_press_count++ & 0x1) {
				led_state = LED_START;
			} else {
				led_state = LED_STOP;
			}
		}
	} else if (btn == &K4) {
		if (button_event_read(btn) == BTN_EVT_DOUBLE_CLICK) {
			led_state = LED_ROLL_CLOCKWISE;
		}
	} else if (btn == &K5) {
		if (button_event_read(btn) == BTN_EVT_DOUBLE_CLICK) {
			led_state = LED_ROLL_ANTI_CLOCKWISE;
		}
	} else if (btn == &K6) {
		if (button_event_read(btn) == BTN_EVT_LONG_HOLD_START) {
			led_state = LED_DIMMING;
		}
		if (button_event_read(btn) == BTN_EVT_LONG_HOLD_UP) {
			led_state = LED_ROLL_CLOCKWISE;
		}
	}
	return 0;
}

static void key_button_init(void) {
	button_register(&K3, "K3", 0, FIR_FILTER, read_key, process_event);
	button_register(&K4, "K4", 0, FIR_FILTER, read_key, process_event);
	button_register(&K5, "K5", 0, FIR_FILTER, read_key, process_event);
	button_register(&K6, "K6", 0, FIR_FILTER, read_key, process_event);
}

static void button_scan_task(void) {
	keys_init();
	key_button_init();

	while (1) {
		button_scan_loop();
		task_sleep(5); // 5 tick == 5ms
	}
}

void project2_main(void) {
	task_create_simple(button_scan_task, 0);
	task_create_simple(led_blink_task, 0);
}
