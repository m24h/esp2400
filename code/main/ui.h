/**
 * Know nothing about copy from left to right,
 * if someone wanna use it, just copy it from left to right
 *                by author m24h
 */
#pragma once

#include "esp_event.h"
#include "font.h"
#include "ST7789.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * lcd device
 */
#define UI_LCD_WIDTH    240
#define UI_LCD_HEIGHT   135

extern ST7789_t * ui_lcd;

extern font_lib_t * font_IVC;
extern font_lib_t * font_IVS;
extern font_lib_t * font_message;
extern font_lib_t * font_menu;
extern font_lib_t * font_sign;

extern uint8_t color_bg[4];
extern uint8_t color_IC[4];
extern uint8_t color_IS[4];
extern uint8_t color_VC[4];
extern uint8_t color_VS[4];
extern uint8_t color_OFF[4];
extern uint8_t color_IT[4];
extern uint8_t color_VT[4];
extern uint8_t color_PT[4];
extern uint8_t color_ET[4];
extern uint8_t color_temp[4];
extern uint8_t color_message[4];
extern uint8_t color_sign[4];
extern uint8_t color_menu[4];
extern uint8_t color_menuhl[4];
extern uint8_t color_menuhint[4];

ESP_EVENT_DECLARE_BASE(UI_E_BASE);

typedef enum {
	UI_E_NONE=0,      /* NO event */
	UI_E_CLOSE,       /* close all */
	UI_E_LEAVE,       /* tell an UI item to leave */
	UI_E_ENTER,       /* tell an UI item to enter, maybe need to clear LCD and redraw */
	UI_E_DONE,        /* returning of subitem, LCD not changed */
	UI_E_RETURN,      /* returning from subitem, LCD changed */
	UI_E_FLUSH,       /* repeated flush current V/I display */
	UI_E_ICLICK,
	UI_E_ILONG,
	UI_E_IROTL,
	UI_E_IROTL_FAST,
	UI_E_IROTR,
	UI_E_IROTR_FAST,
	UI_E_VCLICK,
	UI_E_VLONG,
	UI_E_VROTL,
	UI_E_VROTL_FAST,
	UI_E_VROTR,
	UI_E_VROTR_FAST,
} ui_event_e;

/**
 * an UI event handler
 * [arg] [data] is customized
 * return the unprocessed event to caller to process
 */
typedef int32_t (*ui_handler_t) (void *arg, int32_t event, void *data);

/**
 * init ui
 */
esp_err_t ui_init ();

/**
 * alert some thing, accept bundle reference and normal string
 */
esp_err_t ui_alert(const char * text);

/**
 * draw a curve bar
 * [data] is bytes of length [len], means the height to bottom
 * curve is right alignment
 * [color](0-1) is background color, [color](2-3) is curve color
 */
esp_err_t ui_bar(int x, int y, int width, uint8_t height, const uint8_t *color, const uint8_t * data, int len);

/**
 * draw a text line
 * [color] is 4 bytes, first 2 bytes is background color, second 2 bytes is text color
 * [width] limits the drawing width, and not-drawed region will be filled using background color
 */
esp_err_t ui_text (int x, int y, int width, const char *str, const uint8_t *color, const font_lib_t *lib);

/**
 * draw a multiline text
 * [color] is 4 bytes, first 2 bytes is background color, second 2 bytes is text color
 * [width] limits the drawing width, [height] limits the drawing height, and not-drawed region will be filled using background color
 * line height is automatically determined, nearly 1/5 of font height
 */
esp_err_t ui_text_wrap (int x, int y, int width, int height, const char *str, const uint8_t *color, const font_lib_t *lib);

/**
 * draw an image from RGB565 binary data [pixels]
 * [px_width] is the image width, [width] is the drawing rectangle's width
 */
esp_err_t ui_image (int x, int y, int width, int height, const uint8_t * pixels, int px_width);

/**
 * clear screen
 */
esp_err_t ui_clear();


#ifdef __cplusplus
}
#endif
