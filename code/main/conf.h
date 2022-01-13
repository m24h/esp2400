/**
 * Know nothing about copy from left to right,
 * if someone wanna use it, just copy it from left to right
 *                by author m24h
 */
#pragma once

#include "esp_netif.h"

#include "def.h"
#include "rwlock.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CONF_LOCK_R()    rwlock_lock_r(app_egroup, APP_EG_CONF_W, APP_EG_CONF_R, &conf_lock_readers, portMAX_DELAY)
#define CONF_UNLOCK_R()  rwlock_unlock_r(app_egroup, APP_EG_CONF_W, APP_EG_CONF_R, &conf_lock_readers, portMAX_DELAY)
#define CONF_LOCK_W()    rwlock_lock_w(app_egroup, APP_EG_CONF_W, portMAX_DELAY)
#define CONF_UNLOCK_W()  rwlock_unlock_w(app_egroup, APP_EG_CONF_W)

extern const char * const conf_opt_sta_auth [5];
extern const char * const conf_opt_ap_auth [4];

typedef struct {
	int     num;
	struct {
		int32_t x;
		int32_t y;
	} p [CAL_POINTS];
} conf_cal_t;

typedef struct {
	int             on;       /* on or off */
	int             auth;     /* index of conf_opt_sta_auth/conf_opt_ap_auth */
	char            ssid[32];
	char            pass[16];
	esp_ip4_addr_t  ip;
	esp_ip4_addr_t  mask;
	esp_ip4_addr_t  gw;
} conf_wifi_t;

/** settings */
typedef struct {
	int      version;
	int      bundle;

	char     name[16];
	char     pass[16];
	char     admpass[16];

	struct {
		conf_cal_t isamp;
		conf_cal_t vsamp;
		conf_cal_t iout;
		conf_cal_t vout;
	} cal;

	struct {
		int   i [QUICK_POINTS];
		int   v [QUICK_POINTS];
	} quick;

	conf_wifi_t sta;
	conf_wifi_t ap;

	int fan_temp;
	int v_rise;
	int v_fall;
	int i_rise;
	int i_fall;
} conf_t;

extern conf_t conf_vars;

extern int conf_lock_readers;

/**
 * init configuration, try to load from file, or use default
 */
esp_err_t conf_init();

/**
 * use default configuration
 */
esp_err_t conf_default();

/**
 * load configuration from file
 */
esp_err_t conf_load();

/**
 * save configuration to file
 */
esp_err_t conf_save();

/**
 * delete calibration points from config
 * [cal_samp] and [cal_out] should be in conf_vars.cal
 * return ESP_ERR_INVALID_SIZE if less than 2 points remain when deleting is really done
 */
esp_err_t conf_cal_del(conf_cal_t * cal_samp, conf_cal_t * cal_out, int32_t value);

/**
 * add/set calibration points to config
 * [cal_samp] and [cal_out] should be in conf_vars.cal
 * return ESP_ERR_NO_MEM if there's no romm for new point
 * return ESP_ERR_INVALID_STATE if monotone increasing is broken when updating is really done
 */
esp_err_t conf_cal_set(conf_cal_t * cal_samp, conf_cal_t * cal_out, int32_t value, int32_t samp, int32_t out);

/**
 * calculate multi-point interpolation
 */
int32_t conf_cal(const conf_cal_t * cal, int32_t x);

#ifdef __cplusplus
}
#endif
