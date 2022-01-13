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

/* key parameters */
typedef struct {
	int   on;        /* output on/off status, use I_OFF/V_OFF as output when on==0 */
	int   vs;        /* V setting, in mV */
	int   is;        /* I setting, in mA */
	int   vc;        /* V presently, in mV */
	int   ic;        /* I presently, in mA */
	int   temp;      /* present temperature, in 0.001 celsius degree */
	int   fan;       /* fan output setting, 0-255 */
	int   discharge; /* discharge output setting, 0-255 */

	int     p;       /* power currently, im mW */
	int64_t e;       /* integral energy, uJ as mw multiplied by ms */

	int32_t raw_vc;  /* currently sampling raw, used for calibration */
	int32_t raw_ic;  /* currently sampling raw, used for calibration */
	int32_t raw_vs;  /* currently output   raw, used for calibration */
	int32_t raw_is;  /* currently output   raw, used for calibration */

	char  msg [40];
} main_vars_t;

extern main_vars_t main_vars;

/**
 * set global text message
 */
esp_err_t main_message(const char *s);

/**
 * init main system
 */
esp_err_t main_init();

/**
 * main task for sampling/outputing
 */
void main_task();


#ifdef __cplusplus
}
#endif
