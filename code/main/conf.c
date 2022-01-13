/**
 * Know nothing about copy from left to right,
 * if someone wanna use it, just copy it from left to right
 *                by author m24h
 */
#include "esp_console.h"

#include "app.h"
#include "repl.h"
#include "main.h"
#include "conf.h"

const static char * const TAG="CONF";

conf_t conf_vars;
int conf_lock_readers=0;

const char * const conf_opt_sta_auth [5]={"open", "wep", "wpa_psk", "wpa2_psk", "wpa_1_2_psk"};
const char * const conf_opt_ap_auth  [4]={"open", "wpa_psk", "wpa2_psk", "wpa_1_2_psk"};

static esp_err_t cmd_config(int argc, char **argv)
{
	esp_err_t ret;
	if (argc>1) {
		if (!strcmp(argv[1], "load")) {
			if ((ret=conf_load())!=ESP_OK)
				return ret;

			repl_printf("Configuration is loaded from flash\r\n");
			return ret;
		}

		if (!strcmp(argv[1], "save")) {
			if ((ret=conf_save())!=ESP_OK)
				return ret;

			repl_printf("Configuration is saved to flash\r\n");
			return ret;
		}

		if (!strcmp(argv[1], "default")) {
			if ((ret=conf_default())!=ESP_OK)
				return ret;

			repl_printf("Configuration is restored to default, but not saved\r\n");
			return ret;
		}
	}

	int i;

	repl_printf("Machine Name : %s\r\n", conf_vars.name);
	repl_printf("Password : %s\r\n", conf_vars.pass[0]?"**":"<not set>");
	repl_printf("Admin password : %s\r\n", conf_vars.admpass[0]?"***":"<not set>");

	repl_printf("WIFI Station : %s, ssid=%s, auth=%s, password=%s, ip=" IPSTR ", netmask=" IPSTR ", gateway=" IPSTR "\r\n"
			, conf_vars.sta.on?"on":"off", conf_vars.sta.ssid
			, conf_opt_sta_auth[conf_vars.sta.auth<0?0:(conf_vars.sta.auth>4?4:conf_vars.sta.auth)], conf_vars.sta.pass
			, IP2STR(&conf_vars.sta.ip), IP2STR(&conf_vars.sta.mask), IP2STR(&conf_vars.sta.gw));

	repl_printf("WIFI AP : %s, ssid=%s, auth=%s, password=%s, ip=" IPSTR ", netmask=" IPSTR "\r\n"
			, conf_vars.ap.on?"on":"off", conf_vars.ap.ssid
			, conf_opt_ap_auth[conf_vars.ap.auth<0?0:(conf_vars.ap.auth>3?3:conf_vars.ap.auth)], conf_vars.ap.pass
			, IP2STR(&conf_vars.ap.ip), IP2STR(&conf_vars.ap.mask));

	repl_printf("Quick set voltages (mV) :");
	for (i=0;i<QUICK_POINTS;) {
		repl_printf(" %5d", conf_vars.quick.v[i]);
		i++;
		if (i<QUICK_POINTS && i%8==0) repl_printf("\r\n                         ");
	}
	repl_printf("\r\n");

	repl_printf("Quick set currents (mA) :");
	for (i=0;i<QUICK_POINTS;) {
		repl_printf(" %5d", conf_vars.quick.i[i]);
		i++;
		if (i<QUICK_POINTS && i%8==0) repl_printf("\r\n                         ");
	}
	repl_printf("\r\n");

	repl_printf("Voltage calibrate points (mV) :");
	for (i=0;i<conf_vars.cal.vout.num;) {
		repl_printf(" %5d", conf_vars.cal.vout.p[i].x);
		i++;
		if (i<QUICK_POINTS && i%8==0) repl_printf("\r\n                               ");
	}
	repl_printf("\r\n");

	repl_printf("Current calibrate points (mA) :");
	for (i=0;i<conf_vars.cal.iout.num;) {
		repl_printf(" %5d", conf_vars.cal.iout.p[i].x);
		i++;
		if (i<QUICK_POINTS && i%8==0) repl_printf("\r\n                               ");
	}
	repl_printf("\r\n");

	return ESP_OK;
}

static esp_err_t cmd_name(int argc, char **argv)
{
	if (argc>1) {
		strlcpy(conf_vars.name, argv[1], sizeof(conf_vars.name)-1);
		repl_printf("Changes made and will be effective after saving configuration and reset\r\n");
	}

	repl_printf("Machine name : \"%s\"\r\n", conf_vars.name);
	return ESP_OK;
}

static esp_err_t cmd_pass(int argc, char **argv)
{
	if (argc>1) {
		strlcpy(conf_vars.pass, argv[1], sizeof(conf_vars.pass)-1);
		repl_printf("Changes made. Don't forget to save configuration\r\n");
	}

	repl_printf("Password : \"%s\"\r\n", conf_vars.pass[0]?"***":"<not set>");
	return ESP_OK;
}

static esp_err_t cmd_admpass(int argc, char **argv)
{
	if (argc>1) {
		CONF_LOCK_W();
		strlcpy(conf_vars.admpass, argv[1], sizeof(conf_vars.admpass)-1);
		CONF_UNLOCK_W();
		repl_printf("Changes made. Don't forget to save configuration\r\n");
	}

	repl_printf("Admin Password : \"%s\"\r\n", conf_vars.pass[0]?"***":"<not set>");
	return ESP_OK;
}

static esp_err_t cmd_quickv(int argc, char **argv)
{
	if (argc>2) {
		int pos, toset;
		if (sscanf(argv[1],"%d", &pos)!=1 || sscanf(argv[2],"%d", &toset)!=1
			|| pos<0 || pos>=QUICK_POINTS || toset<V_MIN || toset>V_MAX) {
			repl_printf("Bad position (should between %d-%d) or bad value (shoud between %d-%d)\r\n"
					, 0, QUICK_POINTS-1, V_MIN, V_MAX);
		} else {
			conf_vars.quick.v[pos]=toset;
			repl_printf("Quick-pick voltage %d mV has been set at position %d\r\n"
					    "Don't forget to save configuration\r\n", toset, pos);

		}
	} else if (argc>1) {
		int pos;
		if (sscanf(argv[1],"%d", &pos)!=1 || pos<0 || pos>=QUICK_POINTS) {
			repl_printf("Bad position (should between %d-%d)\r\n", 0, QUICK_POINTS-1);
		} else {
			repl_printf("Quick-pick voltage in position %d is %d mV\r\n", pos, conf_vars.quick.v[pos]);
		}
	} else {
		int i;
		repl_printf("Quick-pick Voltages:\r\n");
		for (i=0; i<QUICK_POINTS; i++)
			repl_printf("#%d\t%d mV\r\n",i,conf_vars.quick.v[i]);
	}

	return ESP_OK;
}

static esp_err_t cmd_quicki(int argc, char **argv)
{
	if (argc>2) {
		int pos, toset;
		if (sscanf(argv[1],"%d", &pos)!=1 || sscanf(argv[2],"%d", &toset)!=1
			|| pos<0 || pos>=QUICK_POINTS || toset<I_MIN || toset>I_MAX) {
			repl_printf("Bad position (should between %d-%d) or bad value (shoud between %d-%d)\r\n"
					, 0, QUICK_POINTS-1, I_MIN, I_MAX);
		} else {
			conf_vars.quick.i[pos]=toset;
			repl_printf("Quick-pick current %d mA has been set at position %d\r\n"
					    "Don't forget to save configuration\r\n", toset, pos);
		}
	} else if (argc>1) {
		int pos;
		if (sscanf(argv[1],"%d", &pos)!=1 || pos<0 || pos>=QUICK_POINTS) {
			repl_printf("Bad position (should between %d-%d)\r\n", 0, QUICK_POINTS-1);
		} else {
			repl_printf("Quick-pick current in position %d is %d mA\r\n", pos, conf_vars.quick.i[pos]);
		}
	} else {
		int i;
		repl_printf("Quick-pick Currents:\r\n");
		for (i=0; i<QUICK_POINTS; i++)
			repl_printf("#%d\t%d mA\r\n",i,conf_vars.quick.i[i]);
	}

	return ESP_OK;
}

static int cal_del_x(conf_cal_t *cal, int32_t value)
{
	int i, j;
	if (cal->num>CAL_POINTS) cal->num=CAL_POINTS;
	for (i=0, j=0;i<cal->num;i++) {
		if (cal->p[i].x!=value) {
			if (i>j) {
				cal->p[j].x=cal->p[i].x;
				cal->p[j].y=cal->p[i].y;
			}
			j++;
		}
	}
	cal->num=j;
	for (;j<CAL_POINTS;j++) cal->p[j].x=cal->p[j].y=0;
	return (cal->num);
}

static int cal_del_y(conf_cal_t *cal, int32_t value)
{
	int i, j;
	if (cal->num>CAL_POINTS) cal->num=CAL_POINTS;
	for (i=0, j=0;i<cal->num;i++) {
		if (cal->p[i].y!=value) {
			if (i>j) {
				cal->p[j].x=cal->p[i].x;
				cal->p[j].y=cal->p[i].y;
			}
			j++;
		}
	}
	cal->num=j;
	for (;j<CAL_POINTS;j++) cal->p[j].x=cal->p[j].y=0;
	return (cal->num);
}

/**
 * add/set/update calibration points
 * return number calibration points after set
 * or -1 if there's no room for it
 * or -2 if monotonously increasing was wrecked
 */
static int cal_set(conf_cal_t *cal, int32_t x, int32_t y)
{
	int i;
	for (i=0; i<cal->num; i++)
		if (cal->p[i].x>=x || cal->p[i].y>=y) break;

	if (cal->p[i].x==x || cal->p[i].y==y) {
		cal->p[i].x=x;
		cal->p[i].y=y;
	} else if (cal->num>=CAL_POINTS)
		return -1; /* no room for new point*/
	else {
		int j;
		for (j=cal->num; j>i; j--) {
			cal->p[j].x=cal->p[j-1].x;
			cal->p[j].y=cal->p[j-1].y;
		}
		cal->p[i].x=x;
		cal->p[i].y=y;
		cal->num++;
	}

	/* check monotonously increasing */
	for (i=1; i<cal->num; i++)
		if (cal->p[i].x<cal->p[i-1].x || cal->p[i].y<cal->p[i-1].y)
			return -2;

	for (i=cal->num;i<CAL_POINTS;i++) cal->p[i].x=cal->p[i].y=0;
	return cal->num;
}

esp_err_t conf_cal_del(conf_cal_t * cal_samp, conf_cal_t * cal_out, int32_t value)
{
	conf_cal_t samp, out;
	CONF_LOCK_R();
	memcpy(&samp, cal_samp, sizeof(conf_cal_t));
	memcpy(&out, cal_out, sizeof(conf_cal_t));
	CONF_UNLOCK_R();

	if (cal_del_y(&samp, value)<2 || cal_del_x(&out, value)<2)
		return ESP_ERR_INVALID_SIZE;

	CONF_LOCK_W();
	memcpy(cal_samp, &samp, sizeof(conf_cal_t));
	memcpy(cal_out, &out, sizeof(conf_cal_t));
	CONF_UNLOCK_W();

	return ESP_OK;
}

esp_err_t conf_cal_set(conf_cal_t * cal_samp, conf_cal_t * cal_out, int32_t value, int32_t samp, int32_t out)
{
	conf_cal_t t_samp, t_out;
	CONF_LOCK_R();
	memcpy(&t_samp, cal_samp, sizeof(conf_cal_t));
	memcpy(&t_out, cal_out, sizeof(conf_cal_t));
	CONF_UNLOCK_R();

	int r=cal_set(&t_samp, samp, value);
	if (r==-1) return ESP_ERR_NO_MEM;
	else if (r==-2) return ESP_ERR_INVALID_STATE;

	r=cal_set(&t_out, value, out);
	if (r==-1) return ESP_ERR_NO_MEM;
	else if (r==-2) return ESP_ERR_INVALID_STATE;

	CONF_LOCK_W();
	memcpy(cal_samp, &t_samp, sizeof(conf_cal_t));
	memcpy(cal_out, &t_out, sizeof(conf_cal_t));
	CONF_UNLOCK_W();

	return ESP_OK;
}

static esp_err_t cmd_calv(int argc, char **argv)
{
	esp_err_t ret;
	if (argc>2) {
		if (!strcmp(argv[1], "del")) {
			int32_t value;
			if (sscanf(argv[2], "%d", &value)==1) {
				if ((ret=conf_cal_del(&conf_vars.cal.vsamp, &conf_vars.cal.vout, value))==ESP_ERR_INVALID_SIZE) {
					repl_printf("Failed, calibration need at least 2 points\r\n");
					return ESP_OK;
				} else if (ret!=ESP_OK)
					return ret;

				repl_printf("Changes made. Don't forget to save configuration\r\n");
			}
		} else if (!strcmp(argv[1], "set")) {
			int32_t value;
			if (sscanf(argv[2], "%d", &value)==1) {
				if ((ret=conf_cal_set(&conf_vars.cal.vsamp, &conf_vars.cal.vout, value, main_vars.raw_vc, main_vars.raw_vs))==ESP_ERR_NO_MEM) {
					repl_printf("Calibration points exceed size, delete some points before add new one\r\n");
					return ESP_OK;
				} else if (ret==ESP_ERR_INVALID_STATE) {
					repl_printf("Calibration points is not monotonously increasing, check please\r\n");
					return ESP_OK;
				} else if (ret!=ESP_OK)
					return ret;

				repl_printf("Changes made, don't forget to save configuraion\r\n");
			}
		}
	}

	int i;
	repl_printf("Sampling voltage calibrate points :\r\n");
	for (i=0;i<conf_vars.cal.vsamp.num;i++)
		repl_printf("#%d\t%5d mV : %d\r\n", i, conf_vars.cal.vsamp.p[i].y, conf_vars.cal.vsamp.p[i].x);
	repl_printf("Ouputing voltage calibrate points :\r\n");
	for (i=0;i<conf_vars.cal.vout.num;i++)
		repl_printf("#%d\t%5d mV : %d\r\n", i, conf_vars.cal.vout.p[i].x, conf_vars.cal.vout.p[i].y);

	return ESP_OK;
}

static esp_err_t cmd_cali(int argc, char **argv)
{
	esp_err_t ret;
	if (argc>2) {
		if (!strcmp(argv[1], "del")) {
			int32_t value;
			if (sscanf(argv[2], "%d", &value)==1) {
				if ((ret=conf_cal_del(&conf_vars.cal.isamp, &conf_vars.cal.iout, value))==ESP_ERR_INVALID_SIZE) {
					repl_printf("Failed, calibration need at least 2 points\r\n");
					return ESP_OK;
				} else if (ret!=ESP_OK)
					return ret;

				repl_printf("Changes made. Don't forget to save configuration\r\n");
			}
		} else if (!strcmp(argv[1], "set")) {
			int32_t value;
			if (sscanf(argv[2], "%d", &value)==1) {
				if ((ret=conf_cal_set(&conf_vars.cal.isamp, &conf_vars.cal.iout, value, main_vars.raw_ic, main_vars.raw_is))==ESP_ERR_NO_MEM) {
					repl_printf("Calibration points exceed size, delete some points before add new one\r\n");
					return ESP_OK;
				} else if (ret==ESP_ERR_INVALID_STATE) {
					repl_printf("Calibration points is not monotonously increasing, check please\r\n");
					return ESP_OK;
				} else if (ret!=ESP_OK)
					return ret;

				repl_printf("Changes made, don't forget to save configuraion\r\n");
			}
		}
	}

	int i;
	repl_printf("Sampling current calibrate points :\r\n");
	for (i=0;i<conf_vars.cal.isamp.num;i++)
		repl_printf("#%d\t%5d mA : %d\r\n", i, conf_vars.cal.isamp.p[i].y, conf_vars.cal.isamp.p[i].x);
	repl_printf("Ouputing current calibrate points :\r\n");
	for (i=0;i<conf_vars.cal.iout.num;i++)
		repl_printf("#%d\t%5d mA : %d\r\n", i, conf_vars.cal.iout.p[i].x, conf_vars.cal.iout.p[i].y);

	return ESP_OK;
}

static int findstr(const char * str, int def, const char * const * array, int size)
{
	int i;
	for (i=0;i<size;i++)
		if (!strcmp(str, array[i]))
			return i;
	return def;
}

static esp_err_t cmd_sta(int argc, char **argv)
{
	int i, changed=0;
	for (i=1; i<argc; i++) {
		if (!strcmp(argv[i], "on")) {
			changed=1;
			conf_vars.sta.on=1;
		} else if (!strcmp(argv[i], "off")) {
			changed=1;
			conf_vars.sta.on=0;
		} else if (!strcmp(argv[i], "auth")) {
			if (i+1<argc) {
				changed=1;
				conf_vars.sta.auth=findstr(argv[++i], 0, conf_opt_sta_auth, 5);
			}
		} else if (!strcmp(argv[i], "ssid")) {
			if (i+1<argc) {
				changed=1;
				snprintf(conf_vars.sta.ssid, sizeof(conf_vars.sta.ssid), "%s", argv[++i]);
			}
		} else if (!strcmp(argv[i], "pass")||!strcmp(argv[i], "password")) {
			if (i+1<argc) {
				changed=1;
				snprintf(conf_vars.sta.pass, sizeof(conf_vars.sta.pass), "%s", argv[++i]);
			}
		} else if (!strcmp(argv[i], "ip")) {
			if (i+1<argc) {
				changed=1;
				conf_vars.sta.ip.addr=esp_ip4addr_aton(argv[++i]);
			}
		} else if (!strcmp(argv[i], "mask")||!strcmp(argv[i], "netmask")||!strcmp(argv[i], "nm")) {
			if (i+1<argc) {
				changed=1;
				conf_vars.sta.mask.addr=esp_ip4addr_aton(argv[++i]);
			}
		} else if (!strcmp(argv[i], "gw")||!strcmp(argv[i], "gateway")) {
			if (i+1<argc) {
				changed=1;
				conf_vars.sta.gw.addr=esp_ip4addr_aton(argv[++i]);
			}
		}
	}

	if (changed) {
		repl_printf("Changes made, will be effective after saving configuration and reset\r\n");
	}

	repl_printf("WIFI Station : %s\r\n    ssid : \"%s\"\r\n    auth : %s\r\n    password : \"%s\"\r\n    ip : " IPSTR "\r\n    netmask : " IPSTR "\r\n    gateway : " IPSTR "\r\n"
			, conf_vars.sta.on?"on":"off", conf_vars.sta.ssid
			, conf_opt_sta_auth[conf_vars.sta.auth<0?0:(conf_vars.sta.auth>4?4:conf_vars.sta.auth)], conf_vars.sta.pass
			, IP2STR(&conf_vars.sta.ip), IP2STR(&conf_vars.sta.mask), IP2STR(&conf_vars.sta.gw));

	return ESP_OK;
}

static esp_err_t cmd_ap(int argc, char **argv)
{
	int i, changed=0;
	for (i=1; i<argc; i++) {
		if (!strcmp(argv[i], "on")) {
			changed=1;
			conf_vars.ap.on=1;
		} else if (!strcmp(argv[i], "off")) {
			changed=1;
			conf_vars.ap.on=0;
		} else if (!strcmp(argv[i], "auth")) {
			if (i+1<argc) {
				changed=1;
				conf_vars.ap.auth=findstr(argv[++i], 0, conf_opt_ap_auth, 4);
			}
		} else if (!strcmp(argv[i], "ssid")) {
			if (i+1<argc) {
				changed=1;
				snprintf(conf_vars.ap.ssid, sizeof(conf_vars.ap.ssid), "%s", argv[++i]);
			}
		} else if (!strcmp(argv[i], "pass")||!strcmp(argv[i], "password")) {
			if (i+1<argc) {
				changed=1;
				snprintf(conf_vars.ap.pass, sizeof(conf_vars.ap.pass), "%s", argv[++i]);
			}
		} else if (!strcmp(argv[i], "ip")) {
			if (i+1<argc) {
				changed=1;
				conf_vars.ap.ip.addr=esp_ip4addr_aton(argv[++i]);
			}
		} else if (!strcmp(argv[i], "mask")||!strcmp(argv[i], "netmask")||!strcmp(argv[i], "nm")) {
			if (i+1<argc) {
				changed=1;
				conf_vars.ap.mask.addr=esp_ip4addr_aton(argv[++i]);
			}
		} else if (!strcmp(argv[i], "gw")||!strcmp(argv[i], "gateway")) {
			if (i+1<argc) {
				changed=1;
				conf_vars.ap.gw.addr=esp_ip4addr_aton(argv[++i]);
			}
		}
	}

	if (changed)
		repl_printf("Changes made, will be effective after saving configuration and reset\r\n");

	repl_printf("WIFI AP : %s\r\n    ssid : \"%s\"\r\n    auth : %s\r\n    password : \"%s\"\r\n    ip : " IPSTR "\r\n    netmask : " IPSTR "\r\n"
			, conf_vars.ap.on?"on":"off", conf_vars.ap.ssid
			, conf_opt_ap_auth[conf_vars.ap.auth<0?0:(conf_vars.ap.auth>3?3:conf_vars.ap.auth)], conf_vars.ap.pass
			, IP2STR(&conf_vars.ap.ip), IP2STR(&conf_vars.ap.mask));

	return ESP_OK;
}

static esp_err_t cmd_fan(int argc, char **argv)
{
	int v;
	if (argc>1 && sscanf(argv[1], "%d", &v)==1)
		conf_vars.fan_temp=v;

	repl_printf("Target temperature : %d celsius degree\r\n", conf_vars.fan_temp);

	return ESP_OK;
}

static esp_err_t cmd_risev(int argc, char **argv)
{
	int v;
	if (argc>1 && sscanf(argv[1], "%d", &v)==1 && v>=0)
		conf_vars.v_rise=v;

	if (conf_vars.v_rise==0)
		repl_printf("Voltage rising limit : NO LIMIT\r\n");
	else
		repl_printf("Voltage rising limit : %d mV/s\r\n", conf_vars.v_rise);

	return ESP_OK;
}

static esp_err_t cmd_fallv(int argc, char **argv)
{
	int v;
	if (argc>1 && sscanf(argv[1], "%d", &v)==1 && v>=0)
		conf_vars.v_fall=v;

	if (conf_vars.v_fall==0)
		repl_printf("Voltage falling limit : NO LIMIT\r\n");
	else
		repl_printf("Voltage falling limit : %d mV/s\r\n", conf_vars.v_fall);

	return ESP_OK;
}

static esp_err_t cmd_risei(int argc, char **argv)
{
	int v;
	if (argc>1 && sscanf(argv[1], "%d", &v)==1 && v>=0)
		conf_vars.i_rise=v;

	if (conf_vars.i_rise==0)
		repl_printf("Current rising limit : NO LIMIT\r\n");
	else
		repl_printf("Current rising limit : %d mA/s\r\n", conf_vars.i_rise);

	return ESP_OK;
}

static esp_err_t cmd_falli(int argc, char **argv)
{
	int v;
	if (argc>1 && sscanf(argv[1], "%d", &v)==1 && v>=0)
		conf_vars.i_fall=v;

	if (conf_vars.i_fall==0)
		repl_printf("Current falling limit : NO LIMIT\r\n");
	else
		repl_printf("Current falling limit : %d mA/s\r\n", conf_vars.i_fall);

	return ESP_OK;
}

static esp_err_t register_repl_cmd()
{
	static const esp_console_cmd_t cmd [] = {
		{
			.command = "config",
			.help = "Configuation operations",
			.hint = "[(empty means show)/load/save/default(restore to factory default)]",
			.func = &cmd_config,
		}, {
			.command = "name",
			.help = "Show (without parameters) or set machine name",
			.hint = "[new machine name if specified, <32 chars]",
			.func = &cmd_name,
		}, {
			.command = "pass",
			.help = "Show existence (without parameters) or set password",
			.hint = "[new password if specified, \"\" means empty, <32 chars]",
			.func = &cmd_pass,
		}, {
			.command = "admpass",
			.help = "Show existence (without parameters) or set admin password",
			.hint = "[new password if specified, \"\" means empty, <32 chars]",
			.func = &cmd_admpass,
		}, {
			.command = "sta",
			.help = "Show (without parameters) or set WIFI station parameters",
			.hint = "[on/off] [auth open/wep/wpa_psk/wpa2_psk/wpa_1_2_psk] [ssid ...] [pass ...] [ip ...] [mask ...] [gw ...]\r\n",
			.func = &cmd_sta,
		}, {
			.command = "ap",
			.help = "Show (without parameters) or set WIFI AP parameters",
			.hint = "[on/off] [auth open/wep/wpa_psk/wpa2_psk/wpa_1_2_psk] [ssid ...] [pass ...] [ip ...] [mask ...]",
			.func = &cmd_ap,
		}, {
			.command = "quickv",
			.help = "show (without parameters) or set quick-pick voltage",
			.hint = "[position to set] [voltage to set (mV)]",
			.func = &cmd_quickv,
		}, {
			.command = "quicki",
			.help = "show (without parameters) or set quick-pick current",
			.hint = "[position to set] [current to set (mA)]",
			.func = &cmd_quicki,
		}, {
			.command = "calv",
			.help = "Show (without parameters) or delete/set calibration of voltage",
			.hint = "[del/set] [voltage to-del/measured (mV)]",
			.func = &cmd_calv,
		}, {
			.command = "cali",
			.help = "Show (without parameters) or delete/set calibration of current",
			.hint = "[del/set] [current to-del/measured (mA)]",
			.func = &cmd_cali,
		}, {
			.command = "fan",
			.help = "Show (without parameters) or set fan-controled temperature",
			.hint = "[target temperature of fan-controling]",
			.func = &cmd_fan,
		}, {
			.command = "risev",
			.help = "Show (without parameters) or set voltage rising limit (mV/s)",
			.hint = "[rising limit (mV/s), 0 means no limit]",
			.func = &cmd_risev,
		}, {
			.command = "fallv",
			.help = "Show (without parameters) or set voltage falling limit (mV/s)",
			.hint = "[falling limit (mV/s), 0 means no limit]",
			.func = &cmd_fallv,
		}, {
			.command = "risei",
			.help = "Show (without parameters) or set current rising limit (mA/s)",
			.hint = "[rising limit (mA/s), 0 means no limit]",
			.func = &cmd_risei,
		}, {
			.command = "falli",
			.help = "Show (without parameters) or set current falling limit (mA/s)",
			.hint = "[falling limit (mA/s), 0 means no limit]",
			.func = &cmd_falli,
		},
	};

	int i;
	for (i=0; i<sizeof(cmd)/sizeof(esp_console_cmd_t); i++)
		esp_console_cmd_register(&cmd[i]);

	return ESP_OK;
}

esp_err_t conf_init()
{
	esp_err_t ret, ret2;

	ESP_RETURN_ON_ERROR(ret=rwlock_init(app_egroup, APP_EG_CONF_W, APP_EG_CONF_R, &conf_lock_readers)
			, TAG, "Failed to init config r/w lock (%d:%s)", ret, esp_err_to_name(ret));

	if ((ret2=conf_load())!=ESP_OK) {
		ESP_LOGW(TAG, "Failed to load configuration file, use default instead (%d:%s)", ret2, esp_err_to_name(ret2));

		ESP_RETURN_ON_ERROR(ret=conf_default()
			, TAG, "Failed to set default configuration (%d:%s)", ret, esp_err_to_name(ret));

		conf_save();
	}

	if (conf_vars.version!=VERSION) {
		ESP_LOGW(TAG, "Wrong version of onfiguration file, use default instead");

		ESP_RETURN_ON_ERROR(ret=conf_default()
			, TAG, "Failed to set default configuration (%d:%s)", ret, esp_err_to_name(ret));

		conf_save();
	}

	return register_repl_cmd();
}

esp_err_t conf_default()
{
	conf_t def={
		.version=VERSION,
		.name="zxd2400",
		.cal={
			.isamp={
				.num=2,
				.p={{.x=CAL_ISAMP_MIN, .y=I_MIN},
					{.x=CAL_ISAMP_MAX, .y=I_MAX}}
			},
			.vsamp={
				.num=2,
				.p={{.x=CAL_VSAMP_MIN, .y=V_MIN},
					{.x=CAL_VSAMP_MAX, .y=V_MAX}}
			},
			.iout={
				.num=2,
				.p={{.x=I_MIN, .y=CAL_IOUT_MIN},
					{.x=I_MAX, .y=CAL_IOUT_MAX}
				}
			},
			.vout={
				.num=2,
				.p={{.x=V_MIN, .y=CAL_VOUT_MIN},
					{.x=V_MAX, .y=CAL_VOUT_MAX}
				}
			}
		},
		.quick={
			.i={3000, 0, 0, 0, 1000,2000,3000,4000,5000,7500,10000,
				15000,20000,25000,30000,35000,40000,45000,50000},
			.v={0,0,0,0,1000,1200,1500,3000,3300,4000,4500,5000,
				6000,9000,12000,15000,18000,24000,36000,48000}
		},
		.sta={
			.auth=3, /* wpa2_psk */
		},
		.ap={
			.ssid="zxd2400",
			.auth=2, /* wpa2_psk */
			.pass="12345678",
			.ip={.addr=0x0109A8C0}, /* 192.168.9.1 */
			.mask={.addr=0x00FFFFFF}, /* 255.255.255.0 */
		},
		.fan_temp=40,
		.v_rise=V_MAX-V_MIN, /* by default limit a full rising in 1s */
		.v_fall=V_MAX-V_MIN, /* by default limit a full falling in 1s */
		.i_rise=I_MAX-I_MIN, /* by default limit a full rising in 1s */
		.i_fall=I_MAX-I_MIN, /* by default limit a full falling in 1s */
	};

	CONF_LOCK_W();
	memcpy(&conf_vars, &def, sizeof(conf_t));
	CONF_UNLOCK_W();

	return ESP_OK;
}

esp_err_t conf_load()
{
	/* allocte memory */
	conf_t *cf=(conf_t *)malloc(sizeof(conf_t));
	ESP_RETURN_ON_FALSE(cf
		, ESP_ERR_NO_MEM , TAG, "Failed to allocate memory to read configuration file");

	esp_err_t ret=ESP_OK;

	FILE *fin = fopen(FS_CONF_FILE, "rb");
	ESP_GOTO_ON_FALSE(fin
		, ESP_FAIL, err, TAG, "Failed to open configuration file for read (%d)", errno);

	ESP_GOTO_ON_FALSE(fread(cf, 1, sizeof(conf_t), fin)==sizeof(conf_t)
		,ESP_ERR_INVALID_SIZE, err, TAG, "Failed to read configuration file (%d)", errno);

	CONF_LOCK_W();
	memcpy(&conf_vars, cf, sizeof(conf_t));
	CONF_UNLOCK_W();

err:
	if (fin) fclose(fin);
	if (cf) free(cf);

	return ret;
}

esp_err_t conf_save()
{
	conf_t * cf=(conf_t *)malloc(sizeof(conf_t));
	ESP_RETURN_ON_FALSE(cf
		, ESP_ERR_NO_MEM , TAG, "Failed to allocate memory to save configuration file");

	esp_err_t ret=ESP_OK;

	FILE *fout = fopen(FS_CONF_FILE, "wb");
	ESP_GOTO_ON_FALSE(fout
		, ESP_FAIL, err, TAG, "Failed to open configuration file for writing (%d)", errno);

	CONF_LOCK_R();
	memcpy(cf, &conf_vars, sizeof(conf_t));
	CONF_UNLOCK_R();

	int wr=fwrite(cf, 1, sizeof(conf_t), fout);
	ESP_GOTO_ON_FALSE(wr==sizeof(conf_t)
		, ESP_ERR_INVALID_SIZE, err, TAG, "Failed to write configuration file (%d)", errno);

err:
	if (fout) fclose(fout);
	if (cf) free(cf);

	return ret;
}

int32_t conf_cal(const conf_cal_t * cal, int32_t x)
{
	if (cal->num<=0) return 0;
	if (cal->num<2)  return cal->p[0].y;

	int i;
	for (i=0; i<cal->num; i++)
		if (cal->p[i].x>=x) break;

	int64_t t;
	if (i==0) {
		if (cal->p[1].x==cal->p[0].x)
			t=cal->p[0].y;
		else
			t=cal->p[0].y-(((int64_t)cal->p[0].x-x)*(cal->p[1].y-cal->p[0].y)+((cal->p[1].x-cal->p[0].x)>>1))/(cal->p[1].x-cal->p[0].x);
	} else if (i<cal->num) {
		if (cal->p[i].x==cal->p[i-1].x)
			t=cal->p[i-1].y;
		else
			t=cal->p[i].y-(((int64_t)cal->p[i].x-x)*(cal->p[i].y-cal->p[i-1].y)+((cal->p[i].x-cal->p[i-1].x)>>1))/(cal->p[i].x-cal->p[i-1].x);
	} else {
		i=cal->num-1;
		if (cal->p[i].x==cal->p[i-1].x)
			t=cal->p[i].y;
		else
			t=cal->p[i].y+(((int64_t)x-cal->p[i].x)*(cal->p[i].y-cal->p[i-1].y)+((cal->p[i].x-cal->p[i-1].x)>>1))/(cal->p[i].x-cal->p[i-1].x);
	}

	if (t>2147483647) return 2147483647;
	if (t<-2147483648) return -2147483648;
	return (int)t;
}
