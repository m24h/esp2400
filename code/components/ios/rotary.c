/**
 * Know nothing about copy from left to right,
 * if someone wanna use it, just copy it from left to right
 *                by author m24h
 */
#include "esp_check.h"
#include "esp_timer.h"
#include "driver/gpio.h"

#include "rotary.h"

static const char * const TAG = "ROTARY";

static void IRAM_ATTR isr_a(void * arg)
{
	rotary_t * rot=(rotary_t *) arg;
	BaseType_t wk=pdFALSE;

	int64_t now=esp_timer_get_time() & 0xfffffffffffffffe;
	int level=gpio_get_level(rot->pin_b)?1:0;
	if (level || rot->type==ROTARY_1P2S || rot->type==ROTARY_1P2S_REVERSE) {
		if (now > rot->last_time_a + rot->time_debounce) {
			int reversed=(rot->type>=ROTARY_1P1S_REVERSE)?1:0;
			int last_level_a=(int)rot->last_time_a & 1;

			if (now > rot->last_time_a + rot->time_fast)
				rot->fast_cnt=0;
			else if (rot->fast_cnt < rot->fast_repeat)
				rot->fast_cnt++;
			else {
				if (rot->e_loop)
					esp_event_isr_post_to(rot->e_loop, rot->e_base,
						(reversed ^ level ^ last_level_a)?rot->e_cw_fast:rot->e_ccw_fast,
						NULL, 0, &wk);
				else
					esp_event_isr_post(rot->e_base,
						(reversed ^ level ^ last_level_a)?rot->e_cw_fast:rot->e_ccw_fast,
						NULL, 0, &wk);
				goto end;
			}

			if (rot->e_loop)
				esp_event_isr_post_to(rot->e_loop, rot->e_base,
					(reversed ^ level ^ last_level_a)?rot->e_cw:rot->e_ccw,
					NULL, 0, &wk);
			else
				esp_event_isr_post(rot->e_base,
					(reversed ^ level ^ last_level_a)?rot->e_cw:rot->e_ccw,
					NULL, 0, &wk);

		end:
			rot->last_time_a=now | (last_level_a ^ 1); //update time, and level is of cause changed
		}
	}

	if (wk!=pdFALSE) portYIELD_FROM_ISR();
}

static void IRAM_ATTR isr_b(void * arg)
{
	rotary_t * rot=(rotary_t *) arg;
	int level=gpio_get_level(rot->pin_a)?1:0;
	rot->last_time_a = (rot->last_time_a & 0xfffffffffffffffe) | level;
}

esp_err_t rotary_init(rotary_t * rotary, rotary_type_e type, int active_level,
		int pin_a, int pin_b, int time_debounce, int time_fast, int fast_repeat,
		esp_event_loop_handle_t e_loop, esp_event_base_t e_base,
		int32_t e_cw, int32_t e_ccw, int32_t e_cw_fast, int32_t e_ccw_fast)
{
	ESP_RETURN_ON_FALSE(rotary && GPIO_IS_VALID_GPIO(pin_a) && GPIO_IS_VALID_GPIO(pin_b)
			, ESP_ERR_INVALID_ARG, TAG, "Invalid argument (%p,%d,%d)", rotary, pin_a, pin_b);

	rotary->type=type;
	rotary->activelevel=active_level?1:0;
	rotary->pin_a=pin_a;
	rotary->pin_b=pin_b;
	rotary->time_debounce=time_debounce;
	rotary->time_fast=time_fast;
	rotary->fast_repeat=fast_repeat;
	rotary->e_loop=e_loop;
	rotary->e_base=e_base;
	rotary->e_cw=e_cw;
	rotary->e_ccw=e_ccw;
	rotary->e_cw_fast=e_cw_fast;
	rotary->e_ccw_fast=e_ccw_fast;
	rotary->fast_cnt=0;

	/* if exists */
	gpio_isr_handler_remove(pin_a);
	gpio_isr_handler_remove(pin_b);

	esp_err_t ret;
	gpio_config_t io = {
		.pin_bit_mask =  (1ULL<<pin_a) | (1ULL<<pin_b),
		.mode = GPIO_MODE_INPUT,
		.pull_up_en=1-rotary->activelevel,
		.pull_down_en=rotary->activelevel,
		.intr_type=GPIO_INTR_ANYEDGE
	};
	ESP_RETURN_ON_ERROR(ret=gpio_config(&io)
			 , TAG, "Failed to config rotary GPIO pin (%d:%s)", ret, esp_err_to_name(ret));

	rotary->last_time_a=esp_timer_get_time() & 0xfffffffffffffffel;
	rotary->last_time_a|=gpio_get_level(pin_a);

	ESP_RETURN_ON_FALSE((ret=gpio_isr_handler_add(pin_a, &isr_a, rotary))==ESP_OK
    			   && (ret=gpio_isr_handler_add(pin_b, &isr_b, rotary))==ESP_OK
   		, ret, TAG, "Failed to install ISR for rotary (%d:%s)", ret, esp_err_to_name(ret));

	return ESP_OK;
}

esp_err_t rotary_deinit(rotary_t * rotary)
{
	ESP_RETURN_ON_FALSE(rotary
		, ESP_ERR_INVALID_ARG, TAG, "NULL argument");

	gpio_isr_handler_remove(rotary->pin_a);
	gpio_isr_handler_remove(rotary->pin_b);

	esp_err_t ret;
	gpio_config_t io = {
		.pin_bit_mask = (1ULL<<rotary->pin_a) | (1ULL<<rotary->pin_b),
		.mode = GPIO_MODE_DISABLE ,
		.intr_type=GPIO_INTR_DISABLE
	};
	ESP_RETURN_ON_ERROR(ret=gpio_config(&io),
			TAG, "Failed to disable rotary GPIO pin (%d:%s)", ret, esp_err_to_name(ret));

	return ESP_OK;
}
