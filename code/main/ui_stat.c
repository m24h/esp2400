/**
 * Know nothing about copy from left to right,
 * if someone wanna use it, just copy it from left to right
 *                by author m24h
 */
#include "app.h"
#include "main.h"
#include "stat.h"
#include "ui.h"

static int period=0; /* 0:s 1:m 2:h */
static int type=0;    /* 0:v 1:i 2:p */

static void show_title()
{
	if (type==0) ui_text(0, 0, 90, BUNDLE.menu.stat_v, color_menu, font_menu);
	else if (type==1) ui_text(0, 0, 90, BUNDLE.menu.stat_i, color_menu, font_menu);
	else  ui_text(0, 0, 75, BUNDLE.menu.stat_p, color_menu, font_menu);

	ui_text(75,  0, 55, BUNDLE.menu.stat_s, period==0?color_menuhl:color_menu, font_menu);
	ui_text(130, 0, 55, BUNDLE.menu.stat_m, period==1?color_menuhl:color_menu, font_menu);
	ui_text(185, 0, 55, BUNDLE.menu.stat_h, period==2?color_menuhl:color_menu, font_menu);

	uint8_t frame_color[2];
	ST7789_encode_color888(frame_color, 80, 80, 80);
	ST7789_frame(ui_lcd, 0, 20, 122, UI_LCD_HEIGHT-20, 1, frame_color);
	ui_text(127, UI_LCD_HEIGHT-16, UI_LCD_WIDTH-125, BUNDLE.menu.stat_clickback, color_menuhint, font_menu);
}

static void type_change(int nt)
{
	nt=nt%3;
	if (nt<0) nt+=3;
	if (nt==type) return;

	type=nt;
	if (type==0) ui_text(0, 0, 75, BUNDLE.menu.stat_v, color_menu, font_menu);
	else if (type==1) ui_text(0, 0, 75, BUNDLE.menu.stat_i, color_menu, font_menu);
	else  ui_text(0, 0, 75, BUNDLE.menu.stat_p, color_menu, font_menu);
}

static void period_change(int np)
{
	np=np%3;
	if (np<0) np+=3;
	if (np==period) return;

	if (period==0) ui_text(75, 0, 55, BUNDLE.menu.stat_s, color_menu, font_menu);
	else if (period==1) ui_text(130, 0, 55, BUNDLE.menu.stat_m, color_menu, font_menu);
	else  ui_text(185, 0, 55, BUNDLE.menu.stat_h, color_menu, font_menu);

	period=np;

	if (period==0) ui_text(75, 0, 55, BUNDLE.menu.stat_s, color_menuhl, font_menu);
	else if (period==1) ui_text(130, 0, 55, BUNDLE.menu.stat_m, color_menuhl, font_menu);
	else  ui_text(185, 0, 55, BUNDLE.menu.stat_h, color_menuhl, font_menu);
}

static int64_t last_enclose=-1;
static int     last_type=-1;
static int     last_period=-1;
static int     last_cur=INT_MIN;
static void show_stat()
{
	union {
		int val[120];
		uint8_t td[120];
		char str[120];
	} u;

	const char * fmt;
	int          cur;
	uint8_t * bar_color;

	if (type==0) {
		bar_color=color_VT;
		fmt="%s: %6.3lfV";
		cur=main_vars.vc;
	} else if (type==1) {
		bar_color=color_IT;
		fmt="%s: %6.3lfA";
		cur=main_vars.ic;
	} else {
		bar_color=color_PT;
		fmt="%s: %6.1lfW";
		cur=main_vars.p;
	}

	if (type!=last_type || period!=last_period || cur!=last_cur) {
		last_cur=cur;
		snprintf(u.str, sizeof(u.str), fmt, BUNDLE.menu.stat_cur, (double)cur/1000);
		ui_text(125, 21, UI_LCD_WIDTH-125, u.str, color_menu, font_menu);
	}

	stat_ring_t * ring=period==0?&stat_data.s:period==1?&stat_data.m:&stat_data.h;
	if (type!=last_type || period!=last_period || ring->last_enclose!=last_enclose) {
		last_enclose=ring->last_enclose;

		int max=INT_MIN;
		int min=INT_MAX;
		int64_t sum=0;

		STAT_LOCK_R();

		int n=ring->num;
		int s=ring->offset;
		int i;
		if (n>120) {
			s+=n-120;
			n=120;
		}

		for (i=0; i<n; i++, s++) {
			if (s>=STAT_SIZE) s-=STAT_SIZE;
			if (type==0) {
				u.val[i]=ring->items[s].v.avg;
				if (max<ring->items[s].v.max) max=ring->items[s].v.max;
				if (min>ring->items[s].v.min) min=ring->items[s].v.min;
			} else if (type==1) {
				u.val[i]=ring->items[s].i.avg;
				if (max<ring->items[s].i.max) max=ring->items[s].i.max;
				if (min>ring->items[s].i.min) min=ring->items[s].i.min;
			} else {
				u.val[i]=ring->items[s].p.avg;
				if (max<ring->items[s].p.max) max=ring->items[s].p.max;
				if (min>ring->items[s].p.min) min=ring->items[s].p.min;
			}
			sum+=u.val[i];
			if (max<u.val[i]) max=u.val[i];
			if (min>u.val[i]) min=u.val[i];
		}

		STAT_UNLOCK_R();

		int barheight=UI_LCD_HEIGHT-20-2;
		if (n<=0) {
			ST7789_rect(ui_lcd, 1, 21, 120, barheight, color_bg);
			ST7789_rect(ui_lcd, 125, 41, UI_LCD_WIDTH-125, 58, color_bg);
		} else {
			int scope=max-min;
			if (scope<=100) scope=100;

			for (i=0; i<n; i++, s++) {
				u.td[i]=(uint8_t)((u.val[i]-min)*(barheight-1)/scope)+1;
			}
			ui_bar(1, 21, 120, barheight, bar_color, u.td, n);

			snprintf(u.str, sizeof(u.str), fmt, BUNDLE.menu.stat_avg, (double)sum/n/1000);
			ui_text(125, 41, UI_LCD_WIDTH-125, u.str, color_menu, font_menu);
			snprintf(u.str, sizeof(u.str), fmt, BUNDLE.menu.stat_min, (double)min/1000);
			ui_text(125, 61, UI_LCD_WIDTH-125, u.str, color_menu, font_menu);
			snprintf(u.str, sizeof(u.str), fmt, BUNDLE.menu.stat_max, (double)max/1000);
			ui_text(125, 81, UI_LCD_WIDTH-125, u.str, color_menu, font_menu);
		}
	}

	last_period=period;
	last_type=type;
}

int32_t ui_stat(void * arg, int32_t event, void *data)
{
	switch(event) {
	case UI_E_CLOSE:
		return UI_E_CLOSE;
	case UI_E_LEAVE:
		return UI_E_RETURN;
	case UI_E_ENTER:
	case UI_E_RETURN:
		type=(int)arg;
		period=0;
		last_enclose=-1;
		last_type=-1;
		last_period=-1;
		last_cur=INT_MIN;
		ui_clear();
		show_title();
		break;
	case UI_E_DONE:
		break;
	case UI_E_FLUSH:
		show_stat();
		break;
	case UI_E_IROTL:
	case UI_E_IROTL_FAST:
		period_change(period-1);
		break;
	case UI_E_IROTR:
	case UI_E_IROTR_FAST:
		period_change(period+1);
		break;
	case UI_E_VROTL:
	case UI_E_VROTL_FAST:
		type_change(type-1);
		break;
	case UI_E_VROTR:
	case UI_E_VROTR_FAST:
		type_change(type+1);
		break;
	case UI_E_ICLICK:
	case UI_E_VCLICK:
		return UI_E_RETURN;
	}

	return UI_E_NONE;
}
