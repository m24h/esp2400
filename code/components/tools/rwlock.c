/**
 * Know nothing about copy from left to right,
 * if someone wanna use it, just copy it from left to right
 *                by author m24h
 */
#include <stdlib.h>
#include <string.h>

#include "rwlock.h"

esp_err_t rwlock_init(EventGroupHandle_t egroup, EventBits_t wbit, EventBits_t rbit, int * readers)
{
	*readers=0;
	xEventGroupClearBits(egroup, rbit);
	xEventGroupSetBits(egroup, wbit);

	return ESP_OK;
}

esp_err_t rwlock_lock_r(EventGroupHandle_t egroup, EventBits_t wbit, EventBits_t rbit, int * readers, TickType_t xTicksToWait)
{
	EventBits_t b=wbit | rbit;
	EventBits_t retb=xEventGroupWaitBits(egroup, b, pdTRUE, pdFALSE, xTicksToWait);
	if (!(retb & b)) return ESP_ERR_TIMEOUT;
	(*readers)++;
	xEventGroupSetBits(egroup, rbit);
	return ESP_OK;
}

esp_err_t rwlock_unlock_r(EventGroupHandle_t egroup, EventBits_t wbit, EventBits_t rbit, int * readers, TickType_t xTicksToWait)
{
	EventBits_t retb=xEventGroupWaitBits(egroup, rbit, pdTRUE, pdFALSE, xTicksToWait);
	if (!(retb & rbit)) return ESP_ERR_TIMEOUT;
	(*readers)--;
	xEventGroupSetBits(egroup, (*readers>0)?rbit:wbit);
	return ESP_OK;
}

esp_err_t rwlock_lock_w(EventGroupHandle_t egroup, EventBits_t wbit, TickType_t xTicksToWait)
{
	EventBits_t retb=xEventGroupWaitBits(egroup, wbit, pdTRUE, pdFALSE, xTicksToWait);
	if (!(retb & wbit)) return ESP_ERR_TIMEOUT;
	return ESP_OK;
}

esp_err_t rwlock_unlock_w(EventGroupHandle_t egroup, EventBits_t wbit)
{
	xEventGroupSetBits(egroup, wbit);
	return ESP_OK;
}
