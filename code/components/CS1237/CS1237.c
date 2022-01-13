/**
 * Know nothing about copy from left to right,
 * if someone wanna use it, just copy it from left to right
 *                by author m24h
 */
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_check.h"
#include "driver/gpio.h"

/* using esp_rom_delay_us */
/*#include "esp_rom/esp_rom_sys.h" */

#include "CS1237.h"

static const char * const TAG = "CS1237";

static portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

/** CS1237 clock keep high/low both in 450 ESP32 CPU clock, nearly 2.8us  when ESP32 in 160MHz
 * to avoid verflow and (maybe) ESP32 internal using CCOUNT (like UART), multi wait
 * [start] and [cur] should be type of uint32_t
 */
#define DELAY_START(start) \
	    __asm__ __volatile__ ("rsr %0,ccount" : "=r"(start));
#define DELAY_WAIT(start, cur)  \
		do {__asm__ __volatile__ ("rsr %0,ccount" : "=r"(cur));} while(cur-start<150u); \
		start=cur; \
		do {__asm__ __volatile__ ("rsr %0,ccount" : "=r"(cur));} while(cur-start<150u); \
		start=cur; \
        do {__asm__ __volatile__ ("rsr %0,ccount" : "=r"(cur));} while(cur-start<150u); \
        start=cur;

esp_err_t CS1237_init_pin(int pin_clk, int pin_data)
{
	ESP_RETURN_ON_FALSE (GPIO_IS_VALID_OUTPUT_GPIO(pin_clk)
			          && GPIO_IS_VALID_OUTPUT_GPIO(pin_data)
			, ESP_ERR_INVALID_ARG, TAG, "Invalid pin number (%d, %d)", pin_clk, pin_data);

	gpio_config_t datacfg = {
		.pin_bit_mask = 1ULL<<pin_data,
		.pull_up_en = 1,
		.mode = GPIO_MODE_INPUT
	};
	gpio_config_t clkcfg = {
		.pin_bit_mask = 1ULL<<pin_clk,
		.mode = GPIO_MODE_OUTPUT
	};

	esp_err_t ret;

	ESP_RETURN_ON_FALSE((ret=gpio_config(&datacfg))==ESP_OK
    		&& (ret=gpio_config(&clkcfg))==ESP_OK
   		, ret, TAG, "Failed to config GPIO pins (%d:%s)", ret, esp_err_to_name(ret));

	ESP_RETURN_ON_ERROR(ret=gpio_set_level(pin_clk, 0),
			TAG, "Fail to set clock pin level (%d:%s)", ret, esp_err_to_name(ret));

	return ESP_OK;
}

esp_err_t CS1237_power_down(int pin_clk)
{
	ESP_RETURN_ON_FALSE (GPIO_IS_VALID_OUTPUT_GPIO(pin_clk)
			, ESP_ERR_INVALID_ARG, TAG, "Invalid pin number (%d)", pin_clk);

	esp_err_t ret;

	ESP_RETURN_ON_ERROR(ret=gpio_set_level(pin_clk, 1),
			TAG, "Fail to set clock pin level (%d:%s)", ret, esp_err_to_name(ret));

	vTaskDelay(1);

	return ESP_OK;
}

esp_err_t CS1237_power_up(int pin_clk)
{
	ESP_RETURN_ON_FALSE (GPIO_IS_VALID_OUTPUT_GPIO(pin_clk)
			, ESP_ERR_INVALID_ARG, TAG, "Invalid pin number (%d)", pin_clk);

	esp_err_t ret;

	ESP_RETURN_ON_ERROR(ret=gpio_set_level(pin_clk, 0),
			TAG, "Fail to set clock pin level (%d:%s)", ret, esp_err_to_name(ret));

	return ESP_OK;
}

esp_err_t CS1237_data(int pin_clk, int pin_data, int32_t * data, uint8_t * update, int xTicksToDelay)
{
	ESP_RETURN_ON_FALSE (GPIO_IS_VALID_OUTPUT_GPIO(pin_clk)
			          && GPIO_IS_VALID_OUTPUT_GPIO(pin_data)
			, ESP_ERR_INVALID_ARG, TAG, "Invalid pin number (%d, %d)", pin_clk, pin_data);

	esp_err_t ret;

	/* only check once */
	ESP_RETURN_ON_ERROR(ret=gpio_set_direction(pin_data, GPIO_MODE_INPUT),
			TAG, "Fail to set data pin mode (%d:%s)", ret, esp_err_to_name(ret));
	ESP_RETURN_ON_ERROR(ret=gpio_set_level(pin_clk, 0),
			TAG, "Fail to set clock pin level (%d:%s)", ret, esp_err_to_name(ret));

	/*wait DRDY , double check */
	while (gpio_get_level(pin_data) || gpio_get_level(pin_data)) {
		if (xTicksToDelay<=0) return ESP_ERR_TIMEOUT;
		xTicksToDelay--;
		vTaskDelay(1);
	}

	int i=0;
	uint32_t d=0;
	uint8_t  upd=0;
	uint32_t start, cur;

	taskENTER_CRITICAL(&mux);

	DELAY_START(start);
	for (i=0;i<24;i++) {
		DELAY_WAIT(start, cur);
		gpio_set_level(pin_clk, 1);
		DELAY_WAIT(start, cur);
		d<<=1;
		d|=gpio_get_level(pin_data)?1:0;
		gpio_set_level(pin_clk, 0);
	}

	d<<=8; /* for neg int32_t output */

	for (;i<26;i++) {
		DELAY_WAIT(start, cur);
		gpio_set_level(pin_clk, 1);
		DELAY_WAIT(start, cur);
		upd=upd<<1;
		upd|=gpio_get_level(pin_data)?1:0;
		gpio_set_level(pin_clk, 0);
	}

	DELAY_WAIT(start, cur);
	gpio_set_level(pin_clk, 1);
	DELAY_WAIT(start, cur);
	gpio_set_level(pin_clk, 0);

	taskEXIT_CRITICAL(&mux);

	if (data) *data=d;
	if (update) *update=upd;

	return ESP_OK;
}

esp_err_t CS1237_read_config(int pin_clk, int pin_data, int32_t * data, uint8_t * update, uint8_t *config, int xTicksToDelay)
{
	ESP_RETURN_ON_FALSE (GPIO_IS_VALID_OUTPUT_GPIO(pin_clk)
			          && GPIO_IS_VALID_OUTPUT_GPIO(pin_data)
			, ESP_ERR_INVALID_ARG, TAG, "Invalid pin number (%d, %d)", pin_clk, pin_data);

	esp_err_t ret;

	/* only check once */
	ESP_RETURN_ON_ERROR(ret=gpio_set_direction(pin_data, GPIO_MODE_INPUT),
			TAG, "Fail to set data pin mode (%d:%s)", ret, esp_err_to_name(ret));
	ESP_RETURN_ON_ERROR(ret=gpio_set_level(pin_clk, 0),
			TAG, "Fail to set clock pin level (%d:%s)", ret, esp_err_to_name(ret));

	/*wait DRDY , double check */
	while (gpio_get_level(pin_data) || gpio_get_level(pin_data)) {
		if (xTicksToDelay<=0) return ESP_ERR_TIMEOUT;
		xTicksToDelay--;
		vTaskDelay(1);
	}

	int i=0;
	uint32_t d=0;
	uint8_t  upd=0;
	uint8_t  cfg=0;
	uint32_t start, cur;

	taskENTER_CRITICAL(&mux);

	DELAY_START(start);
	DELAY_WAIT(start, cur);
	DELAY_WAIT(start, cur);
	for (i=0;i<24;i++) {
		DELAY_WAIT(start, cur);
		gpio_set_level(pin_clk, 1);
		DELAY_WAIT(start, cur);
		d<<=1;
		d|=gpio_get_level(pin_data)?1:0;
		gpio_set_level(pin_clk, 0);
	}

	d<<=8; /* for neg int32_t output */

	for (;i<26;i++) {
		DELAY_WAIT(start, cur);
		gpio_set_level(pin_clk, 1);
		DELAY_WAIT(start, cur);
		upd=upd<<1;
		upd|=gpio_get_level(pin_data)?1:0;
		gpio_set_level(pin_clk, 0);
	}

	for (;i<29;i++) {
		DELAY_WAIT(start, cur);
		gpio_set_level(pin_clk, 1);
		DELAY_WAIT(start, cur);
		gpio_set_level(pin_clk, 0);
	}

	DELAY_WAIT(start, cur);
	gpio_set_direction(pin_data, GPIO_MODE_OUTPUT);
	uint8_t t=0x56;
	for (;i<36;i++) {
		gpio_set_level(pin_data, (t & 0x40)?1:0);
		gpio_set_level(pin_clk, 1);
		DELAY_WAIT(start, cur);
		gpio_set_level(pin_clk, 0);
		DELAY_WAIT(start, cur);
		t<<=1;
	}

	gpio_set_direction(pin_data, GPIO_MODE_INPUT);
	gpio_set_level(pin_clk, 1);
	DELAY_WAIT(start, cur);
	gpio_set_level(pin_clk, 0);
	i++;

	for (;i<45;i++) {
		DELAY_WAIT(start, cur);
		gpio_set_level(pin_clk, 1);
		DELAY_WAIT(start, cur);
		cfg=cfg<<1;
		cfg|=gpio_get_level(pin_data)?1:0;
		gpio_set_level(pin_clk, 0);
	}

	DELAY_WAIT(start, cur);
	gpio_set_level(pin_clk, 1);
	DELAY_WAIT(start, cur);
	gpio_set_level(pin_clk, 0);

	taskEXIT_CRITICAL(&mux);

	if (data) *data=d;
	if (update) *update=upd;
	if (config) *config=cfg;

	return ESP_OK;
}

esp_err_t CS1237_write_config(int pin_clk, int pin_data, int32_t * data, uint8_t * update, uint8_t config, int xTicksToDelay)
{
	ESP_RETURN_ON_FALSE (GPIO_IS_VALID_OUTPUT_GPIO(pin_clk)
			          && GPIO_IS_VALID_OUTPUT_GPIO(pin_data)
			, ESP_ERR_INVALID_ARG, TAG, "Invalid pin number (%d, %d)", pin_clk, pin_data);

	esp_err_t ret;

	/* only check once */
	ESP_RETURN_ON_ERROR(ret=gpio_set_direction(pin_data, GPIO_MODE_INPUT),
			TAG, "Fail to set data pin mode (%d:%s)", ret, esp_err_to_name(ret));
	ESP_RETURN_ON_ERROR(ret=gpio_set_level(pin_clk, 0),
			TAG, "Fail to set clock pin level (%d:%s)", ret, esp_err_to_name(ret));

	/*wait DRDY , double check */
	while (gpio_get_level(pin_data) || gpio_get_level(pin_data)) {
		if (xTicksToDelay<=0) return ESP_ERR_TIMEOUT;
		xTicksToDelay--;
		vTaskDelay(1);
	}

	int i=0;
	uint32_t d=0;
	uint8_t  upd=0;
	uint32_t start, cur;

	taskENTER_CRITICAL(&mux);

	DELAY_START(start);
	DELAY_WAIT(start, cur);
	DELAY_WAIT(start, cur);
	for (i=0;i<24;i++) {
		DELAY_WAIT(start, cur);
		gpio_set_level(pin_clk, 1);
		DELAY_WAIT(start, cur);
		d<<=1;
		d|=gpio_get_level(pin_data)?1:0;
		gpio_set_level(pin_clk, 0);
	}

	d<<=8; /* for neg int32_t output */

	for (;i<26;i++) {
		DELAY_WAIT(start, cur);
		gpio_set_level(pin_clk, 1);
		DELAY_WAIT(start, cur);
		gpio_set_level(pin_clk, 0);
		upd=upd<<1;
		upd|=gpio_get_level(pin_data)?1:0;
	}

	for (;i<29;i++) {
		DELAY_WAIT(start, cur);
		gpio_set_level(pin_clk, 1);
		DELAY_WAIT(start, cur);
		gpio_set_level(pin_clk, 0);
	}

	DELAY_WAIT(start, cur);
	gpio_set_direction(pin_data, GPIO_MODE_OUTPUT);
	uint8_t t=0x65;
	for (;i<36;i++) {
		gpio_set_level(pin_data, (t & 0x40)?1:0);
		gpio_set_level(pin_clk, 1);
		DELAY_WAIT(start, cur);
		gpio_set_level(pin_clk, 0);
		DELAY_WAIT(start, cur);
		t<<=1;
	}

	gpio_set_level(pin_clk, 1);
	DELAY_WAIT(start, cur);
	gpio_set_level(pin_clk, 0);
	i++;

	for (;i<45;i++) {
		DELAY_WAIT(start, cur);
		gpio_set_level(pin_data, (config & 0x80)?1:0);
		gpio_set_level(pin_clk, 1);
		DELAY_WAIT(start, cur);
		gpio_set_level(pin_clk, 0);
		DELAY_WAIT(start, cur);
		config<<=1;
	}

	gpio_set_direction(pin_data, GPIO_MODE_INPUT);
	gpio_set_level(pin_clk, 1);
	DELAY_WAIT(start, cur);
	gpio_set_level(pin_clk, 0);

	taskEXIT_CRITICAL(&mux);

	if (data) *data=d;
	if (update) *update=upd;

	return ESP_OK;
}
