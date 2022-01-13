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
 * return a*b
 */
int fxp_mul (int fix, int a, int b);

/**
 * return a/b
 */
int fxp_div (int fix, int a, int b);

/**
 * return (a*b+c)/d
 */
int fxp_mul_add_div(int fix, int a, int b, int c, int d);

/**
 * return log2(a)
 */
int fxp_log2(int fix, int a);


#ifdef __cplusplus
}
#endif


