/**
 * Know nothing about copy from left to right,
 * if someone wanna use it, just copy it from left to right
 *                by author m24h
 */
#pragma once

#include <stddef.h>

#define BUNDLE_SIZE 2

/* current bundle */
#define BUNDLE          (bundle_data[conf_vars.bundle])
/* get reference of element from buddle */
#define BUNDLESTRREF(elem) ((const char *)&(((bundle_t*)0)->elem))
/* check a (const char *) pointer is a bundle reference or a real (const char*), return the right (const char*) */
#define BUNDLESTR(s) ((size_t)(s)<sizeof(bundle_t)?\
		         *(const char**)((uint8_t*)&(bundle_data[conf_vars.bundle])+(size_t)(s))\
				 :(const char*)(s))

typedef struct {
	const char * dumb; /* just to avoid valid bundle reference to be NULL */
	const struct {
		const char * ready;
	} msg;

	const struct {
		const char * back;
		const char * back2;
		const char * option;
		const char * option_lang;
		const char * option_fan_temp;
		const char * option_v_rise;
		const char * option_v_fall;
		const char * option_i_rise;
		const char * option_i_fall;
		const char * option_fade_nolimit;
		const char * conf;
		const char * conf_save;
		const char * conf_save_hint;
		const char * conf_load;
		const char * conf_load_hint;
		const char * conf_default;
		const char * conf_default_hint;
		const char * conf_format;
		const char * conf_format_hint;
		const char * conf_reset;
		const char * conf_reset_hint;
		const char * quick;
		const char * quick_v;
		const char * quick_i;
		const char * cal;
		const char * cal_err_noroom;
		const char * cal_err_2points;
		const char * cal_err_monoinc;
		const char * cal_addv;
		const char * cal_addv_hint;
		const char * cal_delv;
		const char * cal_delv_hint;
		const char * cal_addi;
		const char * cal_addi_hint;
		const char * cal_deli;
		const char * cal_deli_hint;
		const char * wifi;
		const char * wifi_status;
		const char * wifi_status_prompt;
		const char * wifi_status_err;
		const char * wifi_name;
		const char * wifi_sta;
		const char * wifi_sta_on;
		const char * wifi_sta_ssid;
		const char * wifi_sta_auth;
		const char * wifi_sta_pass;
		const char * wifi_sta_ip;
		const char * wifi_sta_mask;
		const char * wifi_sta_gw;
		const char * wifi_ap;
		const char * wifi_ap_on;
		const char * wifi_ap_ssid;
		const char * wifi_ap_auth;
		const char * wifi_ap_pass;
		const char * wifi_ap_ip;
		const char * wifi_ap_mask;
		const char * wifi_pass;
		const char * wifi_admpass;
		const char * wifi_reset;
		const char * wifi_reset_hint;
		const char * stat;
		const char * stat_v;
		const char * stat_i;
		const char * stat_p;
		const char * stat_e;
		const char * stat_t;
		const char * stat_s;
		const char * stat_m;
		const char * stat_h;
		const char * stat_cur;
		const char * stat_max;
		const char * stat_min;
		const char * stat_avg;
		const char * stat_exit;
		const char * stat_clickback;
		const char * stat_zero;
		const char * stat_zero_hint;
		const char * confirm_ok;
		const char * confirm_cancel;
		const char * onoff_on;
		const char * onoff_off;
	} menu;

} bundle_t;

extern const char * bundle_langs[BUNDLE_SIZE];
extern const bundle_t * const bundle_data;

