/**
 * Know nothing about copy from left to right,
 * if someone wanna use it, just copy it from left to right
 *                by author m24h
 */
#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * init output sub-system
 */
esp_err_t output_init();

/**
 * output I/V
 */
void output_out ();

#ifdef __cplusplus
}
#endif

