/**
 * Know nothing about copy from left to right,
 * if someone wanna use it, just copy it from left to right
 *                by author m24h
 */
#include "app.h"
#include "main.h"
#include "stat.h"
#include "ui.h"

static int  last_ic=INT_MIN;
static void show_ic()
{
	if (main_vars.ic==last_ic) return;
	else last_ic=main_vars.ic;

	char s[16];
	snprintf(s, sizeof(s),"%6.3lfA", (double)last_ic/1000);
	/* height 32 */
	ui_text(120, 0, 120, s, color_IC, font_IVS);
}

static int  last_vc=INT_MIN;
static void show_vc()
{
	if (main_vars.vc==last_vc) return;
	else last_vc=main_vars.vc;

	char s[16];
	snprintf(s, sizeof(s),"%6.3lfV", (double)last_vc/1000);
	/* height 32 */
	ui_text(0, 0, 120, s, color_VC, font_IVS);
}

static void show_title()
{
	ui_text(20,  38, 80, BUNDLE.menu.stat_avg, color_menu, font_menu);
	ui_text(100,  38, 80, BUNDLE.menu.stat_min, color_menu, font_menu);
	ui_text(180,  38, 80, BUNDLE.menu.stat_max, color_menu, font_menu);
	ui_text(0, 118, 80, BUNDLE.menu.stat_e, color_menu, font_menu);
	ui_text(200, 118, UI_LCD_WIDTH-200, BUNDLE.menu.stat_exit, color_menuhint, font_menu);
}

static int last_v_avg=INT_MIN;
static int last_v_min=INT_MIN;
static int last_v_max=INT_MIN;
static int last_i_avg=INT_MIN;
static int last_i_min=INT_MIN;
static int last_i_max=INT_MIN;
static int last_p_avg=INT_MIN;
static int last_p_min=INT_MIN;
static int last_p_max=INT_MIN;
static int64_t last_e=LLONG_MIN;
static void show_stat()
{
	char buf[32];
	if (last_v_avg!=stat_data.t.v.avg) {
		last_v_avg=stat_data.t.v.avg;
		snprintf(buf, sizeof(buf), "%7.3lfV", (double)last_v_avg/1000);
		ui_text(0,  58, 80, buf, color_VT, font_menu);
	}
	if (last_v_min!=stat_data.t.v.min) {
		last_v_min=stat_data.t.v.min;
		snprintf(buf, sizeof(buf), "%7.3lfV", (double)last_v_min/1000);
		ui_text(80,  58, 80, buf, color_VT, font_menu);
	}
	if (last_v_max!=stat_data.t.v.max) {
		last_v_max=stat_data.t.v.max;
		snprintf(buf, sizeof(buf), "%7.3lfV", (double)last_v_max/1000);
		ui_text(160, 58, 80, buf, color_VT, font_menu);
	}
	if (last_i_avg!=stat_data.t.i.avg) {
		last_i_avg=stat_data.t.i.avg;
		snprintf(buf, sizeof(buf), "%7.3lfA", (double)last_i_avg/1000);
		ui_text(0,  78, 80, buf, color_IT, font_menu);
	}
	if (last_i_min!=stat_data.t.i.min) {
		last_i_min=stat_data.t.i.min;
		snprintf(buf, sizeof(buf), "%7.3lfA", (double)last_i_min/1000);
		ui_text(80,  78, 80, buf, color_IT, font_menu);
	}
	if (last_i_max!=stat_data.t.i.max) {
		last_i_max=stat_data.t.i.max;
		snprintf(buf, sizeof(buf), "%7.3lfA", (double)last_i_max/1000);
		ui_text(160, 78, 80, buf, color_IT, font_menu);
	}
	if (last_p_avg!=stat_data.t.p.avg) {
		last_p_avg=stat_data.t.p.avg;
		snprintf(buf, sizeof(buf), "%7.1lfW", (double)last_p_avg/1000);
		ui_text(0,  98, 80, buf, color_PT, font_menu);
	}
	if (last_p_min!=stat_data.t.p.min) {
		last_p_min=stat_data.t.p.min;
		snprintf(buf, sizeof(buf), "%7.1lfW", (double)last_p_min/1000);
		ui_text(80,  98, 80, buf, color_PT, font_menu);
	}
	if (last_p_max!=stat_data.t.p.max) {
		last_p_max=stat_data.t.p.max;
		snprintf(buf, sizeof(buf), "%7.1lfW", (double)last_p_max/1000);
		ui_text(160, 98, 80, buf, color_PT, font_menu);
	}
	if (last_e!=main_vars.e) {
		last_e=main_vars.e;
		snprintf(buf, sizeof(buf), "%lldJ", (last_e+500000)/1000000);
		ui_text(54, 118, 200-54, buf, color_ET, font_menu);
	}
}

int32_t ui_stat2(void * arg, int32_t event, void *data)
{
	switch(event) {
	case UI_E_CLOSE:
		return UI_E_CLOSE;
	case UI_E_LEAVE:
		return UI_E_RETURN;
	case UI_E_ENTER:
	case UI_E_RETURN:
		last_ic=INT_MIN;
		last_vc=INT_MIN;
		last_v_avg=INT_MIN;
		last_v_min=INT_MIN;
		last_v_max=INT_MIN;
		last_i_avg=INT_MIN;
		last_i_min=INT_MIN;
		last_i_max=INT_MIN;
		last_p_avg=INT_MIN;
		last_p_min=INT_MIN;
		last_p_max=INT_MIN;
		last_e=LLONG_MIN;
		ui_clear();
		show_title();
		show_ic();
		show_vc();
		show_stat();
		break;
	case UI_E_DONE:
		break;
	case UI_E_FLUSH:
		show_ic();
		show_vc();
		show_stat();
		break;
	case UI_E_IROTL:
	case UI_E_IROTL_FAST:
	case UI_E_IROTR:
	case UI_E_IROTR_FAST:
	case UI_E_VROTL:
	case UI_E_VROTL_FAST:
	case UI_E_VROTR:
	case UI_E_VROTR_FAST:
		break;
	case UI_E_ICLICK:
	case UI_E_VCLICK:
		return UI_E_RETURN;
	}

	return UI_E_NONE;
}
