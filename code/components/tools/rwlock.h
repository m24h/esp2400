/**
 * Know nothing about copy from left to right,
 * if someone wanna use it, just copy it from left to right
 *                by author m24h
 */
#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * init read/write lock, use two eventgroup bits [wbit] and [rbit], and an int number *[readers]
 * after function called, *[readers] will be set
 */
esp_err_t rwlock_init(EventGroupHandle_t egroup, EventBits_t wbit, EventBits_t rbit, int * readers);

/**
 * wait and lock for read, multi readers can enter
 * return ESP_ERR_TIMEOUT if waited ticks exceed [xTicksToWait]
 * if timeout, nothing changed
 */
esp_err_t rwlock_lock_r(EventGroupHandle_t egroup, EventBits_t wbit, EventBits_t rbit, int * readers, TickType_t xTicksToWait);

/**
 * unlock after reading ends
 * return ESP_ERR_TIMEOUT if waited ticks exceed [xTicksToWait]
 * if timeout, nothing changed, it's still locked, need to unlock again
 * so portMAX_DELAY for [xTicksToWait] is suggested, or treat that timeout as fatal
 */
esp_err_t rwlock_unlock_r(EventGroupHandle_t egroup, EventBits_t wbit, EventBits_t rbit, int * readers, TickType_t xTicksToWait);

/**
 * lock for writing, if ESP_OK returned, no writer or reader can enter before rwlock_unlock_w() called
 * return ESP_ERR_TIMEOUT if waited ticks exceed [xTicksToWait]
 * if timeout, nothing changed
 */
esp_err_t rwlock_lock_w(EventGroupHandle_t egroup, EventBits_t wbit, TickType_t xTicksToWait);

/**
 * unlock for read/write again
 */
esp_err_t rwlock_unlock_w(EventGroupHandle_t egroup, EventBits_t wbit);

#ifdef __cplusplus
}
#endif
