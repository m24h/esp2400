/*
 * Know nothing about copy from left to right,
 * if someone wanna use it, just copy it from left to right
 *                by author m24h
 */
#pragma once

#include "driver/spi_master.h"
#include "font.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ST7789_NOP 	    0x00  /* no operation */
#define ST7789_SWRESET  0x01  /* soft reset */

#define ST7789_SLPIN    0x10  /* Sleep in */
#define ST7789_SLPOUT   0x11  /* Sleep out */
#define ST7789_PTLON    0x12  /* Partial mode on */
#define ST7789_NORON    0x13  /* Partial off (normal) */

#define ST7789_INVOFF   0x20  /* Display inversion off  */
#define ST7789_INVON    0x21  /* Display inversion on */
#define ST7789_DISPOFF  0x28  /* Display off  */
#define ST7789_DISPON   0x29  /* Display on */
#define ST7789_CASET    0x2A  /* Column address set, w(XSh, XSl, XEh��XEl) */
#define ST7789_RASET    0x2B  /* Row address set, w(YSh, YSl, YEh, YEl) */
#define ST7789_RAMWR    0x2C  /* Memory write, w(data ...) */
#define ST7789_RAMRD    0x2E  /* Memory read, r(dummy, data ...) */
#define ST7789_PTLAR    0x30  /* Partial sart/end address set, w(PSh, PSl, PEh, PEl) */
#define ST7789_VSCRDEF  0x33  /* Vertical Scrolling, w(TSAh, TSAl, VSAh, VSAl, BFAh, BFAl) */
#define ST7789_MADCTL   0x36  /* Memory data access control, w(MY|MX|MV|ML|RGB|MH|0|0) */
#define ST7789_IDMOFF   0x38  /* Idle mode off */
#define ST7789_IDMON    0x39  /* Idle mode on */
#define ST7789_COLMOD   0x3A  /* Interface pixel format, w(0|D6|D5|D4|0|D2|D1|D0) */

typedef enum {
	ST7789_240X135 = 0
} ST7789_type_e;

typedef enum {
	ST7789_R0 = 0
} ST7789_rotate_e;

typedef enum {
	ST7789_TOP = 0,
	ST7789_MIDDLE,
	ST7789_BOTTOM
} ST7789_align_e;

typedef struct {
	spi_device_handle_t  dev;
	ST7789_type_e type;
	ST7789_rotate_e rotate;

	int  trunksize;
	int  pin_dc;
	int  pin_reset;
	int  off_x;
	int  off_y;

	int	 buffnum;

	spi_transaction_t trans;

	uint8_t buff[0];
}  ST7789_t;

/**
 * allocate a ST7789 device and attach it to SPI bus
 * if [pin_reset]>=0, then hard reset it after attaching
 * if [freq]=0, 10M will be default
 * [trunksize] is size of one transmiting,
 * [trunksize] is also the size of each internal transmitting buffer (2 buffers), in bytes
 * [trunksize] has an internal minium size, actual size can be get from ST7789_t struct
 * [st7789] is a pointer to a ST7789_t pointer to contain the result
 * multi ST7789 can be allocated and keeping seperated
 */
esp_err_t ST7789_alloc (ST7789_t ** st7789, spi_host_device_t spi,
		ST7789_type_e type, ST7789_rotate_e rotate, int freq, int trunksize,
		int pin_cs, int pin_reset, int pin_dc);

/**
 * detach device from SPI bus and free allocated memory
 * st7789 should not be NULL
 * finally set *[st7789] to NULL
 * this does not automatically change GPIO pin setting back
 */
esp_err_t ST7789_free (ST7789_t ** st7789);

/**
 * reset ST7789
 */
esp_err_t ST7789_reset (ST7789_t *st7789);

/**
 * encode RGB color to big-endian 16bit RGB565 format
 * and store 2 bytes pointed by color
 */
void ST7789_encode_color888(uint8_t *color, uint8_t r, uint8_t g, uint8_t b);

/**
 * encode RGB color (0x00RRGGBB) to big-endian 16bit RGB565 format
 * and store 2 bytes pointed by color
 */
void ST7789_encode_color32(uint8_t *color, uint32_t rgb);

/**
 * wait any transmitting complete
 * no need to call this until state is need to confirm
 */
esp_err_t ST7789_wait_complete(ST7789_t *st7789);

/**
 * send ST7789 raw command
 */
esp_err_t ST7789_command (ST7789_t * st7789, uint8_t cmd);

/**
 * directly transmit [len] bytes small data
 * function will choice polling method
 */
esp_err_t ST7789_data_small (ST7789_t * st7789, uint8_t d1, uint8_t d2, uint8_t d3, uint8_t d4, int len);

/**
 * directly transmit [len] bytes data
 * [data] is at least [len] bytes long
 * [data] should be static DMA_ATTR/DRAM_ATTR modifed or from heap_caps_malloc(size, MALLOC_CAP_DMA) if DMA is using
 * Note: before ST7789_wait_complete(), maybe [data] is using
 * if [data] is NULL, reuse last tran
 * function will choice polling/queue method automatically
 * if data is longer than trunk size, it will be send as several segments
 */
esp_err_t ST7789_data (ST7789_t * st7789, const uint8_t *data, int len);

/**
 * get internal transimitting buffer and store it in *[buff] for putting data
 * buffer size is the trunksize set when allocating
 * do not get twice before a ST7789_buff_put
 */
esp_err_t ST7789_buff_get (ST7789_t * st7789, uint8_t **buff);

/**
 * put back internal buffer as data (not command)
 * and transimit [len] bytes data if [len] is greater than 0
 * buffer size is the trunksize set when allocating
 * function will choice polling/queue method automatically
 */
esp_err_t ST7789_buff_put (ST7789_t * st7789, int len);

/**
 * set drawing window (CASET/RASET)
 */
esp_err_t ST7789_set_window(ST7789_t *st7789, int x, int y, int width, int height);

/**
 * fill drawing window with same specified 2bytes/RGB565 color of number of pixels
 */
esp_err_t ST7789_fill(ST7789_t * st7789, const uint8_t *color, int num);

/**
 * draw pixels (in [bits] palette format) to drawing window
 * [pixels] is at least [num]*[bits] bits long, big-endian or little-endian if [little_endian]!=0
 * [color] is 2^[bits] bytes palette, each color stored in RGB565 2 bytes format
 * if [every]>0, skip [skip] bits every [every] bits used, this is for sth. like line bits alignment
 */
esp_err_t ST7789_palette(ST7789_t * st7789, int bits, const uint8_t *colors, const uint8_t *pixels, int num, int little_endian, int every, int skip);

/**
 * blit pixels (in 2bytes/RGB565 format) to drawing window
 * internal trunk buffer (compatible with DMA) is used
 * [pixels] is at least [num]*2 bytes long
 * if [every]>0, skip [skip] pixels every [every] pixels used, this is for sth. like line alignment
 */
esp_err_t ST7789_blit (ST7789_t * st7789, const uint8_t *pixels, int num, int every, int skip);

/**
 * fill a rectangle using [color] color
 */
esp_err_t ST7789_rect (ST7789_t * st7789, int x, int y, int width, int height, const uint8_t *color);

/**
 * draw a rectangle frame, not filled inside
 */
esp_err_t ST7789_frame(ST7789_t * st7789, int x, int y, int width, int height, int lineWidth, const uint8_t *color);

/**
 * draw a pixel
 */
esp_err_t ST7789_pixel(ST7789_t * st7789, int x, int y, const uint8_t *color);

/**
 * draw a text line
 * [color] is 4 bytes, first 2 bytes is background color, second 2 bytes is text color
 * [y] is the alignment base, Y position of top/middle/bottom
 * fonts should be little-endian if [little_endian] is 1
 * [width] is the limit for avoiding drawing text out of range
 * [*x] will contains the max unreached X position before function returned
 * [*str] will point to last unreached character
 */
esp_err_t ST7789_text (ST7789_t * st7789, int *x, int y, int width, const uint8_t *color,
		                const font_lib_t *lib, const char **str, font_coding_e coding,
						ST7789_align_e align, int little_endian);

/**
 * draw a text as multi-line, text was automatically wrapped, and "\n" is wrapped
 * [lineheight] is line height, maybe different from font height
 * [color] is 4 bytes, first 2 bytes is background color, second 2 bytes is text color
 * [y] is the alignment base
 * fonts should be little-endian if [little_endian] is 1
 * [width] is the wrap limit for avoiding drawing text out of range
 * [height] is the limit for avoiding drawing text out of BOTTOM range
 * [*x] will contains the max unreached X position before function returned
 * [*y] will contains the max unreached Y position before function returned
 * [*str] will point to last unreached character
 */
esp_err_t ST7789_text_wrap (ST7789_t * st7789, int *x, int *y, int width, int height, int lineheight, const uint8_t *color,
		                const font_lib_t *lib, const char **str, font_coding_e coding,
						ST7789_align_e align, int little_endian);

#ifdef __cplusplus
}
#endif

