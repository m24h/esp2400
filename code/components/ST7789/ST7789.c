/**
 * Know nothing about copy from left to right,
 * if someone wanna use it, just copy it from left to right
 *                by author m24h
 */
#include <stdlib.h>
#include <string.h>

#include "esp_check.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"

#include "ST7789.h"

static const char * const TAG = "ST7789";

#define ST7789_POLLING_WHEN 16
#define ST7789_TRUNC_MIN    32

#define ST7789_IDLE    0
#define ST7789_POLLING 1
#define ST7789_QUEUE   2

static uint8_t _calc_rotate(ST7789_t *st7789)
{
	if (st7789->type == ST7789_240X135) {
		if (st7789->rotate == ST7789_R0) {
			st7789->off_x=40;
			st7789->off_y=53;
			return 0x60;
		}
	}

	st7789->off_x=0;
	st7789->off_y=0;
	return 0;
}

esp_err_t ST7789_alloc (ST7789_t ** st7789, spi_host_device_t spi,
		ST7789_type_e type, ST7789_rotate_e rotate, int freq, int trunksize,
		int pin_cs, int pin_reset, int pin_dc)
{
	ESP_RETURN_ON_FALSE(st7789 && pin_dc>=0 && pin_cs>=0
				&& GPIO_IS_VALID_OUTPUT_GPIO(pin_dc) && GPIO_IS_VALID_OUTPUT_GPIO(pin_cs)
				&& (pin_reset<0 || GPIO_IS_VALID_OUTPUT_GPIO(pin_reset))
			, ESP_ERR_INVALID_ARG, TAG, "Invalid argument (%p, %d, %d, %d)", st7789, pin_cs, pin_reset, pin_dc);
	if (trunksize<ST7789_TRUNC_MIN)
		trunksize=ST7789_TRUNC_MIN;

	/* allocte memory */
	ST7789_t *st=(ST7789_t *)heap_caps_malloc(sizeof(ST7789_t)+trunksize+trunksize+8, MALLOC_CAP_DMA);
	ESP_RETURN_ON_FALSE(st
			, ESP_ERR_NO_MEM , TAG, "Failed to allocate ST7789 memory");
	memset(st, 0, sizeof(ST7789_t));

	/* init */
	st->type=type;
	st->rotate=rotate;
	st->pin_dc=pin_dc;
	st->pin_reset=pin_reset;
	st->trunksize=trunksize;
	st->trans.user=ST7789_IDLE;
	
	esp_err_t ret=ESP_OK;

	/* config other GPIO pin */
	gpio_config_t io = {
		.mode = GPIO_MODE_OUTPUT,
		.pin_bit_mask = 1ULL<<pin_dc | (pin_reset<0?0:(1ULL<<pin_reset))
	};
	ESP_GOTO_ON_ERROR(ret=gpio_config(&io)
			, err, TAG, "Failed to config DC/RESET pin as output (%d:%s)", ret, esp_err_to_name(ret));

	/* attach to SPI bus */
	spi_device_interface_config_t cfg={
		.clock_speed_hz=freq>0?freq:SPI_MASTER_FREQ_10M, /* default 10M */
		.mode=0, 
		.spics_io_num=pin_cs,
		.queue_size=1
	};
	ESP_GOTO_ON_ERROR(ret=spi_bus_add_device(spi, &cfg, &(st->dev))
			 , err, TAG, "Failed to attach device to a SPI bus (%d:%s)", ret, esp_err_to_name(ret));

	/* do reset and init after reset */
	if ((ret=ST7789_reset(st))!=ESP_OK)
		goto err;

	*st7789 = st;
	return ret;

err:
	ST7789_free(&st);
    return ret;
}

esp_err_t ST7789_free (ST7789_t **st7789)
{
	ESP_RETURN_ON_FALSE(st7789
			, ESP_ERR_INVALID_ARG, TAG, "NULL argument");
	
	esp_err_t ret;

	if (*st7789) {
		ESP_RETURN_ON_ERROR(ret=spi_bus_remove_device((*st7789)->dev),
				TAG, "Failed to detach device from the SPI bus (%d:%s)", ret, esp_err_to_name(ret));

		heap_caps_free(*st7789);
		*st7789=NULL;
	}

	return ESP_OK;
}

esp_err_t ST7789_reset (ST7789_t *st7789)
{
	ESP_RETURN_ON_FALSE(st7789
			, ESP_ERR_INVALID_ARG, TAG, "NULL device");

	esp_err_t ret;

	/* try hard reset first, otherwise soft reset */
	if (st7789->pin_reset>=0) {
		/* need SPI action complete */
		if ((ret=ST7789_wait_complete(st7789))!=ESP_OK)
			return ret;

		ESP_RETURN_ON_ERROR(ret=gpio_set_level(st7789->pin_reset, 0)
			, TAG, "Fail to set reset pin level to 0 (%d:%s)", ret, esp_err_to_name(ret));

		vTaskDelay(25/portTICK_PERIOD_MS);

		ESP_RETURN_ON_ERROR(ret=gpio_set_level(st7789->pin_reset, 1)
			, TAG, "Fail to set reset pin level to 1 (%d:%s)", ret, esp_err_to_name(ret));

	} else if ((ret=ST7789_command(st7789, ST7789_SWRESET))!=ESP_OK)
			return ret;

	vTaskDelay(25/portTICK_PERIOD_MS);

	/* init ST7789 setting */
	if ((ret=ST7789_command(st7789, ST7789_SLPOUT))!=ESP_OK) return ret;
	if ((ret=ST7789_command(st7789, ST7789_INVON))!=ESP_OK) return ret;
	if ((ret=ST7789_command(st7789, ST7789_NORON))!=ESP_OK) return ret;
	if ((ret=ST7789_command(st7789, ST7789_DISPON))!=ESP_OK) return ret;
	if ((ret=ST7789_command(st7789, ST7789_COLMOD))!=ESP_OK) return ret;
	if ((ret=ST7789_data_small(st7789, 0x55, 0, 0, 0, 1))!=ESP_OK) return ret;
	if ((ret=ST7789_command(st7789, ST7789_MADCTL))!=ESP_OK) return ret;
	return ST7789_data_small(st7789, _calc_rotate(st7789), 0, 0, 0, 1);
}

void ST7789_encode_color888(uint8_t *color, uint8_t r, uint8_t g, uint8_t b)
{
	*color=(r & 0xf8) | g>>5;
	*(color+1)=(g & 0xfc)<<3 | b>>3;
}

void ST7789_encode_color32(uint8_t *color, uint32_t rgb)
{
	*color=((rgb>>16) & 0xf8) | ((rgb>>13) & 0x07);
	*(color+1)=((rgb>>5) & 0xe0) | ((rgb>>3) & 0x1f);
}

esp_err_t ST7789_wait_complete(ST7789_t *st7789)
{
	esp_err_t ret;

	if ((int)st7789->trans.user==ST7789_POLLING) {
		ESP_RETURN_ON_ERROR(ret=spi_device_polling_end(st7789->dev, portMAX_DELAY)
				, TAG, "Failed to wait polling complete (%d:%s)", ret, esp_err_to_name(ret));
	} else if ((int)st7789->trans.user==ST7789_QUEUE) {
		spi_transaction_t *p;
		ESP_RETURN_ON_ERROR(ret=spi_device_get_trans_result(st7789->dev, &p, portMAX_DELAY)
				 , TAG, "Failed to wait queue complete (%d:%s)", ret, esp_err_to_name(ret));
	}

	st7789->trans.user=ST7789_IDLE;
	return ESP_OK;
}

esp_err_t ST7789_command (ST7789_t * st7789, uint8_t cmd)
{
	ESP_RETURN_ON_FALSE(st7789
			, ESP_ERR_INVALID_ARG, TAG, "Null device");

	esp_err_t ret=ST7789_wait_complete(st7789);
	if (ret!=ESP_OK) return ret;

	ESP_RETURN_ON_ERROR(ret=gpio_set_level(st7789->pin_dc, 0)
			, TAG, "Failed to set DC pin level to 0 (%d:%s)", ret, esp_err_to_name(ret));

	st7789->trans.flags=SPI_TRANS_USE_TXDATA;
	st7789->trans.length=8;
	st7789->trans.tx_data[0]=cmd;

	ESP_RETURN_ON_ERROR(ret=spi_device_polling_start(st7789->dev, &st7789->trans, portMAX_DELAY)
			, TAG, "Failed to send SPI data in polling mode (%d:%s)", ret, esp_err_to_name(ret));

	st7789->trans.user=(void*)ST7789_POLLING;
	return ret;
}

esp_err_t ST7789_data_small (ST7789_t * st7789, uint8_t d1, uint8_t d2, uint8_t d3, uint8_t d4, int len)
{
	ESP_RETURN_ON_FALSE(st7789 && len<5
			, ESP_ERR_INVALID_ARG, TAG, "Null device or len is too big (%p, %d)", st7789, len);

	if (len<=0) return ESP_OK;

	esp_err_t ret=ST7789_wait_complete(st7789);
	if (ret!=ESP_OK) return ret;

	ESP_RETURN_ON_ERROR(ret=gpio_set_level(st7789->pin_dc, 1)
			, TAG, "Failed to set DC pin level to 1 (%d:%s)", ret, esp_err_to_name(ret));

	st7789->trans.flags=SPI_TRANS_USE_TXDATA;
	st7789->trans.length=len<<3;
	st7789->trans.tx_data[0]=d1;
	st7789->trans.tx_data[1]=d2;
	st7789->trans.tx_data[2]=d3;
	st7789->trans.tx_data[3]=d4;

	ESP_RETURN_ON_ERROR(ret=spi_device_polling_start(st7789->dev, &st7789->trans, portMAX_DELAY)
			, TAG, "Failed to send SPI data in polling mode (%d:%s)", ret, esp_err_to_name(ret));

	st7789->trans.user=(void*)ST7789_POLLING;
	return ret;
}

esp_err_t ST7789_data (ST7789_t * st7789, const uint8_t *data, int len)
{
	ESP_RETURN_ON_FALSE(st7789
			, ESP_ERR_INVALID_ARG, TAG, "Null device");

	esp_err_t ret=ESP_OK;

	int dc=0;
	int n;
	while (len>0) {
		n=len>st7789->trunksize?st7789->trunksize:len;

		if ((ret=ST7789_wait_complete(st7789))!=ESP_OK)	return ret;

		if (!dc)
			ESP_RETURN_ON_FALSE((ret=gpio_set_level(st7789->pin_dc, dc=1))==ESP_OK
				,ret , TAG, "Failed to set DC pin level to 1 (%d:%s)", ret, esp_err_to_name(ret));

		st7789->trans.flags=0;
		st7789->trans.length=n<<3;
		st7789->trans.tx_buffer=data;

		if (len<=ST7789_POLLING_WHEN) {
			ESP_RETURN_ON_ERROR(ret=spi_device_polling_start(st7789->dev, &st7789->trans, portMAX_DELAY)
					, TAG, "Failed to send SPI data in polling mode (%d:%s)", ret, esp_err_to_name(ret));
			st7789->trans.user=(void*)ST7789_POLLING;
		} else  {
			ESP_RETURN_ON_ERROR(ret=spi_device_queue_trans(st7789->dev, &st7789->trans, portMAX_DELAY)
					, TAG, "Failed to send SPI data in queue mode (%d:%s)", ret, esp_err_to_name(ret));
			st7789->trans.user=(void*)ST7789_QUEUE;
		}

		len-=n;
		data+=n;
	}

	return ret;
}

esp_err_t ST7789_buff_get (ST7789_t * st7789, uint8_t **buff)
{
	ESP_RETURN_ON_FALSE(st7789 && buff
			, ESP_ERR_INVALID_ARG, TAG, "Null argument (%p, %p)", st7789, buff);

   st7789->buffnum=(st7789->buffnum+1) & 1;
   *buff = st7789->buffnum?(st7789->buff+st7789->trunksize):st7789->buff;

   return ESP_OK;
}

esp_err_t ST7789_buff_put (ST7789_t * st7789, int len)
{
	ESP_RETURN_ON_FALSE(st7789
			, ESP_ERR_INVALID_ARG, TAG, "Null device");

	if (len<=0) return ESP_OK;

	esp_err_t ret=ST7789_wait_complete(st7789);
	if (ret!=ESP_OK) return ret;

	ESP_RETURN_ON_ERROR(ret=gpio_set_level(st7789->pin_dc, 1)
			, TAG, "Failed to set DC pin level to 1 (%d:%s)", ret, esp_err_to_name(ret));

	st7789->trans.flags=0;
	st7789->trans.length=len<<3;
	st7789->trans.tx_buffer=st7789->buffnum?(st7789->buff+st7789->trunksize):st7789->buff;

	if (len<=ST7789_POLLING_WHEN) {
		ESP_RETURN_ON_ERROR(ret=spi_device_polling_start(st7789->dev, &st7789->trans, portMAX_DELAY)
				, TAG, "Failed to send SPI data in polling mode (%d:%s)", ret, esp_err_to_name(ret));
		st7789->trans.user=(void*)ST7789_POLLING;
	} else  {
		ESP_RETURN_ON_ERROR(ret=spi_device_queue_trans(st7789->dev, &st7789->trans, portMAX_DELAY)
				, TAG, "Failed to send SPI data in queue mode (%d:%s)", ret, esp_err_to_name(ret));
		st7789->trans.user=(void*)ST7789_QUEUE;
	}

	return ret;
}

esp_err_t ST7789_set_window(ST7789_t *st7789, int x, int y, int width, int height)
{
	ESP_RETURN_ON_FALSE(st7789
			, ESP_ERR_INVALID_ARG, TAG, "NULL device");

	x+=st7789->off_x;
	y+=st7789->off_y;
	int x2=x+width-1;
	int y2=y+height-1;

	esp_err_t ret=ST7789_command(st7789, ST7789_CASET);
	if (ret!=ESP_OK) return ret;
	if ((ret=ST7789_data_small(st7789, x>>8, x, x2>>8, x2, 4))!=ESP_OK) return ret;
	if ((ret=ST7789_command(st7789, ST7789_RASET))!=ESP_OK) return ret;
	return ST7789_data_small(st7789, y>>8, y, y2>>8, y2, 4);
}

esp_err_t ST7789_fill(ST7789_t * st7789, const uint8_t *color, int num)
{
	ESP_RETURN_ON_FALSE(st7789 && color
			, ESP_ERR_INVALID_ARG, TAG, "Null device or null color (%p, %p)", st7789, color);

	esp_err_t ret=ST7789_command(st7789, ST7789_RAMWR);
	if (ret!=ESP_OK) return ret;

	uint8_t *buff;
	if ((ret=ST7789_buff_get(st7789, &buff))!=ESP_OK) return ret;

	int n=st7789->trunksize/2;
	if (n>num) n=num;
	for (int i=0; i<n; i++) *((uint16_t *)buff+i)=*(uint16_t *)color;

	while (num>0) {
		if (n>num) n=num;
		if ((ret=ST7789_buff_put(st7789, n+n))!=ESP_OK) return ret;
		num-=n;
	}

	return ret;
}

esp_err_t ST7789_palette(ST7789_t * st7789, int bits, const uint8_t *colors, const uint8_t *pixels, int num, int little_endian, int every, int skip)
{
	ESP_RETURN_ON_FALSE(st7789 && colors && pixels
			, ESP_ERR_INVALID_ARG, TAG, "Null argument (%p, %p, %p)", st7789, colors, pixels);

	if (every<=0) every=num+1;

	esp_err_t ret=ST7789_command(st7789, ST7789_RAMWR);
	if (ret!=ESP_OK) return ret;

	int x=0;
	int q=0;
	uint8_t *buff;
	int n=st7789->trunksize/2;
	while (num>0) {
		if ((ret=ST7789_buff_get(st7789, &buff))!=ESP_OK) return ret;

		if (n>num) n=num;
		for (int i=0; i<n; i++) {
			int k=0;
			for (int j=0; j<bits; j++) {
				k+=k;
				k|=0x01 & *pixels>>(little_endian?q:(7-q));

				if (++x>=every) {
					q=q+skip;
					x=0;
				}
				if (++q>7) {
					pixels+=q>>3;
					q&=0x07;
				}
			}
			*((uint16_t*)buff+i)=*((uint16_t*)colors+k);
		}

		if ((ret=ST7789_buff_put(st7789, n+n))!=ESP_OK) return ret;

		num-=n;
	}

	return ret;
}

esp_err_t ST7789_blit (ST7789_t * st7789, const uint8_t *pixels, int num, int every, int skip)
{
	ESP_RETURN_ON_FALSE(st7789 && pixels
			, ESP_ERR_INVALID_ARG, TAG, "Null argument (%p, %p)", st7789, pixels);

	if (every<=0) every=num+1;

	esp_err_t ret=ST7789_command(st7789, ST7789_RAMWR);
	if (ret!=ESP_OK) return ret;

	int x=0;
	int k=0;
	int n=st7789->trunksize/2;
	uint8_t *buff;
	while (num>0) {
		if ((ret=ST7789_buff_get(st7789, &buff))!=ESP_OK) return ret;

		if (n>num) n=num;
		for (int i=0; i<n; i++) {
			*((uint16_t *)buff+i)=*((uint16_t*)pixels+k);
			if (++x>=every) {
				k+=skip;
				x=0;
			}
			k++;
		}

		if ((ret=ST7789_buff_put(st7789, n+n))!=ESP_OK)	return ret;

		num-=n;
	}

	return ret;
}
    			
esp_err_t ST7789_rect (ST7789_t * st7789, int x, int y, int width, int height, const uint8_t *color)
{
	esp_err_t ret;
	if ((ret=ST7789_set_window(st7789, x, y, width, height))!=ESP_OK) return ret;
	return ST7789_fill(st7789, color, width*height);
}

esp_err_t ST7789_frame(ST7789_t * st7789, int x, int y, int width, int height, int lineWidth, const uint8_t *color)
{
	esp_err_t ret;
	if ((ret=ST7789_set_window(st7789, x, y, width, lineWidth))!=ESP_OK) return ret;
	if ((ret=ST7789_fill(st7789, color, width*lineWidth))!=ESP_OK) return ret;
	if ((ret=ST7789_set_window(st7789, x+width-lineWidth, y, lineWidth, height))!=ESP_OK) return ret;
	if ((ret=ST7789_fill(st7789, color, height*lineWidth))!=ESP_OK) return ret;
	if ((ret=ST7789_set_window(st7789, x, y+height-lineWidth, width, lineWidth))!=ESP_OK) return ret;
	if ((ret=ST7789_fill(st7789, color, width*lineWidth))!=ESP_OK) return ret;
	if ((ret=ST7789_set_window(st7789, x, y, lineWidth, height))!=ESP_OK) return ret;
	return ST7789_fill(st7789, color, height*lineWidth);
}

esp_err_t ST7789_pixel(ST7789_t * st7789, int x, int y, const uint8_t *color)
{
	esp_err_t ret;
	if ((ret=ST7789_set_window(st7789, x, y, 1, 1))!=ESP_OK) return ret;
	if ((ret=ST7789_command(st7789, ST7789_RAMWR))!=ESP_OK) return ret;
	return ST7789_data_small(st7789, *color, *(color+1), 0, 0, 2);
}

esp_err_t ST7789_text (ST7789_t * st7789, int *x, int y, int width, const uint8_t *color,
		                const font_lib_t *lib, const char **str, font_coding_e coding,
						ST7789_align_e align, int little_endian)
{
	ESP_RETURN_ON_FALSE(st7789 && x && color && lib && str
			, ESP_ERR_INVALID_ARG, TAG, "Null argument (%p, %p, %p, %p, %p)", st7789, x, color, lib, str);

	esp_err_t ret=ESP_OK;

    uint32_t     s;
    font_found_t f;
    int  t;
    const char *p=*str;
    while((s=font_from(&p, coding))!=0) {
    	ESP_RETURN_ON_ERROR(ret=font_find(&f, lib, s)
    		, TAG, "Failed to find font (%d:%s)", ret, esp_err_to_name(ret));
		if (f.data) {
			width-=f.width;
			if (width<0) return ESP_OK;
			t=align==ST7789_TOP?y:(align==ST7789_BOTTOM?y-f.height+1:y-f.height/2);
			if ((ret=ST7789_set_window(st7789, *x, t, f.width, f.height))!=ESP_OK) return ret;
			if ((ret=ST7789_palette(st7789, 1, color, f.data, f.width*f.height, little_endian, f.width, f.skip)) !=ESP_OK) return ret;
			*x+=f.width;
			*str=p;
		}
    }

    return ESP_OK;
}

esp_err_t ST7789_text_wrap (ST7789_t * st7789, int *x, int *y, int width, int height, int lineheight, const uint8_t *color,
		                const font_lib_t *lib, const char **str, font_coding_e coding,
						ST7789_align_e align, int little_endian)
{
	ESP_RETURN_ON_FALSE(st7789 && x && y && color && lib && str
			, ESP_ERR_INVALID_ARG, TAG, "Null argument (%p, %p, %p, %p, %p, %p)", st7789, x, y, color, lib, str);

	esp_err_t ret=ESP_OK;

    uint32_t     s;
    font_found_t f;
    int  y2=*y, x2=*x, y3;
    int  t, x_org=x2;
    height+=y2;
    width+=x2;
    const char *p=*str;
    while(y2<height && (s=font_from(&p, coding))!=0) {
    	if (s=='\r') {
    		*str=p;
    		x2=x_org;
    		continue;
    	}
    	if (s=='\n') {
    		x2=x_org;
    		y2+=lineheight;
    		*str=p;
    		continue;
    	}
    	ESP_RETURN_ON_ERROR(ret=font_find(&f, lib, s)
    		, TAG, "Failed to find font (%d:%s)", ret, esp_err_to_name(ret));
		if (f.data) {
			if (x2+f.width>width) {
	    		x2=x_org;
	    		y2+=lineheight;
			}
			t=align==ST7789_TOP?y2:(align==ST7789_BOTTOM?y2-f.height+1:y2-f.height/2);
			y3=t+f.height;
			if (y3>height) break;
			if ((ret=ST7789_set_window(st7789, x2, t, f.width, f.height))!=ESP_OK) return ret;
			if ((ret=ST7789_palette(st7789, 1, color, f.data, f.width*f.height, little_endian, f.width, f.skip)) !=ESP_OK) return ret;
			x2+=f.width;
			*str=p;
			if (*x<x2) *x=x2;
			if (*y<y3) *y=y3;
		}
    }

    return ESP_OK;
}
