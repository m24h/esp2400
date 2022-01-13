/**
 * Know nothing about copy from left to right,
 * if someone wanna use it, just copy it from left to right
 *                by author m24h
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	BUTTON_FAIL=0,
	BUTTON_UNKNOW,
	BUTTON_INACTIVE,
	BUTTON_ACTIVE,
	BUTTON_CLICK,
	BUTTON_LONG
} button_event_e;

/**
 * button object of config and state
 */
typedef struct {
	int pin;
	int debounce;
	int longpress;
	int activelevel;
	int64_t last;
} button_t;

/**
 * init button state including GPIO config
 * [button] should point to a button_t struct
 * [active_level] means the level of button pressing
 * opposite pull-up/pull-down resistor will be configured
 * [time_debounce] and [time_long_press] is in microsecond
 */
esp_err_t button_init(button_t * button, int pin, int time_debounce, int time_long_press, int active_level);

/**
 * free allocated button resources and disable the GPIO pin
 * and set *[button] to NULL
 */
esp_err_t button_deinit(button_t * button);

/**
 * scan for button state or events of click/long press
 */
button_event_e button_scan(button_t * button);

#ifdef __cplusplus
}
#endif
