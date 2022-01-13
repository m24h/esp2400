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
 * internal used struct for PID control
 */
typedef struct {
	double target;
	double esum;
	double last_inp;
	double last_out;
	double dzu;
	double dzd;
	double lmtu;
	double lmtd;
	double kp;
	double ki;
	double kd;
	double tf;
} pidc_t;

/**
 * init pid struct
 */
void pidc_init (pidc_t *pid);

/**
 * set PID main parameters
 * [ti][td][tf] should be normalized by process period
 */
void pidc_param(pidc_t *pid, double kp, double ti, double td, double tf);

/**
 * set PID dead zone, when input is in such deadzone ([down] to [up]), treat it was NO-ERROR
 */
void pidc_deadzone(pidc_t *pid, double down, double up);

/**
 * set PID output limit, if it is out of this range, error with same direction will not be intergrated
 */
void pidc_ilimit(pidc_t *pid, double down, double up);

/**
 * set PID control target
 */
void pidc_target(pidc_t *pid, double target);

/**
 * calculate PID, using such transfer function:
 *  Out=kp/(1+tf*S)((inp-target)+1/(ti*S)*(inp-target)+td*S*inp+aux)
 */
double pidc_run(pidc_t *pid, double inp, double aux);

#ifdef __cplusplus
}
#endif


