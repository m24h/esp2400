/**
 * Know nothing about copy from left to right,
 * if someone wanna use it, just copy it from left to right
 *                by author m24h
 */
#include "app.h"
#include "main.h"
#include "stat.h"
#include "wifi.h"
#include "ui.h"
#include "menu.h"

#define ROT_FAST_MULTI          10 /* multiple changes if fast rotation */
#define IVS_FRAME_HOLD_TIME 500000

/* from ui_menu.c */
extern menu_list_t ui_menu_top;
extern menu_option_t ui_menu_pick_i;
extern menu_option_t ui_menu_pick_v;

static void ivstr(char * s, int num)
{
	if (num>=0) {
		num=(num+5)/10;
		if (num>9999) {
			*(s++)='+';
			*(s++)=' ';
			*(s++)='.';
			*(s++)='O';
			*(s++)='V';
			*s=0;
			return;
		}
		if (num>=1000)
			*(s++)='0'+num/1000;
		else
			*(s++)=' ';
		*(s++)='0'+(num/100)%10;
		*(s++)='.';
		*(s++)='0'+(num/10)%10;
		*(s++)='0'+num%10;
		*s=0;
	} else {
		num=(-num+50)/100;
		if (num>999) {
			*(s++)='-';
			*(s++)=' ';
			*(s++)='.';
			*(s++)='O';
			*(s++)='V';
			*s=0;
			return;
		}

		*(s++)='-';
		if (num>=100)
			*(s++)='0'+num/100;
		else
			*(s++)=' ';
		*(s++)='0'+(num/10)%10;
		*(s++)='.';
		*(s++)='0'+num%10;
		*s=0;
	}
}

static int  last_ic=INT_MIN;
static void show_ic()
{
	if (main_vars.ic==last_ic) return;
	else last_ic=main_vars.ic;

	char s[8];
	ivstr(s, last_ic);
	/* x:128-239, width:4*24+16=112 height:4+45+4*/
	ui_text(128, 42, 112, s, color_IC, font_IVC);
}

static int  last_vc=INT_MIN;
static void show_vc()
{
	if (main_vars.vc==last_vc) return;
	else last_vc=main_vars.vc;

	char s[8];
	ivstr(s, last_vc);
	/* x: 0-111, width:4*24+16=112 */
	ui_text(0, 42, 112, s, color_VC, font_IVC);
}

static int     last_is=INT_MIN;
static int     last_is_on=INT_MIN;
static int64_t last_is_frame=0;
static void show_is(int cached)
{
	int same=(main_vars.is==last_is && last_is_on==main_vars.on);
	if (cached && same) {
		if (last_is_frame!=0 && last_is_frame+IVS_FRAME_HOLD_TIME<esp_timer_get_time()) {
			ST7789_frame(ui_lcd, 134, 0, 106, 38, 1, color_IS);
			last_is_frame=0;
		}
		return;
	} else {
		last_is=main_vars.is;
		last_is_on=main_vars.on;
	}

	char s[8];
	ivstr(s, last_is);
	s[5]='A';
	s[6]=0;
	/* x:137-236, width:4*16+10+26=100, height:3+32+3 */
	ui_text(137, 3, 100, s, last_is_on?color_IS:color_OFF, font_IVS);

	if (!same) {
		last_is_frame=esp_timer_get_time();
		ST7789_frame(ui_lcd, 134, 0, 106, 38, 1, color_IS+2);
	}
}

static int     last_vs=INT_MIN;
static int     last_vs_on=INT_MIN;
static int64_t last_vs_frame=0;
static void show_vs(int cached)
{
	int same=(main_vars.vs==last_vs && last_vs_on==main_vars.on);
	if (cached && same) {
		if (last_vs_frame!=0 && last_vs_frame+IVS_FRAME_HOLD_TIME<esp_timer_get_time()) {
			ST7789_frame(ui_lcd, 0, 0, 106, 38, 1, color_VS);
			last_vs_frame=0;
		}
		return;
	} else {
		last_vs=main_vars.vs;
		last_vs_on=main_vars.on;
	}

	char s[8];
	ivstr(s, last_vs);
	s[5]='V';
	s[6]=0;
	/* x:3-102, width:4*16+10+26=100, height:3+32+3 */
	ui_text(3, 3, 100, s, last_vs_on?color_VS:color_OFF, font_IVS);

	if (!same) {
		last_vs_frame=esp_timer_get_time();
		ST7789_frame(ui_lcd, 0, 0, 106, 38, 1, color_VS+2);
	}
}

static int  last_temp=INT_MIN;
static void show_temp()
{
	int t=(main_vars.temp+500)/1000;
	if (t==last_temp) return;
	else last_temp=t;

	char s[8];
	sprintf(s, "%3d", last_temp);
	/* x:108-131, width:7*3, height:3+12+2+12+3 */
	ui_text(109, 4, 21, s, color_temp, font_message);
	/* width: 15 for HZ */
	ui_text(114, 18, 15, "℃", color_temp, font_message);
}

static int64_t last_it=-1;
static void show_it()
{
	if (stat_data.s.last_enclose==last_it) return;
	else last_it=stat_data.s.last_enclose;

	int8_t td [120];

	STAT_LOCK_R();

	int n=stat_data.s.num;
	int s=stat_data.s.offset;
	int i;
	if (n>119) {
		s+=n-119;
		n=119;
	}

	for (i=0; i<n; i++, s++) {
		if (s>=STAT_SIZE) s-=STAT_SIZE;
		td[i]=(int8_t)((stat_data.s.items[s].i.avg-I_MIN)*27/(I_MAX-I_MIN));
		if (td[i]<1) td[i]=1;
		else if (td[i]>27) td[i]=27;
	}

	STAT_UNLOCK_R();

	ui_bar(121, 92, 119, 27, color_IT, (uint8_t*)td, n);
}

static int64_t last_vt=-1;
static void show_vt()
{
	if (stat_data.s.last_enclose==last_vt) return;
	else last_vt=stat_data.s.last_enclose;

	int8_t td [119];

	STAT_LOCK_R();

	int n=stat_data.s.num;
	int s=stat_data.s.offset;
	int i;
	if (n>119) {
		s+=n-119;
		n=119;
	}
	for (i=0; i<n; i++, s++) {
		if (s>=STAT_SIZE) s-=STAT_SIZE;
		td[i]=(int8_t)((stat_data.s.items[s].v.avg-V_MIN)*27/(V_MAX-V_MIN));
		if (td[i]<1) td[i]=1;
		else if (td[i]>27) td[i]=27;
	}

	STAT_UNLOCK_R();

	ui_bar(0, 92, 119, 27, color_VT, (uint8_t*)td, n);
}

static char last_msg[40]={0xff, 0x0};
static void show_message()
{
	if (!strncmp(last_msg, main_vars.msg, sizeof(last_msg))) return;
	else strlcpy(last_msg, main_vars.msg, sizeof(last_msg));

	int w=UI_LCD_WIDTH-14*3; /* for signs */
	if (last_msg[0])
		ui_text(0, 123, w , last_msg, color_message, font_message);
	else
		ui_text(0, 123, w , BUNDLE.msg.ready, color_message, font_message);
}

static int last_sign=0;
static void show_sign()
{
	int sign=0;
	if (main_vars.discharge>2) sign|=1;
	if (main_vars.fan>2) sign|=2;
	if (wifi_state_sta>=4 || wifi_state_ap>=4) sign|=4;
	if (sign==last_sign) return;
	else last_sign=sign;

	ui_text(UI_LCD_WIDTH-12, 123, 12, (sign & 1)?"\x02":"\x01", color_sign, font_sign);
	ui_text(UI_LCD_WIDTH-26, 123, 12, (sign & 2)?"\x03":"\x01", color_sign, font_sign);
	ui_text(UI_LCD_WIDTH-40, 123, 12, (sign & 4)?"\x04":"\x01", color_sign, font_sign);
}

static ui_handler_t sub_handler=NULL;
static       void * sub_arg=NULL;

int32_t ui_main(void * arg, int32_t event, void *data)
{
	int t;

	/* toggle outputing status if V button long pressed */
	if (event== UI_E_VLONG) {
		main_vars.on=main_vars.on?0:1;
		return UI_E_NONE;
	}

	if (sub_handler) {
		/* exit sub-menu if I button long pressed */
		if (event==UI_E_ILONG) event=UI_E_RETURN;
		else event=(*sub_handler)(sub_arg, event, data);
	}

main_process:
	switch(event) {
	case UI_E_CLOSE:
		sub_handler=NULL;
		return UI_E_CLOSE;
	case UI_E_LEAVE:
		sub_handler=NULL;
		return UI_E_RETURN;
	case UI_E_ENTER:
	case UI_E_RETURN:
		sub_handler=NULL;
		last_ic=INT_MIN;
		last_vc=INT_MIN;
		last_temp=INT_MIN;
		last_it=-1;
		last_vt=-1;
		last_sign=-1;
		last_msg[0]=0xff;
		last_msg[1]=0x0;
		ui_clear();
		show_temp();
		show_vs(0);
		show_is(0);
		show_vc();
		show_ic();
		show_vt();
		show_it();
		show_message();
		show_sign();
		break;
	case UI_E_DONE:
		sub_handler=NULL;
		break;
	case UI_E_FLUSH:
		show_temp();
		show_vs(1);
		show_is(1);
		show_vc();
		show_ic();
		show_vt();
		show_it();
		show_message();
		show_sign();
		break;
	case UI_E_IROTL:
		t=(main_vars.is>=0?(main_vars.is+5):(main_vars.is-5))/10;
		t=(t-1)*10;
		main_vars.is=t<I_MIN?I_MIN:t;
		show_is(0);
		break;
	case UI_E_IROTL_FAST:
		t=(main_vars.is>=0?(main_vars.is+5):(main_vars.is-5))/10;
		t=(t-ROT_FAST_MULTI)*10;
		main_vars.is=t<I_MIN?I_MIN:t;
		show_is(0);
		break;
	case UI_E_IROTR:
		t=(main_vars.is>=0?(main_vars.is+5):(main_vars.is-5))/10;
		t=(t+1)*10;
		main_vars.is=t>I_MAX?I_MAX:t;
		show_is(0);
		break;
	case UI_E_IROTR_FAST:
		t=(main_vars.is>=0?(main_vars.is+5):(main_vars.is-5))/10;
		t=(t+ROT_FAST_MULTI)*10;
		main_vars.is=t>I_MAX?I_MAX:t;
		show_is(0);
		break;
	case UI_E_VROTL:
		t=(main_vars.vs>=0?(main_vars.vs+5):(main_vars.vs-5))/10;
		t=(t-1)*10;
		main_vars.vs=t<V_MIN?V_MIN:t;
		show_vs(0);
		break;
	case UI_E_VROTL_FAST:
		t=(main_vars.vs>=0?(main_vars.vs+5):(main_vars.vs-5))/10;
		t=(t-ROT_FAST_MULTI)*10;
		main_vars.vs=t<V_MIN?V_MIN:t;
		show_vs(0);
		break;
	case UI_E_VROTR:
		t=(main_vars.vs>=0?(main_vars.vs+5):(main_vars.vs-5))/10;
		t=(t+1)*10;
		main_vars.vs=t>V_MAX?V_MAX:t;
		show_vs(0);
		break;
	case UI_E_VROTR_FAST:
		t=(main_vars.vs>=0?(main_vars.vs+5):(main_vars.vs-5))/10;
		t=(t+ROT_FAST_MULTI)*10;
		main_vars.vs=t>V_MAX?V_MAX:t;
		show_vs(0);
		break;
	case UI_E_ICLICK:
		sub_handler=&menu_option_handler;
		sub_arg=&ui_menu_pick_i;
		event=(*sub_handler)(sub_arg, UI_E_ENTER, NULL);
		goto main_process;
	case UI_E_VCLICK:
		sub_handler=&menu_option_handler;
		sub_arg=&ui_menu_pick_v;
		event=(*sub_handler)(sub_arg, UI_E_ENTER, NULL);
		goto main_process;
	case UI_E_ILONG:
		sub_handler=&menu_list_handler;
		sub_arg=&ui_menu_top;
		event=(*sub_handler)(sub_arg, UI_E_ENTER, NULL);
		goto main_process;
	}

	return UI_E_NONE;
}


