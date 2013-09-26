#ifndef _CANOPUS_SUPPORT_ALGEBRA_H_
#define _CANOPUS_SUPPORT_ALGEBRA_H_

#include <math.h>
#include <canopus/types.h>

typedef float vectorf_t[3];
typedef float matrixf_t[3][3];

typedef double vectord_t[3];
typedef int16_t vectori16_t[3];

/* Buga buga NaN NaN, floating point bug incantation. */

/* This works for numbers outside of -.5:.5, in -.5:.5 fmod inverts direction */
#define C89ROUNDF_NOn11(__x)\
( (float)\
		fmod(fabs(__x>0?ceil((double)(__x)):floor((double)(__x))), fabs((double)(__x)))<=.5? /*yes: round "up" no: round "down" */\
			( __x>0?ceil((double)(__x)):floor((double)(__x)) ) /*yes: "up" is ceil no: "up" is floor */\
		    :(__x>0?floor((double)(__x)):ceil((double)(__x)) ) /*yes: "up" is ceil no: "up" is floor */\
)

#define C89ROUNDF(__x)\
	( ( __x <= -.5 || __x >= .5)?C89ROUNDF_NOn11(__x):0.)

/* End of floating point bug incantation, your code now has fp bugs */

#define MATRIXF_IDENTITY {{1.f,0.f,0.f}, {0.f,1.f,0.f}, {0.f,0.f,1.f}}

#define VCOPY(dst,src) do {dst[0] = src[0]; dst[1] = src[1]; dst[2] = src[2];} while (0);
#define MCOPY(dst,src) do {dst[0][0] = src[0][0];dst[0][1] = src[0][1];dst[0][2] = src[0][2];\
						   dst[1][0] = src[1][0];dst[1][1] = src[1][1];dst[0][2] = src[1][2];\
						   dst[2][0] = src[2][0];dst[2][1] = src[2][1];dst[2][2] = src[2][2];\
} while (0);

#define SATURATE(d,m) (d>0?(d>m?m:d):(-d>m?-m:d)) // FIXME peter and document/name

void smult(float, vectorf_t);
void vsubst(vectorf_t, const vectorf_t);
void deadzone(vectorf_t, float);

void applym(const matrixf_t, vectorf_t);
void transp(matrixf_t);
void mprod(matrixf_t, matrixf_t, matrixf_t);

void cross(vectorf_t, const vectorf_t, const vectorf_t);

float norm(const vectorf_t);
float sqnorm(const vectorf_t);

#endif
