/**
 * Know nothing about copy from left to right,
 * if someone wanna use it, just copy it from left to right
 *                by author m24h
 */
#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_event.h"
#include "esp_check.h"
#include "esp_err.h"
#include "esp_log.h"

#include "def.h"
#include "conf.h"
#include "bundle.h"

#ifdef __cplusplus
extern "C" {
#endif

/* event loop */
extern esp_event_loop_handle_t  app_eloop;

ESP_EVENT_DECLARE_BASE(APP_E_BASE);

typedef enum {
	APP_E_READY=0,
	APP_E_NET_OK,
	APP_E_WEB_OK,
} app_event_e;

/* event group, for some notify or resources mutex */
extern EventGroupHandle_t       app_egroup;

#define APP_EG_READY            0x0001   /* ready status, shoud not be clear by sub-system */
#define APP_EG_CONSOLE          0x0002   /* used for console mutex */
#define APP_EG_CONF_W  		    0x0004   /* used for config r/w lock */
#define APP_EG_CONF_R  		    0x0008   /* used for config r/w lock */
#define APP_EG_STAT_W  		    0x0010   /* used for config r/w lock */
#define APP_EG_STAT_R  		    0x0020   /* used for config r/w lock */
#define APP_EG_NET_OK           0x0040   /* primary netif worked */
#define APP_EG_WEB_OK           0x0080   /* primary web server worked */
#define APP_EG_PING             0x0100   /* set when pinging completes */

/**
 * reset system
 */
esp_err_t app_reset();

/**
 * format storage
 */
esp_err_t app_format();


#ifdef __cplusplus
}
#endif
