/**
 * Know nothing about copy from left to right,
 * if someone wanna use it, just copy it from left to right
 *                by author m24h
 */
#include "esp_check.h"
#include "esp_timer.h"
#include "driver/gpio.h"

#include "button.h"

static const char * const TAG = "BUTTON";

esp_err_t button_init(button_t * button, int pin, int time_debounce, int time_long_press, int active_level)
{
	ESP_RETURN_ON_FALSE(button && GPIO_IS_VALID_GPIO(pin)
			, ESP_ERR_INVALID_ARG, TAG, "Invalid argument (%p, %d)", button, pin);

	button->activelevel=active_level?1:0;
	button->debounce=time_debounce;
	button->longpress=time_long_press;
	button->pin=pin;

	esp_err_t ret=ESP_OK;

	gpio_config_t io = {
		.pin_bit_mask =  1ULL<<pin,
		.mode = GPIO_MODE_INPUT,
		.pull_up_en=1-button->activelevel,
		.pull_down_en=button->activelevel,
		.intr_type=GPIO_INTR_DISABLE
	};
	ESP_RETURN_ON_ERROR(ret=gpio_config(&io)
			 , TAG, "Failed to config button GPIO pin (%d:%s)", ret, esp_err_to_name(ret));

	button->last=esp_timer_get_time() & 0xfffffffffffffffe;
	button->last |=(gpio_get_level(pin)?1:0)==button->activelevel;

	return ESP_OK;
}

esp_err_t button_deinit(button_t * button)
{
	ESP_RETURN_ON_FALSE(button
		, ESP_ERR_INVALID_ARG, TAG, "NULL argument");

	gpio_config_t io = {
		.pin_bit_mask = 1ULL<<button->pin,
		.mode = GPIO_MODE_DISABLE ,
		.intr_type=GPIO_INTR_DISABLE
	};

	esp_err_t ret;
	ESP_RETURN_ON_ERROR(ret=gpio_config(&io),
			TAG, "Failed to disable button GPIO pin (%d:%s)", ret, esp_err_to_name(ret));

	return ESP_OK;
}

button_event_e button_scan(button_t * button)
{
	int64_t now=esp_timer_get_time() & 0xfffffffffffffffc;

	/* scan buttons */
	int active=((gpio_get_level(button->pin)?1:0)==button->activelevel)?1:0;
	if ((int)button->last & 2) {
		if ((int)button->last & 1) {
			if (active) {
				if (now > button->last+button->longpress) {
					button->last &=0xfffffffffffffffd;
					return BUTTON_LONG;
				} else if (now > button->last+button->debounce) {
					return BUTTON_ACTIVE;
				} else {
					return BUTTON_UNKNOW;
				}
			} else {
				if (now > button->last+button->debounce) {
					button->last = now | 2l;
					return BUTTON_CLICK;
				} else {
					button->last = now | 2l;
					return BUTTON_UNKNOW;
				}
			}
		} else {
			if (active) {
				button->last = now | 3l;
				return BUTTON_UNKNOW;
			} else {
				if (now > button->last+button->debounce)
					return BUTTON_INACTIVE;
				else
					return BUTTON_UNKNOW;
			}
		}
	} else {
		if ((int)button->last & 1) {
			if (active) {
				if (now > button->last+button->debounce) {
					return BUTTON_ACTIVE;
				} else {
					return BUTTON_UNKNOW;
				}
			} else {
				button->last = now | 2l;
				return BUTTON_UNKNOW;
			}
		} else {
			if (active) {
				button->last = now | 3l;
				return BUTTON_UNKNOW;
			} else {
				if (now > button->last+button->debounce)
					return BUTTON_INACTIVE;
				else
					return BUTTON_UNKNOW;
			}
		}
	}

	return BUTTON_UNKNOW;
}

