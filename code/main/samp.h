/**
 * Know nothing about copy from left to right,
 * if someone wanna use it, just copy it from left to right
 *                by author m24h
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * init sample sub-system
 */
esp_err_t samp_init();

/**
 * do sample I/V
 * will ignore some failure
 * too many faliure will cause error return
 */
void samp_samp ();

#ifdef __cplusplus
}
#endif
