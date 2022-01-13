/**
 * Know nothing about copy from left to right,
 * if someone wanna use it, just copy it from left to right
 *                by author m24h
 */
#include <math.h>

#include "app.h"
#include "wifi.h"
#include "menu.h"
#include "conf.h"
#include "main.h"
#include "stat.h"
#include "ui.h"

extern int32_t ui_stat(void * arg, int32_t event, void *data);
extern int32_t ui_stat2(void * arg, int32_t event, void *data);

/** static buffers */
static union {
	struct {
		char         buff[QUICK_POINTS][8];
		const char * idx[QUICK_POINTS];
	} init_quick;

	struct {
		char         buff[CAL_POINTS*2][8];
		const char * idx[CAL_POINTS*2];
		int32_t      value[CAL_POINTS*2];
	} cal_del;

	struct {
		char         buff[QUICK_POINTS][8];
		const char * idx[QUICK_POINTS];
	} option_init_quick;

	char wifi_status [256];
} ui_menu_buff;

/**
 * initializer of menu-option getting data from quick-pick I/V
 * [init_arg] can be 1:quick I,  2:quick V
 */
static int ui_menu_option_init_quick(void *arg, const char * const ** items, int num, int * focused)
{
	int i;
	if ((int)arg==1) {
		for (i=0; i<QUICK_POINTS; i++) {
			snprintf(ui_menu_buff.init_quick.buff[i], sizeof(ui_menu_buff.init_quick.buff[i]), "%5.2lfA", (double)(conf_vars.quick.i[i])/1000.0);
			ui_menu_buff.init_quick.idx[i]=ui_menu_buff.init_quick.buff[i];
		}
	} else {
		for (i=0; i<QUICK_POINTS; i++) {
			snprintf(ui_menu_buff.init_quick.buff[i], sizeof(ui_menu_buff.init_quick.buff[i]), "%5.2lfV", (double)(conf_vars.quick.v[i])/1000.0);
			ui_menu_buff.init_quick.idx[i]=ui_menu_buff.init_quick.buff[i];
		}
	}

	*items=ui_menu_buff.init_quick.idx;
	return QUICK_POINTS;
}

/**
 * handler for setting selected quick-pick value to config:
 * can be 1:quick I,  2:quick V
 */
static int32_t ui_menu_option_set_quick (void *arg, int32_t event, void *data)
{
	if (event==UI_E_CLOSE)
		return UI_E_CLOSE;
	else if (event==UI_E_ENTER) {
		if ((int)arg==1) main_vars.is=conf_vars.quick.i[(int)data];
		else main_vars.vs=conf_vars.quick.v[(int)data];
	}

	return UI_E_LEAVE;
}

static void ui_menu_quick_input_v_getter (void * arg, void *data, char *buff, int size)
{
	int *p=conf_vars.quick.v+(int)data;
	snprintf(buff, size,"%-5.2lf", (double)*p/1000);
}

static void ui_menu_quick_input_v_setter (void * arg, void * data, const char *str)
{
	double a;
	int *p=conf_vars.quick.v+(int)data;
	if (sscanf(str, "%lf", &a)==1) {
		int t=(int)round(a*1000);
		if (t<V_MIN) t=V_MIN;
		else if (t>V_MAX) t=V_MAX;
		*p=t;
	}
}

static void ui_menu_quick_input_i_getter (void * arg, void *data, char *buff, int size)
{
	int *p=conf_vars.quick.i+(int)data;
	snprintf(buff,size,"%-5.2lf", (double)*p/1000);
}

static void ui_menu_quick_input_i_setter (void * arg, void * data, const char *str)
{
	double a;
	int *p=conf_vars.quick.i+(int)data;
	if (sscanf(str, "%lf", &a)==1) {
		int t=(int)round(a*1000);
		if (t<I_MIN) t=I_MIN;
		else if (t>I_MAX) t=I_MAX;
		*p=t;
	}
}

static void ui_menu_cal_addi_setter (void * arg, void *data, const char *str)
{
	double a;
	int p;

	if (sscanf(str, "%lf", &a)==1) {
		p=(int)round(a*1000);
		esp_err_t ret=conf_cal_set(&conf_vars.cal.isamp, &conf_vars.cal.iout, p, main_vars.raw_ic, main_vars.raw_is);
		if (ret==ESP_ERR_NO_MEM)
			ui_alert(BUNDLESTRREF(menu.cal_err_noroom));
		else if (ret==ESP_ERR_INVALID_STATE)
			ui_alert(BUNDLESTRREF(menu.cal_err_monoinc));
	}
}

static void ui_menu_cal_addv_setter (void * arg, void *data, const char *str)
{
	double a;
	int p;

	if (sscanf(str, "%lf", &a)==1) {
		p=(int)round(a*1000);
		esp_err_t ret=conf_cal_set(&conf_vars.cal.vsamp, &conf_vars.cal.vout, p, main_vars.raw_vc, main_vars.raw_vs);
		if (ret==ESP_ERR_NO_MEM)
			ui_alert(BUNDLESTRREF(menu.cal_err_noroom));
		else if (ret==ESP_ERR_INVALID_STATE)
			ui_alert(BUNDLESTRREF(menu.cal_err_monoinc));
	}
}

static int ui_menu_cal_del_init (void *arg, const char * const ** items, int num, int * focused)
{
	conf_cal_t * samp = (int)arg==1?&conf_vars.cal.isamp:&conf_vars.cal.vsamp;
	conf_cal_t * out = (int)arg==1?&conf_vars.cal.iout:&conf_vars.cal.vout;
	const char * fmt = (int)arg==1?"%5.2lfA":"%5.2lfV";

	int     n,i,j,t;
	for (n=0; n<samp->num; n++) ui_menu_buff.cal_del.value[n]=samp->p[n].y;
	for (i=0; i<out->num; i++) {
		t=out->p[i].x;
		for (j=0; j<samp->num; j++)
			if (ui_menu_buff.cal_del.value[j]==t)
				break;
		if (j>=samp->num)
			ui_menu_buff.cal_del.value[n++]=t;
	}

	for (i=0; i<n; i++) {
		snprintf(ui_menu_buff.cal_del.buff[i], sizeof(ui_menu_buff.cal_del.buff[i]), fmt, (double)ui_menu_buff.cal_del.value[i]/1000.0);
		ui_menu_buff.cal_del.idx[i]=ui_menu_buff.cal_del.buff[i];
	}

	*items=ui_menu_buff.cal_del.idx;
	return n;
}

static int32_t ui_menu_cal_del_delete (void *arg, int32_t event, void *data)
{
	if (event==UI_E_CLOSE)
		return UI_E_CLOSE;
	else if (event==UI_E_ENTER) {
		conf_cal_t * samp = (int)arg==1?&conf_vars.cal.isamp:&conf_vars.cal.vsamp;
		conf_cal_t * out = (int)arg==1?&conf_vars.cal.iout:&conf_vars.cal.vout;
		esp_err_t ret=conf_cal_del(samp, out, ui_menu_buff.cal_del.value[(int)data]);
		if (ret==ESP_ERR_INVALID_SIZE)
			ui_alert(BUNDLESTRREF(menu.cal_err_2points));
	}

	return UI_E_LEAVE;
}

static int32_t ui_menu_wifi_status (void *arg, int32_t event, void *data)
{
	if (event==UI_E_ENTER) {
		if (!wifi_intf_sta || !wifi_intf_ap) {
			arg="Network is not initialized, retry few monments later";
		} else {
			esp_netif_ip_info_t sta, ap;
			esp_err_t ret;
			if ((ret=esp_netif_get_ip_info(wifi_intf_sta, &sta))==ESP_OK &&
				(ret=esp_netif_get_ip_info(wifi_intf_ap, &ap))==ESP_OK) {
				snprintf(ui_menu_buff.wifi_status, sizeof(ui_menu_buff.wifi_status),
						BUNDLE.menu.wifi_status_prompt,
						IP2STR(&sta.ip), IP2STR(&sta.netmask), IP2STR(&sta.gw),
						IP2STR(&ap.ip), IP2STR(&ap.netmask));
			} else {
				snprintf(ui_menu_buff.wifi_status, sizeof(ui_menu_buff.wifi_status),
						BUNDLE.menu.wifi_status_err, ret);
			}
			arg=ui_menu_buff.wifi_status;
		}
	}

	return menu_prompt_handler(arg, event, NULL);
}

static esp_err_t stat_zero()
{
	main_vars.e=0;
	return stat_reset_ts();
}

static const char * onoff_items [] ={
	BUNDLESTRREF(menu.onoff_off),
	BUNDLESTRREF(menu.onoff_on),
};

static menu_confirm_t ui_menu_conf_save={
	.hint=BUNDLESTRREF(menu.conf_save_hint),
	.handler=&menu_caller_simple,
	.arg=&conf_save
};

static menu_confirm_t ui_menu_conf_load={
	.hint=BUNDLESTRREF(menu.conf_load_hint),
	.handler=menu_caller_simple,
	.arg=&conf_load
};

static menu_confirm_t ui_menu_conf_default={
	.hint=BUNDLESTRREF(menu.conf_default_hint),
	.handler=menu_caller_simple,
	.arg=&conf_default
};

static menu_confirm_t ui_menu_conf_format={
	.hint=BUNDLESTRREF(menu.conf_format_hint),
	.handler=menu_caller_simple,
	.arg=&app_format
};

static menu_confirm_t ui_menu_conf_reset={
	.hint=BUNDLESTRREF(menu.conf_reset_hint),
	.handler=menu_caller_simple,
	.arg=&app_reset
};

static const menu_list_item_t ui_menu_conf_items [] ={
	{
		.text=BUNDLESTRREF(menu.back),
		.arg=NULL,
		.handler=&menu_return_handler
	},{
		.text=BUNDLESTRREF(menu.conf_save),
		.arg=&ui_menu_conf_save,
		.handler=&menu_confirm_handler
	},{
		.text=BUNDLESTRREF(menu.conf_load),
		.arg=&ui_menu_conf_load,
		.handler=&menu_confirm_handler
	}, {
		.text=BUNDLESTRREF(menu.conf_default),
		.arg=&ui_menu_conf_default,
		.handler=&menu_confirm_handler
	}, {
		.text=BUNDLESTRREF(menu.conf_format),
		.arg=&ui_menu_conf_format,
		.handler=&menu_confirm_handler
	}, {
		.text=BUNDLESTRREF(menu.conf_reset),
		.arg=&ui_menu_conf_reset,
		.handler=&menu_confirm_handler
	},
};

static menu_list_t ui_menu_conf = {
	.items=ui_menu_conf_items,
	.num=sizeof(ui_menu_conf_items)/sizeof(ui_menu_conf_items[0]),
};

static menu_input_t ui_menu_quick_v_input={
	.chars="\x08\x7f-.0123456789",
	.size=16,
	.getter=&ui_menu_quick_input_v_getter,
	.setter=&ui_menu_quick_input_v_setter,
};

static menu_option_t ui_menu_quick_v={
	.cols=4,
	.init=&ui_menu_option_init_quick,
	.init_arg=(void*)2,
	.handler=&menu_input_handler,
	.arg=&ui_menu_quick_v_input
};

static menu_input_t ui_menu_quick_i_input={
	.chars="\x08\x7f-.0123456789",
	.size=16,
	.getter=&ui_menu_quick_input_i_getter,
	.setter=&ui_menu_quick_input_i_setter,
};

static menu_option_t ui_menu_quick_i={
	.cols=4,
	.init=&ui_menu_option_init_quick,
	.init_arg=(void*)1,
	.handler=&menu_input_handler,
	.arg=&ui_menu_quick_i_input
};

static const menu_list_item_t menu_quick_items [] ={
	{
		.text=BUNDLESTRREF(menu.back),
		.arg=NULL,
		.handler=&menu_return_handler
	},{
		.text=BUNDLESTRREF(menu.quick_v),
		.arg=&ui_menu_quick_v,
		.handler=&menu_option_handler
	},{
		.text=BUNDLESTRREF(menu.quick_i),
		.arg=&ui_menu_quick_i,
		.handler=&menu_option_handler
	},
};

static menu_list_t ui_menu_quick = {
	.items=menu_quick_items,
	.num=sizeof(menu_quick_items)/sizeof(menu_quick_items[0]),
};

static menu_input_t ui_menu_cal_addv_input={
	.chars="\x08\x7f-.0123456789",
	.size=16,
	.getter=&menu_input_getter_iv,
	.getter_arg=&main_vars.vc,
	.setter=&ui_menu_cal_addv_setter,
};

static menu_confirm_t ui_menu_cal_addv={
	.hint=BUNDLESTRREF(menu.cal_addv_hint),
	.handler=&menu_input_handler,
	.arg=&ui_menu_cal_addv_input
};

static menu_input_t ui_menu_cal_addi_input={
	.chars="\x08\x7f-.0123456789",
	.size=16,
	.getter=&menu_input_getter_iv,
	.getter_arg=&main_vars.ic,
	.setter=&ui_menu_cal_addi_setter,
};

static menu_confirm_t ui_menu_cal_addi={
	.hint=BUNDLESTRREF(menu.cal_addi_hint),
	.handler=&menu_input_handler,
	.arg=&ui_menu_cal_addi_input
};

static menu_confirm_t ui_menu_cal_delv_confirm={
	.hint=BUNDLESTRREF(menu.cal_delv_hint),
	.handler=&ui_menu_cal_del_delete,
	.arg=(void*)2
};

static menu_option_t ui_menu_cal_delv={
	.cols=4,
	.init=&ui_menu_cal_del_init,
	.init_arg=(void*)2,
	.handler=&menu_confirm_handler,
	.arg=&ui_menu_cal_delv_confirm
};

static menu_confirm_t ui_menu_cal_deli_confirm={
	.hint=BUNDLESTRREF(menu.cal_deli_hint),
	.handler=&ui_menu_cal_del_delete,
	.arg=(void*)1
};

static menu_option_t ui_menu_cal_deli={
	.cols=4,
	.init=&ui_menu_cal_del_init,
	.init_arg=(void*)1,
	.handler=&menu_confirm_handler,
	.arg=&ui_menu_cal_deli_confirm
};

static const menu_list_item_t ui_menu_cal_items [] ={
	{
		.text=BUNDLESTRREF(menu.back),
		.arg=NULL,
		.handler=&menu_return_handler
	},{
		.text=BUNDLESTRREF(menu.cal_addv),
		.arg=&ui_menu_cal_addv,
		.handler=&menu_confirm_handler
	},{
		.text=BUNDLESTRREF(menu.cal_delv),
		.arg=&ui_menu_cal_delv,
		.handler=&menu_option_handler
	},{
		.text=BUNDLESTRREF(menu.cal_addi),
		.arg=&ui_menu_cal_addi,
		.handler=&menu_confirm_handler
	},{
		.text=BUNDLESTRREF(menu.cal_deli),
		.arg=&ui_menu_cal_deli,
		.handler=&menu_option_handler
	},
};

static menu_list_t ui_menu_cal = {
	.items=ui_menu_cal_items,
	.num=sizeof(ui_menu_cal_items)/sizeof(ui_menu_cal_items[0]),
};

static menu_option_t ui_menu_wifi_sta_on={
	.items=onoff_items,
	.num=sizeof(onoff_items)/sizeof(onoff_items[0]),
	.cols=1,
	.init=&menu_option_init_index,
	.init_arg=&(conf_vars.sta.on),
	.handler=&menu_option_set_index,
	.arg=&(conf_vars.sta.on)
};

static menu_input_t ui_menu_wifi_sta_ssid={
	.chars="\x08\x7f!\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~",
	.size=32,
	.getter=&menu_input_getter_str,
	.getter_arg=conf_vars.sta.ssid,
	.setter=&menu_input_setter_str,
	.setter_arg=conf_vars.sta.ssid,
};

static menu_option_t ui_menu_wifi_sta_auth={
	.items=conf_opt_sta_auth,
	.num=sizeof(conf_opt_sta_auth)/sizeof(conf_opt_sta_auth[0]),
	.cols=1,
	.init=&menu_option_init_index,
	.init_arg=&(conf_vars.sta.auth),
	.handler=&menu_option_set_index,
	.arg=&(conf_vars.sta.auth)
};

static menu_input_t ui_menu_wifi_sta_pass={
	.chars="\x08\x7f!\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~",
	.size=16,
	.getter=&menu_input_getter_str,
	.getter_arg=conf_vars.sta.pass,
	.setter=&menu_input_setter_str,
	.setter_arg=conf_vars.sta.pass,
};

static menu_input_t ui_menu_wifi_sta_ip={
	.chars="\x08\x7f.0123456789",
	.size=16,
	.getter=&menu_input_getter_ip,
	.getter_arg=&conf_vars.sta.ip,
	.setter=&menu_input_setter_ip,
	.setter_arg=&conf_vars.sta.ip,
};

static menu_input_t ui_menu_wifi_sta_mask={
	.chars="\x08\x7f.0123456789",
	.size=16,
	.getter=&menu_input_getter_ip,
	.getter_arg=&conf_vars.sta.mask,
	.setter=&menu_input_setter_ip,
	.setter_arg=&conf_vars.sta.mask,
};

static menu_input_t ui_menu_wifi_sta_gw={
	.chars="\x08\x7f.0123456789",
	.size=16,
	.getter=&menu_input_getter_ip,
	.getter_arg=&conf_vars.sta.gw,
	.setter=&menu_input_setter_ip,
	.setter_arg=&conf_vars.sta.gw,
};

static const menu_list_item_t ui_menu_wifi_sta_items [] ={
		{
			.text=BUNDLESTRREF(menu.back),
			.arg=NULL,
			.handler=&menu_return_handler
		},{
			.text=BUNDLESTRREF(menu.wifi_sta_on),
			.arg=&ui_menu_wifi_sta_on,
			.handler=&menu_option_handler
		},{
			.text=BUNDLESTRREF(menu.wifi_sta_ssid),
			.arg=&ui_menu_wifi_sta_ssid,
			.handler=&menu_input_handler
		},{
			.text=BUNDLESTRREF(menu.wifi_sta_auth),
			.arg=&ui_menu_wifi_sta_auth,
			.handler=&menu_option_handler
		},{
			.text=BUNDLESTRREF(menu.wifi_sta_pass),
			.arg=&ui_menu_wifi_sta_pass,
			.handler=&menu_input_handler
		},{
			.text=BUNDLESTRREF(menu.wifi_sta_ip),
			.arg=&ui_menu_wifi_sta_ip,
			.handler=&menu_input_handler
		},{
			.text=BUNDLESTRREF(menu.wifi_sta_mask),
			.arg=&ui_menu_wifi_sta_mask,
			.handler=&menu_input_handler
		},{
			.text=BUNDLESTRREF(menu.wifi_sta_gw),
			.arg=&ui_menu_wifi_sta_gw,
			.handler=&menu_input_handler
		},
};

static menu_list_t ui_menu_wifi_sta = {
	.items=ui_menu_wifi_sta_items,
	.num=sizeof(ui_menu_wifi_sta_items)/sizeof(ui_menu_wifi_sta_items[0]),
};

static menu_option_t ui_menu_wifi_ap_on={
	.items=onoff_items,
	.num=sizeof(onoff_items)/sizeof(onoff_items[0]),
	.cols=1,
	.init=&menu_option_init_index,
	.init_arg=&(conf_vars.ap.on),
	.handler=&menu_option_set_index,
	.arg=&(conf_vars.ap.on)
};

static menu_input_t ui_menu_wifi_ap_ssid={
	.chars="\x08\x7f-0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz",
	.size=32,
	.getter=&menu_input_getter_str,
	.getter_arg=conf_vars.ap.ssid,
	.setter=&menu_input_setter_str,
	.setter_arg=conf_vars.ap.ssid,
};

static menu_option_t ui_menu_wifi_ap_auth={
	.items=conf_opt_ap_auth,
	.num=sizeof(conf_opt_ap_auth)/sizeof(conf_opt_ap_auth[0]),
	.cols=1,
	.init=&menu_option_init_index,
	.init_arg=&(conf_vars.ap.auth),
	.handler=&menu_option_set_index,
	.arg=&(conf_vars.ap.auth)
};

static menu_input_t ui_menu_wifi_ap_pass={
	.chars="\x08\x7f!\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~",
	.size=16,
	.getter=&menu_input_getter_str,
	.getter_arg=conf_vars.ap.pass,
	.setter=&menu_input_setter_str,
	.setter_arg=conf_vars.ap.pass,
};

static menu_input_t ui_menu_wifi_ap_ip={
	.chars="\x08\x7f.0123456789",
	.size=16,
	.getter=&menu_input_getter_ip,
	.getter_arg=&conf_vars.ap.ip,
	.setter=&menu_input_setter_ip,
	.setter_arg=&conf_vars.ap.ip,
};

static menu_input_t ui_menu_wifi_ap_mask={
	.chars="\x08\x7f.0123456789",
	.size=16,
	.getter=&menu_input_getter_ip,
	.getter_arg=&conf_vars.ap.mask,
	.setter=&menu_input_setter_ip,
	.setter_arg=&conf_vars.ap.mask,
};

static const menu_list_item_t ui_menu_wifi_ap_items [] ={
	{
		.text=BUNDLESTRREF(menu.back),
		.arg=NULL,
		.handler=&menu_return_handler
	},{
		.text=BUNDLESTRREF(menu.wifi_ap_on),
		.arg=&ui_menu_wifi_ap_on,
		.handler=&menu_option_handler
	},{
		.text=BUNDLESTRREF(menu.wifi_ap_ssid),
		.arg=&ui_menu_wifi_ap_ssid,
		.handler=&menu_input_handler
	},{
		.text=BUNDLESTRREF(menu.wifi_ap_auth),
		.arg=&ui_menu_wifi_ap_auth,
		.handler=&menu_option_handler
	},{
		.text=BUNDLESTRREF(menu.wifi_ap_pass),
		.arg=&ui_menu_wifi_ap_pass,
		.handler=&menu_input_handler
	},{
		.text=BUNDLESTRREF(menu.wifi_ap_ip),
		.arg=&ui_menu_wifi_ap_ip,
		.handler=&menu_input_handler
	},{
		.text=BUNDLESTRREF(menu.wifi_ap_mask),
		.arg=&ui_menu_wifi_ap_mask,
		.handler=&menu_input_handler
	},
};

static menu_list_t ui_menu_wifi_ap = {
	.items=ui_menu_wifi_ap_items,
	.num=sizeof(ui_menu_wifi_ap_items)/sizeof(ui_menu_wifi_ap_items[0]),
};

static menu_confirm_t ui_menu_wifi_reset={
	.hint=BUNDLESTRREF(menu.wifi_reset_hint),
	.handler=&menu_caller_simple,
	.arg=&wifi_reset
};

static menu_input_t ui_menu_wifi_name={
	.chars="\x08\x7f-0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz",
	.size=16,
	.getter=&menu_input_getter_str,
	.getter_arg=conf_vars.name,
	.setter=&menu_input_setter_str,
	.setter_arg=conf_vars.name,
};

static menu_input_t ui_menu_wifi_pass={
	.chars="\x08\x7f!\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~",
	.size=16,
	.getter=&menu_input_getter_str,
	.getter_arg=conf_vars.pass,
	.setter=&menu_input_setter_str,
	.setter_arg=conf_vars.pass,
};

static menu_input_t ui_menu_wifi_admpass={
	.chars="\x08\x7f!\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~",
	.size=16,
	.getter=&menu_input_getter_str,
	.getter_arg=conf_vars.admpass,
	.setter=&menu_input_setter_str,
	.setter_arg=conf_vars.admpass,
};

static const menu_list_item_t ui_menu_wifi_items [] ={
	{
		.text=BUNDLESTRREF(menu.back),
		.arg=NULL,
		.handler=&menu_return_handler
	},{
		.text=BUNDLESTRREF(menu.wifi_status),
		.handler=&ui_menu_wifi_status,
	},{
		.text=BUNDLESTRREF(menu.wifi_name),
		.arg=&ui_menu_wifi_name,
		.handler=&menu_input_handler,
	},{
		.text=BUNDLESTRREF(menu.wifi_sta),
		.arg=&ui_menu_wifi_sta,
		.handler=&menu_list_handler
	},{
		.text=BUNDLESTRREF(menu.wifi_ap),
		.arg=&ui_menu_wifi_ap,
		.handler=&menu_list_handler
	},{
		.text=BUNDLESTRREF(menu.wifi_pass),
		.arg=&ui_menu_wifi_pass,
		.handler=&menu_input_handler
	},{
		.text=BUNDLESTRREF(menu.wifi_admpass),
		.arg=&ui_menu_wifi_admpass,
		.handler=&menu_input_handler
	},{
		.text=BUNDLESTRREF(menu.wifi_reset),
		.arg=&ui_menu_wifi_reset,
		.handler=&menu_confirm_handler
	},
};

static menu_list_t ui_menu_wifi = {
	.items=ui_menu_wifi_items,
	.num=sizeof(ui_menu_wifi_items)/sizeof(ui_menu_wifi_items[0]),
};

static menu_confirm_t ui_menu_stat_zero={
	.hint=BUNDLESTRREF(menu.stat_zero_hint),
	.handler=&menu_caller_simple,
	.arg=&stat_zero
};

static const menu_list_item_t ui_menu_stat_items [] ={
	{
		.text=BUNDLESTRREF(menu.back),
		.arg=NULL,
		.handler=&menu_return_handler
	},{
		.text=BUNDLESTRREF(menu.stat_t),
		.arg=NULL,
		.handler=&ui_stat2
	},{
		.text=BUNDLESTRREF(menu.stat_v),
		.arg=(void*)0,
		.handler=&ui_stat
	},{
		.text=BUNDLESTRREF(menu.stat_i),
		.arg=(void*)1,
		.handler=&ui_stat
	},{
		.text=BUNDLESTRREF(menu.stat_p),
		.arg=(void*)2,
		.handler=&ui_stat
	},{
		.text=BUNDLESTRREF(menu.stat_zero),
		.arg=&ui_menu_stat_zero,
		.handler=&menu_confirm_handler
	},
};

static menu_list_t ui_menu_stat = {
	.items=ui_menu_stat_items,
	.num=sizeof(ui_menu_stat_items)/sizeof(ui_menu_stat_items[0]),
};

static menu_option_t ui_menu_option_lang={
	.items=bundle_langs,
	.num=sizeof(bundle_langs)/sizeof(bundle_langs[0]),
	.cols=1,
	.init=&menu_option_init_index,
	.init_arg=&(conf_vars.bundle),
	.handler=&menu_option_set_index,
	.arg=&(conf_vars.bundle)
};

static const int    opt_fan_temp     [6]={25, 30, 35, 40, 45, 50};
static const char * opt_fan_temp_str [6]={"25 ℃", "30 ℃", "35 ℃", "40 ℃", "45 ℃", "50 ℃"};

menu_option_int_arg ui_menu_option_fan_temp_arg={
	.value=&conf_vars.fan_temp,
	.items=opt_fan_temp,
};

static menu_option_t ui_menu_option_fan_temp={
	.items=opt_fan_temp_str,
	.num=sizeof(opt_fan_temp)/sizeof(opt_fan_temp[0]),
	.cols=1,
	.init=&menu_option_init_int,
	.init_arg=&ui_menu_option_fan_temp_arg,
	.handler=&menu_option_set_int,
	.arg=&ui_menu_option_fan_temp_arg
};

static const int    opt_v_fade       [8]={0, 1000, 2000, 5000, 10000, 20000, 50000, 100000};
static const char * opt_v_fade_str   [8]={BUNDLESTRREF(menu.option_fade_nolimit), "1 V/s", "2 V/s", "5 V/s", "10 V/s", "20 V/s", "50 V/s", "100 V/s"};

menu_option_int_arg ui_menu_option_v_rise_arg={
	.value=&conf_vars.v_rise,
	.items=opt_v_fade,
};

static menu_option_t ui_menu_option_v_rise={
	.items=opt_v_fade_str,
	.num=sizeof(opt_v_fade)/sizeof(opt_v_fade[0]),
	.cols=1,
	.init=&menu_option_init_int,
	.init_arg=&ui_menu_option_v_rise_arg,
	.handler=&menu_option_set_int,
	.arg=&ui_menu_option_v_rise_arg
};

menu_option_int_arg ui_menu_option_v_fall_arg={
	.value=&conf_vars.v_fall,
	.items=opt_v_fade,
};

static menu_option_t ui_menu_option_v_fall={
	.items=opt_v_fade_str,
	.num=sizeof(opt_v_fade)/sizeof(opt_v_fade[0]),
	.cols=1,
	.init=&menu_option_init_int,
	.init_arg=&ui_menu_option_v_fall_arg,
	.handler=&menu_option_set_int,
	.arg=&ui_menu_option_v_fall_arg
};

static const int    opt_i_fade       [8]={0, 1000, 2000, 5000, 10000, 20000, 50000, 100000};
static const char * opt_i_fade_str   [8]={BUNDLESTRREF(menu.option_fade_nolimit), "1 A/s", "2 A/s", "5 A/s", "10 A/s", "20 A/s", "50 A/s", "100 A/s"};

menu_option_int_arg ui_menu_option_i_rise_arg={
	.value=&conf_vars.i_rise,
	.items=opt_i_fade,
};

static menu_option_t ui_menu_option_i_rise={
	.items=opt_i_fade_str,
	.num=sizeof(opt_i_fade)/sizeof(opt_i_fade[0]),
	.cols=1,
	.init=&menu_option_init_int,
	.init_arg=&ui_menu_option_i_rise_arg,
	.handler=&menu_option_set_int,
	.arg=&ui_menu_option_i_rise_arg
};

menu_option_int_arg ui_menu_option_i_fall_arg={
	.value=&conf_vars.i_fall,
	.items=opt_i_fade,
};

static menu_option_t ui_menu_option_i_fall={
	.items=opt_i_fade_str,
	.num=sizeof(opt_i_fade)/sizeof(opt_i_fade[0]),
	.cols=1,
	.init=&menu_option_init_int,
	.init_arg=&ui_menu_option_i_fall_arg,
	.handler=&menu_option_set_int,
	.arg=&ui_menu_option_i_fall_arg
};

static const menu_list_item_t ui_menu_option_items [] ={
	{
		.text=BUNDLESTRREF(menu.back),
		.arg=NULL,
		.handler=&menu_return_handler
	},{
		.text=BUNDLESTRREF(menu.option_lang),
		.arg=&ui_menu_option_lang,
		.handler=&menu_option_handler
	},{
		.text=BUNDLESTRREF(menu.option_fan_temp),
		.arg=&ui_menu_option_fan_temp,
		.handler=&menu_option_handler
	},{
		.text=BUNDLESTRREF(menu.option_v_rise),
		.arg=&ui_menu_option_v_rise,
		.handler=&menu_option_handler
	},{
		.text=BUNDLESTRREF(menu.option_v_fall),
		.arg=&ui_menu_option_v_fall,
		.handler=&menu_option_handler
	},{
		.text=BUNDLESTRREF(menu.option_i_rise),
		.arg=&ui_menu_option_i_rise,
		.handler=&menu_option_handler
	},{
		.text=BUNDLESTRREF(menu.option_i_fall),
		.arg=&ui_menu_option_i_fall,
		.handler=&menu_option_handler
	},
};

static menu_list_t ui_menu_option = {
	.items=ui_menu_option_items,
	.num=sizeof(ui_menu_option_items)/sizeof(ui_menu_option_items[0]),
};

static const menu_list_item_t menu_top_items [] ={
	{
		.text=BUNDLESTRREF(menu.back),
		.arg=NULL,
		.handler=&menu_return_handler
	},{
		.text=BUNDLESTRREF(menu.conf),
		.arg=&ui_menu_conf,
		.handler=&menu_list_handler
	}, {
		.text=BUNDLESTRREF(menu.quick),
		.arg=&ui_menu_quick,
		.handler=&menu_list_handler
	}, {
		.text=BUNDLESTRREF(menu.cal),
		.arg=&ui_menu_cal,
		.handler=&menu_list_handler
	}, {
		.text=BUNDLESTRREF(menu.wifi),
		.arg=&ui_menu_wifi,
		.handler=&menu_list_handler
	}, {
		.text=BUNDLESTRREF(menu.stat),
		.arg=&ui_menu_stat,
		.handler=&menu_list_handler
	},{
		.text=BUNDLESTRREF(menu.option),
		.arg=&ui_menu_option,
		.handler=&menu_list_handler
	},
};

menu_list_t ui_menu_top = {
	.items=menu_top_items,
	.num=sizeof(menu_top_items)/sizeof(menu_top_items[0]),
};

menu_option_t ui_menu_pick_i = {
	.cols=4,
	.init=&ui_menu_option_init_quick,
	.init_arg=(void*)1,
	.handler=&ui_menu_option_set_quick,
	.arg=(void*)1,
};

menu_option_t ui_menu_pick_v = {
	.cols=4,
	.init=&ui_menu_option_init_quick,
	.init_arg=(void*)2,
	.handler=&ui_menu_option_set_quick,
	.arg=(void*)2,
};


