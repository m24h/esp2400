/**
 * Know nothing about copy from left to right,
 * if someone wanna use it, just copy it from left to right
 *                by author m24h
 */
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "driver/spi_common.h"
#include "esp_timer.h"

#include "button.h"
#include "rotary.h"

#include "app.h"
#include "menu.h"
#include "ui.h"

const static char * const TAG="UI";

#define LCD_SPI_FREQ       20000000
#define LCD_SPI_HOST       SPI2_HOST
#define LCD_BUF_SIZE       1000
#define PERIOD_SCAN_BUTTON 5000
#define PERIOD_FLUSH       100000
#define BTN_DEBOUNCE 	   50000   /* in micro-second */
#define ROTARY_DEBOUNCE    25000   /* in micro-second */
#define ROT_FAST_INTERVAL  150000  /* fast rotation judgement: us between rotation */
#define ROT_FAST_REPEAT    2       /* fast rotation judgement: times of fast rotation */

ST7789_t   * ui_lcd=NULL;

ESP_EVENT_DEFINE_BASE(UI_E_BASE);

font_lib_t * font_IVC = NULL;
font_lib_t * font_IVS = NULL;
font_lib_t * font_message = NULL;
font_lib_t * font_menu = NULL;
font_lib_t * font_sign = NULL;

uint8_t   color_bg[4];
uint8_t   color_IC[4];
uint8_t   color_IS[4];
uint8_t   color_VC[4];
uint8_t   color_VS[4];
uint8_t   color_OFF[4];
uint8_t   color_IT[4];
uint8_t   color_VT[4];
uint8_t   color_PT[4];
uint8_t   color_ET[4];
uint8_t   color_temp[4];
uint8_t   color_message[4];
uint8_t   color_sign[4];
uint8_t   color_menu[4];
uint8_t   color_menuhl[4];
uint8_t   color_menuhint[4];

/* external UI handler */
extern int32_t ui_main(void * arg, int32_t event, void *data);

static button_t ibtn;
static button_t vbtn;
static rotary_t irot;
static rotary_t vrot;

static esp_timer_handle_t timer_button=NULL;
static esp_timer_handle_t timer_flush=NULL;

/* external fonts binaries */
extern const uint8_t FONT_MESSAGE[] asm("_binary_message_fon_start");
extern const uint8_t INDX_MESSAGE[] asm("_binary_message_idx_start");
extern const uint8_t FONT_HZ[] asm("_binary_hz_fon_start");
extern const uint8_t INDX_HZ[] asm("_binary_hz_idx_start");
extern const uint8_t FONT_IVC[] asm("_binary_ivc_fon_start");
extern const uint8_t INDX_IVC[] asm("_binary_ivc_idx_start");
extern const uint8_t FONT_IVC2[] asm("_binary_ivc2_fon_start");
extern const uint8_t INDX_IVC2[]   asm("_binary_ivc2_idx_start");
extern const uint8_t FONT_IVS[]    asm("_binary_ivs_fon_start");
extern const uint8_t INDX_IVS[]    asm("_binary_ivs_idx_start");
extern const uint8_t FONT_IVS2[]   asm("_binary_ivs2_fon_start");
extern const uint8_t INDX_IVS2[]   asm("_binary_ivs2_idx_start");
extern const uint8_t FONT_IVS3[]   asm("_binary_ivs3_fon_start");
extern const uint8_t INDX_IVS3[]   asm("_binary_ivs3_idx_start");
extern const uint8_t FONT_IVS4[]   asm("_binary_ivs4_fon_start");
extern const uint8_t INDX_IVS4[]   asm("_binary_ivs4_idx_start");
extern const uint8_t FONT_MENU[]   asm("_binary_menu_fon_start");
extern const uint8_t INDX_MENU[]   asm("_binary_menu_idx_start");
extern const uint8_t FONT_MENUHZ[] asm("_binary_menuhz_fon_start");
extern const uint8_t INDX_MENUHZ[] asm("_binary_menuhz_idx_start");
static const uint8_t FONT_SIGN[]={ /* 12*12 */
	/* empty */
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	/* discharge */
	0x07,0x80,0x07,0x80,0x07,0x00,0x0E,0x00,0x0E,0x00,0x1F,0x80,0x1F,0x80,0x03,0x00,
	0x06,0x00,0x0C,0x00,0x08,0x00,0x10,0x00,
	/* fan */
	0x03,0x00,0x03,0x80,0x03,0xC0,0x01,0xC0,0x01,0x80,0x3D,0x80,0x78,0x00,0x73,0x00,
	0x63,0xE0,0x01,0xE0,0x00,0xC0,0x00,0x00,
	/* wifi */
	0x00,0x00,0x1F,0x80,0x70,0xE0,0xC6,0x30,0x1F,0x80,0x30,0xC0,0x06,0x00,0x09,0x00,
	0x00,0x00,0x06,0x00,0x06,0x00,0x00,0x00,
};
static const uint8_t INDX_SIGN[]={0x01,0x00,0x00,0x00,
								  0x02,0x00,0x00,0x00,
								  0x03,0x00,0x00,0x00,
								  0x04,0x00,0x00,0x00,
								  0x00,0x00,0x00,0x00};

static void on_timer_flush(void * arg)
{
	/* not so important, ignore it if out of half flush time */
	esp_event_post_to(app_eloop, UI_E_BASE, UI_E_FLUSH, NULL, 0, PERIOD_FLUSH/2000/portTICK_PERIOD_MS);
}

static void on_timer_button(void * arg)
{
	button_event_e e=button_scan(&ibtn);
	if (e==BUTTON_CLICK)
		esp_event_post_to(app_eloop, UI_E_BASE, UI_E_ICLICK, NULL, 0, portMAX_DELAY);
	else if (e==BUTTON_LONG)
		esp_event_post_to(app_eloop, UI_E_BASE, UI_E_ILONG, NULL, 0, portMAX_DELAY);

	e=button_scan(&vbtn);
	if (e==BUTTON_CLICK)
		esp_event_post_to(app_eloop, UI_E_BASE, UI_E_VCLICK, NULL, 0, portMAX_DELAY);
	else if (e==BUTTON_LONG)
		esp_event_post_to(app_eloop, UI_E_BASE, UI_E_VLONG, NULL, 0, portMAX_DELAY);
}

static const char * alert_text=NULL;
static ui_handler_t alert_handler=NULL;

static void on_ui(void* arg, esp_event_base_t base, int32_t event, void* data)
{
	if (alert_handler) {
		event=(*alert_handler)((void*)alert_text, event, data);
	} else if (alert_text) {
		alert_handler=&menu_prompt_handler;
		event=(*alert_handler)((void*)alert_text, UI_E_ENTER, NULL);
	}

	if (alert_handler && (event==UI_E_RETURN || event==UI_E_DONE || event==UI_E_CLOSE)) {
		alert_text=NULL;
		alert_handler=NULL;
	}

	event=ui_main(NULL, event, data);
}

esp_err_t ui_init ()
{
	/* no need to free resources if failed, coz system will reset or halt */
	esp_err_t ret;
    font_lib_t * font_t;

    /* set colors */
	ST7789_encode_color32(color_bg,   COLOR_BG);
	ST7789_encode_color32(color_bg+2, COLOR_BG);
    ST7789_encode_color32(color_IC, COLOR_BG);
    ST7789_encode_color32(color_IC+2, COLOR_IC);
    ST7789_encode_color32(color_IS, COLOR_BG);
    ST7789_encode_color32(color_IS+2, COLOR_IS);
    ST7789_encode_color32(color_VC, COLOR_BG);
    ST7789_encode_color32(color_VC+2, COLOR_VC);
    ST7789_encode_color32(color_VS, COLOR_BG);
    ST7789_encode_color32(color_VS+2, COLOR_VS);
    ST7789_encode_color32(color_OFF, COLOR_BG);
    ST7789_encode_color32(color_OFF+2, COLOR_OFF);
    ST7789_encode_color32(color_IT, COLOR_BG);
    ST7789_encode_color32(color_IT+2, COLOR_IT);
    ST7789_encode_color32(color_VT, COLOR_BG);
    ST7789_encode_color32(color_VT+2, COLOR_VT);
    ST7789_encode_color32(color_PT, COLOR_BG);
    ST7789_encode_color32(color_PT+2, COLOR_PT);
    ST7789_encode_color32(color_ET, COLOR_BG);
    ST7789_encode_color32(color_ET+2, COLOR_ET);
    ST7789_encode_color32(color_temp, COLOR_BG);
    ST7789_encode_color32(color_temp+2, COLOR_TEMP);
    ST7789_encode_color32(color_message, COLOR_BG);
    ST7789_encode_color32(color_message+2, COLOR_MESSAGE);
    ST7789_encode_color32(color_sign, COLOR_BG);
    ST7789_encode_color32(color_sign+2, COLOR_SIGN);
    ST7789_encode_color32(color_menu, COLOR_BG);
    ST7789_encode_color32(color_menu+2, COLOR_MENU);
    ST7789_encode_color32(color_menuhl, COLOR_BG);
    ST7789_encode_color32(color_menuhl+2, COLOR_MENUHL);
    ST7789_encode_color32(color_menuhint, COLOR_BG);
    ST7789_encode_color32(color_menuhint+2, COLOR_MENUHINT);

    /* load fonts */
    ESP_RETURN_ON_FALSE(
    		   ((ret=font_lib_wrap (&font_message, 7, 12, 1, (7+1)*12/8, FONT_MESSAGE, (const uint32_t *)INDX_MESSAGE, 32700, FONT_SORTED))==ESP_OK)
    		&& ((ret=font_lib_wrap (&font_t, 15, 12, 1, (15+1)*12/8, FONT_HZ, (const uint32_t *)INDX_HZ, 32700, FONT_SORTED))==ESP_OK)
			&& (font_message->next=font_t, font_t=NULL, 1)
    		&& ((ret=font_lib_wrap (&font_IVC, 24, 45, 0, (24+0)*45/8, FONT_IVC, (const uint32_t *)INDX_IVC, 32700, FONT_SORTED))==ESP_OK)
    		&& ((ret=font_lib_wrap (&font_t, 16, 45, 0, (16+0)*45/8, FONT_IVC2, (const uint32_t *)INDX_IVC2, 32700, FONT_SORTED))==ESP_OK)
			&& (font_IVC->next=font_t, font_t=NULL, 1)
    		&& ((ret=font_lib_wrap (&font_IVS, 16, 32, 0, (16+0)*32/8, FONT_IVS, (const uint32_t *)INDX_IVS, 32700, FONT_SORTED))==ESP_OK)
    		&& ((ret=font_lib_wrap (&font_t, 10, 32, 6, (10+6)*32/8, FONT_IVS2, (const uint32_t *)INDX_IVS2, 32700, FONT_SORTED))==ESP_OK)
			&& (font_IVS->next=font_t, font_t=NULL, 1)
    		&& ((ret=font_lib_wrap (&font_t, 26, 32, 6, (26+6)*32/8, FONT_IVS3, (const uint32_t *)INDX_IVS3, 32700, FONT_SORTED))==ESP_OK)
			&& (font_IVS->next->next=font_t, font_t=NULL, 1)
    		&& ((ret=font_lib_wrap (&font_t, 32, 32, 0, (32+0)*32/8, FONT_IVS4, (const uint32_t *)INDX_IVS4, 32700, FONT_SORTED))==ESP_OK)
			&& (font_IVS->next->next->next=font_t, font_t=NULL, 1)
    		&& ((ret=font_lib_wrap (&font_menu, 8, 16, 0, (8+0)*16/8, FONT_MENU, (const uint32_t *)INDX_MENU, 32700, FONT_SORTED))==ESP_OK)
    		&& ((ret=font_lib_wrap (&font_t, 16, 16, 0, (16+0)*16/8, FONT_MENUHZ, (const uint32_t *)INDX_MENUHZ, 32700, FONT_SORTED))==ESP_OK)
			&& (font_menu->next=font_t, font_t=NULL, 1)
    		&& ((ret=font_lib_wrap (&font_sign, 12, 12, 4, (12+4)*12/8, FONT_SIGN, (const uint32_t *)INDX_SIGN, 32700, FONT_SORTED))==ESP_OK)
    			, ret, TAG, "Failed to load fonts (%d:%s)", ret, esp_err_to_name(ret));

	/* init LCD */
    spi_host_device_t spi=LCD_SPI_HOST;
	spi_bus_config_t buscfg={
		 .mosi_io_num=PIN_MOSI,
		 .sclk_io_num=PIN_SPICLK,
		 .miso_io_num=-1,
		 .data2_io_num=-1,
		 .data3_io_num=-1,
		 .data4_io_num=-1,
		 .data5_io_num=-1,
		 .data6_io_num=-1,
		 .data7_io_num=-1,
		 .max_transfer_sz=LCD_BUF_SIZE+16,
		 .flags=SPICOMMON_BUSFLAG_GPIO_PINS | SPICOMMON_BUSFLAG_MASTER | SPICOMMON_BUSFLAG_MOSI | SPICOMMON_BUSFLAG_SCLK
	};
	ESP_RETURN_ON_ERROR(ret=spi_bus_initialize(spi, &buscfg, SPI_DMA_CH_AUTO)
    		, TAG, "Fail to init SPI bus (%d:%s)", ret, esp_err_to_name(ret));
    ESP_RETURN_ON_ERROR(ret=ST7789_alloc(&ui_lcd, spi, ST7789_240X135, ST7789_R0, LCD_SPI_FREQ, LCD_BUF_SIZE, PIN_LCDCS, PIN_RESETLCD, PIN_SPIAUX)
    		, TAG, "Failed to init ST7789 device (%d:%s)", ret, esp_err_to_name(ret));
    ESP_RETURN_ON_ERROR(ret=ui_clear()
    		, TAG, "Failed to clear screen (%d:%s)", ret, esp_err_to_name(ret));

    /* tell main handler to enter */
    on_ui(NULL, UI_E_BASE, UI_E_ENTER, NULL);

    /* register UI event dispatch handler */
    ESP_RETURN_ON_ERROR(ret=esp_event_handler_register_with(app_eloop, UI_E_BASE, ESP_EVENT_ANY_ID, &on_ui, NULL)
    		, TAG, "Failed to register UI event handler (%d:%s)", ret, esp_err_to_name(ret));

    /* init button and EC11 */
    ESP_RETURN_ON_FALSE((ret=button_init(&ibtn, PIN_IBTN, BTN_DEBOUNCE, BTN_LONGTIME, 0))==ESP_OK
    		         && (ret=button_init(&vbtn, PIN_VBTN, BTN_DEBOUNCE, BTN_LONGTIME, 0))==ESP_OK
		, ret, TAG, "Failed to alloc button for EC11 (%d:%s)", ret, esp_err_to_name(ret));

	esp_timer_create_args_t timercfg = {
		.callback = on_timer_button,
		.arg = NULL,
		.dispatch_method = ESP_TIMER_TASK,
		.name = "ui_timer_button"
	};
	ESP_RETURN_ON_FALSE((ret=esp_timer_create(&timercfg, &timer_button))==ESP_OK
			&& (ret=esp_timer_start_periodic(timer_button, PERIOD_SCAN_BUTTON))==ESP_OK
		, ret, TAG, "Failed to start button scan timer (%d:%s)", ret, esp_err_to_name(ret));

    ESP_RETURN_ON_FALSE((ret=rotary_init(&irot, ROTARY_TYPE, 0, PIN_IROTA, PIN_IROTB, ROTARY_DEBOUNCE, ROT_FAST_INTERVAL, ROT_FAST_REPEAT, app_eloop, UI_E_BASE, UI_E_IROTR, UI_E_IROTL, UI_E_IROTR_FAST, UI_E_IROTL_FAST))==ESP_OK
    		         && (ret=rotary_init(&vrot, ROTARY_TYPE, 0, PIN_VROTA, PIN_VROTB, ROTARY_DEBOUNCE, ROT_FAST_INTERVAL, ROT_FAST_REPEAT, app_eloop, UI_E_BASE, UI_E_VROTR, UI_E_VROTL, UI_E_VROTR_FAST, UI_E_VROTL_FAST))==ESP_OK
		, ret, TAG, "Failed to alloc rotary for EC11 (%d:%s)", ret, esp_err_to_name(ret));

    /* init timer for flush event */
	esp_timer_create_args_t timercfg2 = {
		.callback = on_timer_flush,
		.arg = NULL,
		.dispatch_method = ESP_TIMER_TASK,
		.name = "ui_timer_flush"
	};
	ESP_RETURN_ON_FALSE((ret=esp_timer_create(&timercfg2, &timer_flush))==ESP_OK
			&& (ret=esp_timer_start_periodic(timer_flush, PERIOD_FLUSH))==ESP_OK
		, ret, TAG, "Failed to start flush timer (%d:%s)", ret, esp_err_to_name(ret));

    return ESP_OK;
}

esp_err_t ui_alert(const char * text)
{
	alert_text=text;
	return ESP_OK;
}

esp_err_t ui_bar(int x, int y, int width, uint8_t height, const uint8_t *color, const uint8_t * data, int len)
{
	int i=0;
	if (len>width) i=len-width;
	int p=x;
	if (len<width) p+=width-len;

	esp_err_t ret=ESP_OK;

	/* draw it line by line, slower but looking smooth */
	if (p>x && (ret=ST7789_rect (ui_lcd, x, y, p-x, height, color))!=ESP_OK) return ret;
	while (i<len) {
		if (data[i]<height && (ret=ST7789_rect (ui_lcd, p, y, 1, height-data[i], color))!=ESP_OK) return ret;
		if (data[i]>0 && (ret=ST7789_rect (ui_lcd, p, y+height-data[i], 1, data[i], color+2))!=ESP_OK) return ret;
		i++;
		p++;
	}

	return ret;
}

esp_err_t ui_clear()
{
	return ST7789_rect(ui_lcd, 0, 0, UI_LCD_WIDTH, UI_LCD_HEIGHT, color_bg);
}

esp_err_t ui_text (int x, int y, int width, const char *str, const uint8_t *color, const font_lib_t *lib)
{
	int xend=x+width;
	if (xend>UI_LCD_WIDTH) {
		xend=UI_LCD_WIDTH;
		width=UI_LCD_WIDTH-x;
	}

	str=BUNDLESTR(str);
	esp_err_t ret=ST7789_text(ui_lcd, &x, y, width, color, lib, &str,  FONT_UTF8, ST7789_TOP, 0);
	if (ret==ESP_OK && x<xend) ret=ST7789_rect (ui_lcd, x, y, xend-x, lib->height, color);
	return ret;
}

esp_err_t ui_text_wrap (int x, int y, int width, int height, const char *str, const uint8_t *color, const font_lib_t *lib)
{
	int xend=x+width;
	int yend=y+height;
	if (xend>UI_LCD_WIDTH) {
		xend=UI_LCD_WIDTH;
		width=xend-x;
	}
	if (yend>UI_LCD_HEIGHT) {
		yend=UI_LCD_HEIGHT;
		height=yend-y;
	}
	int lineheight=(lib->height*6+4)/5;
	int xorg=x, yorg=y;

	str=BUNDLESTR(str);
	esp_err_t ret=ST7789_text_wrap(ui_lcd, &x, &y, width, height, lineheight, color, lib, &str,  FONT_UTF8, ST7789_TOP, 0);
	if (ret==ESP_OK && x<xend) ret=ST7789_rect (ui_lcd, x, yorg, xend-x, y-yorg, color);
	if (ret==ESP_OK && y<yend) ret=ST7789_rect (ui_lcd, xorg, y, xend-xorg, yend-y, color);
	return ret;
}

esp_err_t ui_image (int x, int y, int width, int height, const uint8_t * pixels, int px_width)
{
	esp_err_t ret=ST7789_set_window(ui_lcd, x, y, width, height);
	if (ret==ESP_OK) ret=ST7789_blit (ui_lcd, pixels, width*height, width, px_width-width);
	return ret;
}
