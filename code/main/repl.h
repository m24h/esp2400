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
 * init REPL sub-system
 */
esp_err_t repl_init();

/**
 * run a command line with output stream [output] specified
 */
void repl_run(const char * line, FILE * output);

/**
 * print output to REPL
 */
int repl_printf(const char * fmt, ...);

#ifdef __cplusplus
}
#endif
