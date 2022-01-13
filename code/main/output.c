/**
 * Know nothing about copy from left to right,
 * if someone wanna use it, just copy it from left to right
 *                by author m24h
 */
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/dac.h"

#include "app.h"
#include "main.h"
#include "output.h"

const static char * const TAG="OUTPUT";

#define TIMER_IV           LEDC_TIMER_0
#define CHANNEL_I          LEDC_CHANNEL_0
#define CHANNEL_V          LEDC_CHANNEL_1
#define FREQUENCY_IV       2400 /* 2.4kHz and 14bits resolution, nearly using 40MHz clock */
#define RESOLUTION_IV      14

#define TIMER_CTL          LEDC_TIMER_1
#define CHANNEL_FAN        LEDC_CHANNEL_2
#define CHANNEL_DISCHARGE  LEDC_CHANNEL_3
#define FREQUENCY_CTL      20000  /* 25kHz and 8bits resolution, nearly using 5MHz clock */

static int fail_i=0;
static int fail_v=0;
static int fail_fan=0;
static int fail_discharge=0;

static int32_t last_is=0;
static int32_t last_vs=0;
static int last_fan=0;
static int last_discharge=0;

static int64_t last_time=0;

/**
 * should return 0-(2^31-1)
 */
static int32_t correct_i(int outp)
{
	if (outp<I_MIN) outp=I_MIN;
	else if (outp>I_MAX) outp=I_MAX;

	CONF_LOCK_R();
	outp=conf_cal(&conf_vars.cal.iout, outp);
	CONF_UNLOCK_R();

	return outp;
}

/**
 * should return 0-(2^31-1)
 */
static int32_t correct_v(int outp)
{
	if (outp<V_MIN) outp=V_MIN;
	else if (outp>V_MAX) outp=V_MAX;

	CONF_LOCK_R();
	outp=conf_cal(&conf_vars.cal.vout, outp);
	CONF_UNLOCK_R();

	return outp;
}

esp_err_t output_init()
{
	esp_err_t ret;

	ledc_timer_config_t ledc_timer = {
		.speed_mode       = LEDC_HIGH_SPEED_MODE,
		.timer_num        = TIMER_IV,
		.duty_resolution  = RESOLUTION_IV,
		.freq_hz          = FREQUENCY_IV,
		.clk_cfg          = LEDC_APB_CLK
	};
	ESP_RETURN_ON_ERROR(ret=ledc_timer_config(&ledc_timer)
			, TAG, "Failed to config I/V PWM timer (%d:%s)", ret, esp_err_to_name(ret));

    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_HIGH_SPEED_MODE,
        .channel        = CHANNEL_I,
        .timer_sel      = TIMER_IV,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = PIN_IPWM,
        .duty           = 0,
        .hpoint         = 0
    };
	ESP_RETURN_ON_ERROR(ret=ledc_channel_config(&ledc_channel)
			, TAG, "Failed to config I PWM channel (%d:%s)", ret, esp_err_to_name(ret));

	ledc_channel.gpio_num=PIN_VPWM;
	ledc_channel.channel=CHANNEL_V;
	ESP_RETURN_ON_ERROR(ret=ledc_channel_config(&ledc_channel)
			, TAG, "Failed to config V PWM channel (%d:%s)", ret, esp_err_to_name(ret));

#if FAN_PWM || DISCHARGE_PWM
	ledc_timer.timer_num = TIMER_CTL;
	ledc_timer.freq_hz = FREQUENCY_CTL;
	ledc_timer.duty_resolution = 8;
	ESP_RETURN_ON_ERROR(ret=ledc_timer_config(&ledc_timer)
			, TAG, "Failed to config Fan/Discharge PWM timer (%d:%s)", ret, esp_err_to_name(ret));
#else
	gpio_config_t iocfg = {
		.mode = GPIO_MODE_OUTPUT
	};
#endif

#if FAN_PWM
	ledc_channel.timer_sel= TIMER_CTL;
	ledc_channel.gpio_num=PIN_FAN;
	ledc_channel.channel=CHANNEL_FAN;
	ESP_RETURN_ON_ERROR(ret=ledc_channel_config(&ledc_channel)
			, TAG, "Failed to config FAN PWM channel (%d:%s)", ret, esp_err_to_name(ret));
#else
	iocfg.pin_bit_mask = (1ULL<<PIN_FAN);
	ESP_RETURN_ON_FALSE((ret=gpio_config(&iocfg))==ESP_OK
   		, ret, TAG, "Failed to config FAN GPIO pins (%d:%s)", ret, esp_err_to_name(ret));
#endif

#if DISCHARGE_PWM
	ledc_channel.timer_sel= TIMER_CTL;
	ledc_channel.gpio_num=PIN_DISCHARGE;
	ledc_channel.channel=CHANNEL_DISCHARGE;
	ESP_RETURN_ON_ERROR(ret=ledc_channel_config(&ledc_channel)
			, TAG, "Failed to config DISCHARGE PWM channel (%d:%s)", ret, esp_err_to_name(ret));
#elif DISCHARGE_DAC
	ESP_RETURN_ON_ERROR(ret=dac_output_enable(DAC_DISCHARGE)
			, TAG, "Failed to config DISCHARGE DAC channel (%d:%s)", ret, esp_err_to_name(ret));
#else
	iocfg.pin_bit_mask = (1ULL<<PIN_DISCHARGE);
	ESP_RETURN_ON_FALSE((ret=gpio_config(&iocfg))==ESP_OK
   		, ret, TAG, "Failed to config DISCHARGE GPIO pins (%d:%s)", ret, esp_err_to_name(ret));
#endif

	last_is=0;
	last_vs=0;
	last_fan=0;
	last_discharge=0;

	return ESP_OK;
}

void output_out ()
{
	int toset;
	int32_t raw;
	esp_err_t ret2;

	int64_t now=esp_timer_get_time();

	/* ignore first time */
	if (last_time==0) {
		last_time=now;
		return;
	}

	/* considering on/off status and fade effect */
	if (main_vars.on) {
		if (main_vars.is>last_is) {
			if (conf_vars.i_rise>0) {
				toset=last_is+(int)(conf_vars.i_rise*(now-last_time)/1000000)+1;
				if (toset>main_vars.is || toset<last_is)
					toset=main_vars.is;
			} else
				toset=main_vars.is;
		} else if (main_vars.is<last_is) {
			if (conf_vars.i_fall>0) {
				toset=last_is-(int)(conf_vars.i_fall*(now-last_time)/1000000)-1;
				if (toset<main_vars.is || toset>last_is)
					toset=main_vars.is;
			} else
				toset=main_vars.is;
		} else
			toset=main_vars.is;

		if (toset>I_MAX)
			toset=I_MAX;
		else if (toset<I_MIN)
			toset=I_MIN;
	} else
		toset=I_OFF;

	raw=correct_i(toset);
	if (main_vars.raw_is!=raw || fail_i>0) {
		if ((ret2=ledc_set_duty(LEDC_HIGH_SPEED_MODE, CHANNEL_I, raw>>(31-RESOLUTION_IV)))==ESP_OK
		 && (ret2=ledc_update_duty(LEDC_HIGH_SPEED_MODE, CHANNEL_I))==ESP_OK) {
			main_vars.raw_is=raw;
			last_is=toset;
			fail_i=0;
		} else {
			fail_i++;
			ESP_LOGW(TAG, "Failed to set I PWM duty (%d:%s)", ret2, esp_err_to_name(ret2));
		}
	}

	/* considering on/off status and fade effect */
	if (main_vars.on) {
		if (main_vars.vs>last_vs) {
			if (conf_vars.v_rise>0) {
				toset=last_vs+(int)(conf_vars.v_rise*(now-last_time)/1000000)+1;
				if (toset>main_vars.vs || toset<last_vs)
					toset=main_vars.vs;
			} else
				toset=main_vars.vs;
		} else if (main_vars.vs<last_vs) {
			if (conf_vars.v_fall>0) {
				toset=last_vs-(int)(conf_vars.v_fall*(now-last_time)/1000000)-1;
				if (toset<main_vars.vs || toset>last_vs)
					toset=main_vars.vs;
			} else
				toset=main_vars.vs;
		} else
			toset=main_vars.vs;

		if (toset>V_MAX)
			toset=V_MAX;
		else if (toset<V_MIN)
			toset=V_MIN;

	} else
		toset=V_OFF;

	raw=correct_v(toset);
	if (main_vars.raw_vs!=raw || fail_v>0) {
		if ((ret2=ledc_set_duty(LEDC_HIGH_SPEED_MODE, CHANNEL_V, raw>>(31-RESOLUTION_IV)))==ESP_OK
		 && (ret2=ledc_update_duty(LEDC_HIGH_SPEED_MODE, CHANNEL_V))==ESP_OK) {
			main_vars.raw_vs=raw;
			last_vs=toset;
			fail_v=0;
		} else {
			fail_v++;
			ESP_LOGW(TAG, "Failed to set V PWM duty (%d:%s)", ret2, esp_err_to_name(ret2));
		}
	}

#if FAN_PWM
	/* fans are nominal 0.5A but in fact 0.3A, show compress 50%-100 to %75-100% */
	if (main_vars.fan<32) toset=22;
	else if (main_vars.fan<192) toset=(main_vars.fan+main_vars.fan)/3;
	else if (main_vars.fan<255) toset=((main_vars.fan-192)<<1) + 128;
	else toset=255;
	if (toset!=last_fan || fail_fan>0) {
		if ((ret2=ledc_set_duty(LEDC_HIGH_SPEED_MODE, CHANNEL_FAN, toset))==ESP_OK
		 && (ret2=ledc_update_duty(LEDC_HIGH_SPEED_MODE, CHANNEL_FAN))==ESP_OK) {
			last_fan=toset;
			fail_fan=0;
		} else {
			fail_fan++;
			ESP_LOGW(TAG, "Failed to set FAN PWM duty to %d/255 (%d:%s)", toset, ret2, esp_err_to_name(ret2));
		}
	}
#else
	toset=main_vars.fan>127?1:0;
	if (toset!=last_fan || fail_fan>0) {
		if ((ret2=gpio_set_level(PIN_FAN, toset))==ESP_OK) {
			last_fan=toset;
			fail_fan=0;
		} else {
			fail_fan++;
			ESP_LOGW(TAG, "Failed to set FAN level to %d (%d:%s)", toset, ret2, esp_err_to_name(ret2));
		}
	}
#endif

#if DISCHARGE_PWM
	toset=main_vars.discharge<0?0:(main_vars.discharge>255?255:main_vars.discharge);
	if (output_sync || toset!=last_discharge || fail_discharge>0) {
		if ((ret2=ledc_set_duty(LEDC_HIGH_SPEED_MODE, CHANNEL_DISCHARGE, toset)==ESP_OK
		 && (ret2=ledc_update_duty(LEDC_HIGH_SPEED_MODE, CHANNEL_DISCHARGE))==ESP_OK) {
			last_discharge=toset;
			fail_discharge=0;
		} else {
			fail_discharge++;
			ESP_LOGW(TAG, "Failed to set DISCHARGE PWM duty to %d/255 (%d:%s)", toset, ret2, esp_err_to_name(ret2));
		}
	}
#elif DISCHARGE_DAC
	toset=main_vars.discharge<0?0:(main_vars.discharge>255?255:main_vars.discharge);
	if (toset!=last_discharge || fail_discharge>0) {
		if ((ret2=dac_output_voltage(DAC_DISCHARGE, (uint8_t)toset))==ESP_OK) {
			last_discharge=toset;
			fail_discharge=0;
		} else {
			fail_discharge++;
			ESP_LOGW(TAG, "Failed to set DISCHARGE DAC value to %d/255 (%d:%s)", toset, ret2, esp_err_to_name(ret2));
		}
	}
#else
	toset=main_vars.discharge>127?1:0;
	if (toset!=last_discharge || fail_discharge>0) {
		if ((ret2=gpio_set_level(PIN_DISCHARGE, toset))==ESP_OK) {
			last_discharge=toset;
			fail_discharge=0;
		} else {
			fail_discharge++;
			ESP_LOGW(TAG, "Failed to set DISCHARGE level to %d (%d:%s)", toset, ret2, esp_err_to_name(ret2));
		}
	}
#endif

	last_time=now;

	if (fail_i>MAX_RECOVERABLE_FAILURES
	 || fail_v>MAX_RECOVERABLE_FAILURES
	 || fail_fan>MAX_RECOVERABLE_FAILURES
	 || fail_discharge>MAX_RECOVERABLE_FAILURES) {
		ESP_LOGE(TAG, "Too many outputing failures (%d, %d, %d, %d)", fail_i, fail_v, fail_fan, fail_discharge);
		app_reset();
	}
}
