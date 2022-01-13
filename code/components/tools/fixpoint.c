/**
 * Know nothing about copy from left to right,
 * if someone wanna use it, just copy it from left to right
 *                by author m24h
 */
#include <stdlib.h>
#include <string.h>

#include "fixpoint.h"

/**
 * return a*b
 */
int fxp_mul (int fix, int a, int b)
{
	return (int)(((int64_t)a*b+(1ll<<(fix-1)))>>fix);
}

/**
 * return a/b
 */
int fxp_div (int fix, int a, int b)
{
	if (b>=0) {
		if (a>=0)
			return (int)((((int64_t)a<<fix)+((int64_t)b>>1))/b);
		else
			return (int)((((int64_t)a<<fix)-((int64_t)b>>1))/b);
	} else {
		if (a>=0)
			return (int)((((int64_t)a<<fix)-((int64_t)b>>1))/b);
		else
			return (int)((((int64_t)a<<fix)+((int64_t)b>>1))/b);
	}
}

/**
 * return (a*b+c)/d
 */
int fxp_mul_add_div(int fix, int a, int b, int c, int d)
{
	int64_t t=(int64_t)a*b+((int64_t)c<<fix);
	if (d>=0) {
		if (t>=0)
			return (int)((t+((int64_t)d>>1))/d);
		else
			return (int)((t-((int64_t)d>>1))/d);
	} else {
		if (t>=0)
			return (int)((t-((int64_t)d>>1))/d);
		else
			return (int)((t+((int64_t)d>>1))/d);
	}
}

int fxp_log2(int fix, int a)
{
	int y=0;
	int n=1<<fix;
	while (a<n) {
		a<<=1;
		y--;
	}
	if (fix<31) {
		n<<=1;
		while(a>n) {
			a>>=1;
			y++;
		}
	}

	int64_t a64=(int64_t)a;
	int64_t h=1ll<<(fix-1);
	int64_t t=1ll<<(fix+1);
	for (int i=0;i<fix;i++) {
		y<<=1;
		a64=(a64*a64+h)>>fix;
		if (a64>=t) {
			a64>>=1;
			y++;
		}
	}
	return y;
}
