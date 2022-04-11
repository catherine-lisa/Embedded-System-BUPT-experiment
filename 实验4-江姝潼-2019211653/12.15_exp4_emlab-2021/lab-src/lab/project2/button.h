/*
 * button.h
 *
 */

#ifndef PROJECT2_BUTTON_H_
#define PROJECT2_BUTTON_H_

#define CHATTER_WINDOW_MS           30	        	//ms
#define MULTI_CLICK_INTERVAL_MS_MAX 300	        	//0..MAX: a multi clock, > max:a short click
#define PRESS_TIME_MS_SHORT         100	        	//period for a short push
#define PRESS_TIME_MS_LONG          1000	        //period for a long push
#define PRESS_TIME_MS_HOLD          2000	        //period for a push and hold

struct button;
typedef int (*button_callback_t)(struct button*);

typedef enum {
	BTN_EVT_DOWN = 0,
	BTN_EVT_CLICK,
	BTN_EVT_DOUBLE_CLICK,
	BTN_EVT_MULTI_CLICK,
	BTN_EVT_LONG_START,
	BTN_EVT_LONG_UP,
	BTN_EVT_LONG_HOLD_START,
	BTN_EVT_LONG_HOLD_UP,
	BTN_EVT_MAX,
} button_event_t;

typedef enum {
    DELAY_SAMPLE = 0,
    FIR_FILTER
} debounce_method_t;

typedef struct button {
	TAILQ_ENTRY(button) link;	//link all buttons
	const char *name;
	button_callback_t read_button;  //user privided info
	button_callback_t on_event;

	unsigned char cfg_pressed_level :1;    //1 means key up
	unsigned char last_read_level 	:1;
	unsigned char output_level 		:1;    //output info
	unsigned char output_edge 		:1;
	unsigned char output_event 		:4;

	unsigned int output_level_ms;   //time info
	unsigned int output_edge_ms;
	unsigned int last_edge_ms;
	unsigned int last_read_level_ms;

    debounce_method_t debounce;

	unsigned int click_count;
} button_t;

static inline button_event_t button_event_read(button_t *btn) {
	return (button_event_t) (btn->output_event);
}

void button_register(button_t *btn, const char *name, int cfg_pressed_level, debounce_method_t cfg_debounce,
		button_callback_t read_button, button_callback_t on_event);
void button_scan_loop(void);

#endif /* PROJECT2_BUTTON_H_ */
