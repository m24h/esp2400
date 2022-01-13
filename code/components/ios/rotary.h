/**
 * Know nothing about copy from left to right,
 * if someone wanna use it, just copy it from left to right
 *                by author m24h
 */
#pragma once

#include "esp_event.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * rotary type : 1 pulse per step / 1 pulse 2 steps
 * and it is maybe reversed from different manufacturer
 */
typedef enum {
	ROTARY_1P1S=0,
	ROTARY_1P2S,
	ROTARY_1P1S_REVERSE,
	ROTARY_1P2S_REVERSE,
} rotary_type_e;

/**
 * button object of config and state
 */
typedef struct {
	rotary_type_e type;
	int           activelevel;
	int           pin_a;
	int           pin_b;
	int           time_debounce;
	int           time_fast;
	int           fast_repeat;
	esp_event_loop_handle_t e_loop;
	esp_event_base_t        e_base;
	int32_t                 e_cw;
	int32_t                 e_ccw;
	int32_t                 e_cw_fast;
	int32_t                 e_ccw_fast;
	int64_t last_time_a;   /* combine of last time and last a level */
	int64_t fast_cnt;
} rotary_t;

/**
 * init and config GPIO for rotary (including pull-up/pull-down resistors)
 * then install rotary ISR
 * [time_debounce] is in micro-second
 * if rotation occured, [e_base]:[e_cw] or [e_base]:[e_ccw] event will be sent to [e_loop]
 * if [e_loop] is NULL, events will be post to the default event loop
 * [time_fast] is the microseconds of interval specified for judgement of FAST rotating,
 * if it is FAST rotating, and it happened [fast_repeat] times before, [e_base]:[e_cw_last] or [e_base]:[e_ccw_fast] is send instead of non-fast events
 * NOTE: gpio_install_isr_service() should be called first somewhere for ISR per pin
 *       and ESP_INTR_FLAG_IRAM flag shoudl not be used in that call
 */
esp_err_t rotary_init(rotary_t * rotary, rotary_type_e type, int active_level,
		int pin_a, int pin_b, int time_debounce, int time_fast, int fast_repeat,
		esp_event_loop_handle_t e_loop, esp_event_base_t e_base,
		int32_t e_cw, int32_t e_ccw, int32_t e_cw_fast, int32_t e_ccw_fast);

/**
 * uninstall rotary ISR
 * disable pins' GPIO
 */
esp_err_t rotary_deinit(rotary_t * rotary);

#ifdef __cplusplus
}
#endif
