/**
 * Know nothing about copy from left to right,
 * if someone wanna use it, just copy it from left to right
 *                by author m24h
 */
#pragma once

#include "def.h"
#include "rwlock.h"

#ifdef __cplusplus
extern "C" {
#endif

#define STAT_LOCK_R()       rwlock_lock_r(app_egroup, APP_EG_STAT_W, APP_EG_STAT_R, &stat_lock_readers, portMAX_DELAY)
#define STAT_UNLOCK_R() 	rwlock_unlock_r(app_egroup, APP_EG_STAT_W, APP_EG_STAT_R, &stat_lock_readers, portMAX_DELAY)

/**
 * stat value
 */
typedef struct {
	int avg;
	int min;
	int max;
} stat_value_t;

/**
 * [i] Current (in mA)
 * [v] Voltage (in mV)
 * [p] Power (in mW)
 */
typedef struct {
	stat_value_t i;
	stat_value_t v;
	stat_value_t p;
} stat_item_t;

/**
 * this is a ring data buffer
 * [.._off] is start offset
 * [.._num] is usable number of points
 * items out of [.._num] specified scope is not initialized or currently writing
 * the order of data is as increment of time
 * [last_avg_cnt] is used internally, the count number of currently unfinished recording
 */
typedef struct {
	stat_item_t  items[STAT_SIZE];
	int          offset;
	int          num;
	int          last_avg_cnt;
	int64_t      last_enclose;
} stat_ring_t;

/**
 * stat data of seconds/minutes/hours
 */
typedef struct {
	stat_ring_t s;
	stat_ring_t m;
	stat_ring_t h;
	stat_item_t t; /* totally statistics */
} stat_data_t;

/** statistics data's last sampling time */
extern stat_data_t stat_data;

/** rwlock parameter for stat data */
extern int stat_lock_readers;

/**
 * init statistics sub-system
 */
esp_err_t stat_init();

/**
 * gather statistics
 */
void stat_stat ();

/**
 * reset totally data for starting statistics again
 */
esp_err_t stat_reset_ts ();


#ifdef __cplusplus
}
#endif
