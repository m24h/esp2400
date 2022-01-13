/**
 * Know nothing about copy from left to right,
 * if someone wanna use it, just copy it from left to right
 *                by author m24h
 */
#include <math.h>
#include "pidc.h"

void pidc_init (pidc_t *pid)
{
	pid->esum=0;
	pid->last_inp=NAN;
	pid->last_out=NAN;
}

void pidc_param(pidc_t *pid, double kp, double ti, double td, double tf)
{
	pid->kp=kp;
	pid->ki=ti>0?kp/ti:0;
	pid->kd=td>0?kp*td:0;
	pid->tf=tf;
}

void pidc_deadzone(pidc_t *pid, double down, double up)
{
	pid->dzu=up;
	pid->dzd=down;
}

void pidc_ilimit(pidc_t *pid, double down, double up)
{
	pid->lmtu=up;
	pid->lmtd=down;
}

void pidc_target(pidc_t *pid, double target)
{
	pid->target=target;
}

double pidc_run(pidc_t *pid, double inp, double aux)
{
	double e=inp-pid->target;
	if (e>pid->dzu) e-=pid->dzu;
	else if (e<pid->dzd) e-=pid->dzd;
	else e=0;

	aux+=e*pid->kp;
	aux+=(pid->esum+e)*pid->ki;
	if (!isnan(pid->last_inp))	aux+=(inp-pid->last_inp)*pid->kd;
	pid->last_inp=inp;

	if (pid->kp>=0) {
		if (e>=0) {
			if (aux<pid->lmtu) pid->esum+=e;
		} else {
			if (aux>pid->lmtd) pid->esum+=e;
		}
	} else {
		if (e>=0) {
			if (aux>pid->lmtd) pid->esum+=e;
		} else {
			if (aux<pid->lmtu) pid->esum+=e;
		}
	}

	if (!isnan(pid->last_out)) {
		aux=(aux+pid->tf*pid->last_out)/(1+pid->tf);
	}
	pid->last_out=aux;

	return aux;
}
