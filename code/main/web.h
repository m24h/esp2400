/**
 * Know nothing about copy from left to right,
 * if someone wanna use it, just copy it from left to right
 *                by author m24h
 */
#pragma once

#include "esp_http_server.h"

#ifdef __cplusplus
extern "C" {
#endif

extern httpd_handle_t web_server;

/**
 * init web sub-system
 * this should be called after netif environment ready, or after wifi_init()
 */
esp_err_t web_init();

#ifdef __cplusplus
}
#endif
