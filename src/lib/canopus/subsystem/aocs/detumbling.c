#include <canopus/subsystem/aocs/algebra.h>
#include <canopus/subsystem/aocs/detumbling.h>
#include <canopus/nvram.h>

/* I don't know where to put this... */
const static vectorf_t dipole_r = {0,0,0};
const static float bdot_cicle = 1./250;

double MTQ_DIPOLE_MAX_ALLOWED  = .2;
double MTQ_DIPOLE_SURVIVAL_PERCENT = .31;

static void saturate_dipole(vectorf_t dipole_c)
{
	int i;

    for(i=0;i<3;i++) {
    	dipole_c[i] = SATURATE(dipole_c[i], MTQ_DIPOLE_MAX_ALLOWED);
    }
}

void Bxw_mtq_dipole(const vectorf_t Bcurr, const vectorf_t Wcurr,
        vectorf_t dipole_c)
{
	vectorf_t Wwork;
	VCOPY(Wwork, Wcurr);

    deadzone(Wwork, BDOT_GYROSCOPE_DEADZONE_LIMIT);

    cross(dipole_c, Bcurr, Wwork);

    smult(nvram.aocs.Bxw_gain, dipole_c);
    vsubst(dipole_c, dipole_r);

    saturate_dipole(dipole_c);
}

/*
void bdot_mtq_dipole(const vectorf_t Bpre, const vectorf_t Bcurr, vectorf_t dipole_c)
{
    VCOPY(dipole_c, Bpre);
    vsubst(dipole_c,Bcurr);
    smult(bdot_cicle, dipole_c);

    smult(nvram.aocs.bdot_gain, dipole_c);
    vsubst(dipole_c, dipole_r);

	saturate_dipole(dipole_c);
}
*/
