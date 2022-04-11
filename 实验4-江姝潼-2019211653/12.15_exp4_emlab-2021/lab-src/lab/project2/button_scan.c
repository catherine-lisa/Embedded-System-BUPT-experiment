/*
 *      Author: ljp
 */
#include <stdint.h>
#include "kern/queue.h"
#include "kern/debug.h"
#include "button.h"

#define LOCAL_TRACE 1

extern uint32_t HAL_GetTick(void);

static TAILQ_HEAD(button_list_t, button)
_button_list = TAILQ_HEAD_INITIALIZER(_button_list);

void lib_button_init() {
	TAILQ_INIT(&_button_list);
}

#define EVENT_STRING(e) (#e)

static const char *button_event_string[] = { EVENT_STRING(BTN_EVT_DOWN),
		EVENT_STRING(BTN_EVT_CLICK), EVENT_STRING(BTN_EVT_DOUBLE_CLICK),
		EVENT_STRING(BTN_EVT_MULTI_CLICK), EVENT_STRING(BTN_EVT_LONG_START),
		EVENT_STRING(BTN_EVT_LONG_UP), EVENT_STRING(BTN_EVT_LONG_HOLD_START),
		EVENT_STRING(BTN_EVT_LONG_HOLD_UP), };

void button_register(button_t *btn, const char *name, int cfg_pressed_level, debounce_method_t cfg_debounce, 
		button_callback_t read_button, button_callback_t on_event) {
	btn->name = name;
	btn->cfg_pressed_level = !!cfg_pressed_level;
	btn->read_button = read_button;
	btn->on_event = on_event;
	btn->output_level = !cfg_pressed_level; //guard
	btn->last_read_level = btn->output_level;

	TAILQ_INSERT_TAIL(&_button_list, btn, link);
}


#define FILTER_ORDER 4
static int fir_history[FILTER_ORDER];
static int fir_idx = 0;

//scan one button
void scan_button(button_t *btn) {
	//reset edge detect
	btn->output_edge = 0;

	if (!btn->read_button) {
		return;
	}

	unsigned int now = HAL_GetTick();
	//detecting button press down/up edge
	unsigned int curr_read_level = !!btn->read_button(btn); //0 or 1

	if(btn->debounce == DELAY_SAMPLE) {
		//detecting edge
		if (curr_read_level != btn->last_read_level) {
			btn->last_read_level = curr_read_level;
			btn->last_read_level_ms = now;
			return;
		}
		//debouncing
		if ((now - btn->last_read_level_ms) < CHATTER_WINDOW_MS) {
			return;
		}
	} 

	if(btn->debounce == FIR_FILTER) {
		//TODO：实现递推平均滤波法输出按键值
	}
	
	//reset internal state
	btn->last_read_level = curr_read_level;

	//signal stable now, output stable state
	btn->output_edge = btn->output_level ^ curr_read_level;
	if (btn->output_edge) {
		btn->last_edge_ms = btn->output_edge_ms;
		btn->output_edge_ms = now;
	}
	btn->output_level = curr_read_level;
	btn->output_level_ms = now;
}

static inline void fire_event(button_t *btn, button_event_t evt) {
	TRACEK(
			"Button %s Event [%d - %24s] edge %d level %d time[%d %d %d] repeat %d\n",
			btn->name, evt, button_event_string[evt], btn->output_edge,
			btn->output_level, btn->last_edge_ms, btn->output_edge_ms,
			btn->output_level_ms, btn->click_count);
	btn->output_event = evt;
	if (btn->on_event) {
		btn->on_event(btn);
	}
}

static inline void fire_event_if_not(button_t *btn, button_event_t evt) {
	if (btn->output_event != evt) {
		fire_event(btn, evt);
	}
}

void generate_button_event(button_t *btn) {
	//detect signal and handle by state
	if (btn->output_level == btn->cfg_pressed_level) {	//button pressed
		if (btn->output_edge) { //detected press edge
			fire_event(btn, BTN_EVT_DOWN);
		}
		if ((btn->output_level_ms - btn->output_edge_ms) > PRESS_TIME_MS_HOLD) { //long hold
			fire_event_if_not(btn, BTN_EVT_LONG_HOLD_START);
		} else if ((btn->output_level_ms - btn->output_edge_ms)
				> PRESS_TIME_MS_LONG) { // long press
			fire_event_if_not(btn, BTN_EVT_LONG_START);
		}
	} else { //button released
		if (btn->output_edge) { //detected release edge
			if ((btn->output_edge_ms - btn->last_edge_ms) > PRESS_TIME_MS_HOLD) {
				fire_event(btn, BTN_EVT_LONG_HOLD_UP);
			} else if ((btn->output_edge_ms - btn->last_edge_ms)
					> PRESS_TIME_MS_LONG) {
				fire_event(btn, BTN_EVT_LONG_UP);
			} else { //need more time to distinguish short click vs multi click
				btn->click_count++;
			}
		}
		if ((btn->output_level_ms - btn->output_edge_ms)
				> MULTI_CLICK_INTERVAL_MS_MAX) { //no more multi click
			if (btn->click_count == 1) {
				fire_event(btn, BTN_EVT_CLICK);
			} else if (btn->click_count == 2) {
				fire_event(btn, BTN_EVT_DOUBLE_CLICK);
			} else if (btn->click_count > 2) {
				fire_event(btn, BTN_EVT_MULTI_CLICK);
			}
			btn->click_count = 0;
		}
	}
}

void scan_buttons(void) {
	button_t *btn;
	TAILQ_FOREACH(btn, &_button_list, link)
	{
		scan_button(btn);
	}
}

void process_buttons(void) {
	button_t *btn;
	TAILQ_FOREACH(btn, &_button_list, link)
	{
		generate_button_event(btn);
	}
}

void button_scan_loop(void) {
	scan_buttons();
	process_buttons();
}
