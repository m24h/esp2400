/**
 * Know nothing about copy from left to right,
 * if someone wanna use it, just copy it from left to right
 *                by author m24h
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"

#define CS1237_REFO        0x40
#define CS1237_REFO_ON     0x00
#define CS1237_REFO_OFF    0x40

#define CS1237_SPEED       0x30
#define CS1237_SPEED_10    0x00
#define CS1237_SPEED_40    0x10
#define CS1237_SPEED_640   0x20
#define CS1237_SPEED_1280  0x30

#define CS1237_PGA        0x0C
#define CS1237_PGA_1      0x00
#define CS1237_PGA_2	  0x04
#define CS1237_PGA_64     0x08
#define CS1237_PGA_128    0x0c

#define CS1237_CH           0x03
#define CS1237_CH_A         0x00
#define CS1237_CH_RESERVED  0x01
#define CS1237_CH_TEMP      0x02
#define CS1237_CH_SHORT     0x03

/**
 * setup CS1237 pins before use it, after init CS1237 is in power-on state
 * there should be a setup duration between CS1237 giving out data and function completing
 * according to data rate 10/40/640/1280, it will be 300/75/6.25/3.125 ms
 */
esp_err_t CS1237_init_pin(int pin_clk, int pin_data);

/**
 * power down CS1237
 */
esp_err_t CS1237_power_down(int pin_clk);

/**
 * power up CS1237
 * there should be a setup duration between CS1237 giving out data and function completing
 * according to data rate 10/40/640/1280, it will be 300/75/6.25/3.125 ms
 */
esp_err_t CS1237_power_up(int pin_clk);

/**
 * read data and 2 updating info bits
 * *[data] is in range -2^31 - (2^31-1)
 * if *[update](bit 1) is 1, means last config was updated
 * null [data] or null [update] means not storing
 * [xTicksToDelay] is the ticks to wait if DRDY is not low
 * if timeout, return ESP_ERR_TIMEOUT
 */
esp_err_t CS1237_data(int pin_clk, int pin_data, int32_t * data, uint8_t * update, int xTicksToDelay);

/**
 * read data and 2 updating info bits, then read config
 * *[data] is in range -2^31 - (2^31-1)
 * if *[update](bit 1) is 1, means last config was updated
 * [xTicksToDelay] is the ticks to wait if DRDY is not low
 * if timeout, return ESP_ERR_TIMEOUT
 */
esp_err_t CS1237_read_config(int pin_clk, int pin_data, int32_t * data, uint8_t * update, uint8_t *config, int xTicksToDelay);

/**
 * read data and 2 updating info bits, then write config
 * *[data] is in range -2^31 - (2^31-1)
 * if *[update](bit 1) is 1, means last config was updated
 * [xTicksToDelay] is the ticks to wait if DRDY is not low
 * if timeout, return ESP_ERR_TIMEOU
 * if [config] is null, will write default config 0x0c
 * there should be a setup duration between CS1237 giving out data and function completing
 * according to data rate 10/40/640/1280, it will be 300/75/6.25/3.125 ms
 *
 */
esp_err_t CS1237_write_config(int pin_clk, int pin_data, int32_t * data, uint8_t * update, uint8_t config, int xTicksToDelay);


#ifdef __cplusplus
}
#endif


