/**
 * Know nothing about copy from left to right,
 * if someone wanna use it, just copy it from left to right
 *                by author m24h
 */
#include "driver/adc_common.h"
#include "esp_adc_cal.h"

#include <math.h>

#include "CS1237.h"
#include "rwlock.h"

#include "app.h"
#include "main.h"
#include "samp.h"

const static char * const TAG="SAMP";

static int fail_i=0;
static int fail_v=0;
static int fail_temp=0;

/* for esp32 */
#define ADC_VOLT_REF  1100

#define RAW_SMOOTH_COUNT  8   /* how many points used to calculate raw data for calibration smoothing */

static esp_adc_cal_characteristics_t adc_cal_temp;

/* buffer for low-pass filter of sampling data */
static int     raw_i_off=0;
static int     raw_v_off=0;
static int     raw_t_off=0;
static int32_t raw_i[RAW_SMOOTH_COUNT]={};
static int32_t raw_v[RAW_SMOOTH_COUNT]={};
static int32_t raw_t[RAW_SMOOTH_COUNT]={};

/* average get rid off max and min */
static int32_t calc_raw (int32_t *raw, int *index, int32_t value)
{
	*index=(*index+1) % RAW_SMOOTH_COUNT;
	raw[*index]=value;

	int i;
	int32_t r, raw_min=raw[0], raw_max=raw[0];
	int64_t ret=raw[0];
	for (i=1; i<RAW_SMOOTH_COUNT; i++) {
		r=raw[i];
		ret+=r;
		if (r<raw_min) {
			raw_min=r;
		} else if (r>raw_max) {
			raw_max=r;
		}
	}
	ret-=raw_max;
	ret-=raw_min;
	if (ret>=0) return (int32_t)((ret+(RAW_SMOOTH_COUNT-2)/2)/(RAW_SMOOTH_COUNT-2));
	else return (int32_t)((ret-(RAW_SMOOTH_COUNT-2)/2)/(RAW_SMOOTH_COUNT-2));
}

/**
 * input -2^31 ~ (2^31-1)
 */
static int correct_i(int32_t samp)
{
	CONF_LOCK_R();
	samp=conf_cal(&conf_vars.cal.isamp, samp);
	CONF_UNLOCK_R();

	return samp;
}

/**
 * input -2^31 ~ (2^31-1)
 */
static int correct_v(int32_t samp)
{
	CONF_LOCK_R();
	samp=conf_cal(&conf_vars.cal.vsamp, samp);
	CONF_UNLOCK_R();

	return samp;
}

/**
 *  input is volt in mV
 */
static int correct_temp(int32_t samp)
{
	/*
	 * temperature signal is Vtemp=Vfan*0.81k/(0.81k+Rthermistor[4.7k])*15k/115k
	 * Rthermistor = Rn * e ^ (B*(1/T-1/Tn))
	 * Rn=4.7k, and I guess, and normally, B=3950, Tn=273.15+25
	 * So Rthermistor = 4.7k * e^ 3950*(1/T-1/298.15)
	 * when T is in range 0 ~ 50 ~ 100 ~ 150 Celsius degree
	 * Rthermistor = 15.8k ~ 1.69k ~ 0.328k ~ 0.09385k ( in datasheet: 15.3k ~ 1.69k ~ 0.305k ~ 0.077k)
	 * So Vtemp = Vfan * (0.049 ~ 0.32 ~ 0.73 ~ 0.91) * 0.13
	 * if Vfan= 15V (12V/0.5A fan full running tied with 5.1 series resistors, need 15V power)
	 * Vtemp = 0.095 ~ 0.50 ~ 1.4 ~ 1.8 (V) at 0~50~100~150 Celsius degree
	 * ZXD2400 will stop if Vfan*0.81k/(0.81k+Rthermistor) is bigger than (8*604+Vfan*149)/753=9.39V (start again at 6.42V)
	 * in which condition, T=87 Celsius degree
	 * the result seens to prove my conjecture
	 *
	 * V=Vfan*0.81k/(0.81k+Rthermistor[4.7k])*15k/115k
	 * Vfan=15V
	 * so V=Vfan*0.105652/(0.81+4.7*e^(3950*(1/T-1/298.15)))
	 * so T=1/(ln((Vfan*0.105652/V-0.81)/4.7)/3950+0.003354016)
	 * and temperature in celsius degree is T-273.15
	 */
	if (samp<50) return 0;

	double s;
	s=1000/(log((105.652*VFAN/samp-0.81)/4.7)/3950.0+0.003354016)-273150;

	return (int)s;
}

esp_err_t samp_init()
{
	esp_err_t ret;

	uint8_t newcfg;
    uint8_t cfg=CS1237_REFO_OFF|CS1237_SPEED_40|CS1237_PGA_1;

	ESP_RETURN_ON_FALSE((ret=CS1237_init_pin(PIN_ISAMPK, PIN_ISAMPD))==ESP_OK
			&& (ret=CS1237_write_config(PIN_ISAMPK, PIN_ISAMPD, NULL, NULL, cfg, 100))==ESP_OK
			&& (ret=CS1237_read_config(PIN_ISAMPK, PIN_ISAMPD, NULL, NULL, &newcfg, 100))==ESP_OK
            && (ret=(newcfg & (CS1237_REFO|CS1237_SPEED|CS1237_PGA))==cfg?ESP_OK:ESP_ERR_INVALID_STATE)==ESP_OK
		, ret, TAG, "Failed to config CS1237:ISAMP (%d:%s)", ret, esp_err_to_name(ret));

	ESP_RETURN_ON_FALSE((ret=CS1237_init_pin(PIN_VSAMPK, PIN_VSAMPD))==ESP_OK
			&& (ret=CS1237_write_config(PIN_VSAMPK, PIN_VSAMPD, NULL, NULL, cfg, 100))==ESP_OK
			&& (ret=CS1237_read_config(PIN_VSAMPK, PIN_VSAMPD, NULL, NULL, &newcfg, 100))==ESP_OK
			&& (ret=(newcfg & (CS1237_REFO|CS1237_SPEED|CS1237_PGA))==cfg?ESP_OK:ESP_ERR_INVALID_STATE)==ESP_OK
		, ret, TAG, "Failed to config CS1237:VSAMP (%d:%s)", ret, esp_err_to_name(ret));

	ESP_RETURN_ON_FALSE((ret=adc1_config_width(ADC_WIDTH_BIT_12))==ESP_OK
			&& (ret=adc1_config_channel_atten(ADC1_CHAN_TEMP, ADC_ATTEN_DB_11))==ESP_OK
		, ret, TAG, "Failed to config ADC1 channel (%d:%s)", ret, esp_err_to_name(ret));

	esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, ADC_VOLT_REF, &adc_cal_temp);

	return ret;
}

void samp_samp ()
{
	int32_t samp;
	esp_err_t ret2;

	/* should get before 10 ticks */
	if ((ret2=CS1237_data(PIN_ISAMPK, PIN_ISAMPD, &samp, NULL, 10))==ESP_OK) {
		main_vars.raw_ic=calc_raw(raw_i, &raw_i_off, samp);
		main_vars.ic=correct_i(main_vars.raw_ic);
		fail_i=0;
	} else {
    	ESP_LOGW(TAG, "Failed to sample I (%d:%s)", ret2, esp_err_to_name(ret2));
		fail_i++;
	}

	if ((ret2=CS1237_data(PIN_VSAMPK, PIN_VSAMPD, &samp, NULL, 10))==ESP_OK) {
		main_vars.raw_vc=calc_raw(raw_v, &raw_v_off, samp);
		main_vars.vc=correct_v(main_vars.raw_vc);
		fail_v=0;
	} else {
    	ESP_LOGW(TAG, "Failed to sample V (%d:%s)", ret2, esp_err_to_name(ret2));
		fail_v++;
	}

	if ((ret2=esp_adc_cal_get_voltage(ADC1_CHAN_TEMP, &adc_cal_temp, (uint32_t*)&samp))==ESP_OK) {
		samp=calc_raw(raw_t, &raw_t_off, (int32_t)samp);
		main_vars.temp=correct_temp(samp);
		fail_temp=0;
	} else {
    	ESP_LOGW(TAG, "Failed to sample temperature (%d:%s)", ret2, esp_err_to_name(ret2));
		fail_temp++;
	}

	if (fail_i>MAX_RECOVERABLE_FAILURES
	 || fail_v>MAX_RECOVERABLE_FAILURES
	 || fail_temp>MAX_RECOVERABLE_FAILURES) {
		ESP_LOGE(TAG, "Too many sampling failures (%d, %d, %d)", fail_i, fail_v, fail_temp);
		app_reset();
	}
}
