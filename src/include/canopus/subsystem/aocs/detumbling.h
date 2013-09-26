#ifndef _INCLUDE_BDOT_H_
#define _INCLUDE_BDOT_H_
#include <canopus/subsystem/aocs/algebra.h>

extern float bdot_gain;

void bdot_mtq_dipole(const vectorf_t Bpre, const vectorf_t Bcurr, vectorf_t);
void Bxw_mtq_dipole(const vectorf_t B, const vectorf_t W, vectorf_t);

#define  MTQ_DIPOLE_MAX .2
#define  MTQ_DIPOLE_MAX_0 .4
#define  MTQ_DIPOLE_MAX_1 .4
#define  MTQ_DIPOLE_MAX_2 .2
extern double MTQ_DIPOLE_MAX_ALLOWED;
extern double MTQ_DIPOLE_LOW_POWER_PERCENT;
extern double MTQ_DIPOLE_SURVIVAL_PERCENT;

/* Bdot gain, stored in nvram */
#define BDOT_DEFAULT_NVRAM_GAIN -.006

/* Bxw gain, stored in nvram */
#define BXW_DEFAULT_NVRAM_GAIN -.0005

#endif


