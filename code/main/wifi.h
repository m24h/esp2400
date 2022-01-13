/**
 * Know nothing about copy from left to right,
 * if someone wanna use it, just copy it from left to right
 *                by author m24h
 */
#pragma once

#include "def.h"

#ifdef __cplusplus
extern "C" {
#endif

extern int wifi_state_sta; /* 0: not start, 1:started, 2:connecting 3:connected 4:ip got */
extern int wifi_state_ap;  /* 0: not start, 4:working */

extern esp_netif_t * wifi_intf_sta;
extern esp_netif_t * wifi_intf_ap;

/**
 * init WIFI sub-system
 */
esp_err_t wifi_init();

/**
 * let WIFI work as configuratiob
 */
esp_err_t wifi_reset();
#ifdef __cplusplus
}
#endif
