#include <canopus/types.h>
#include <canopus/subsystem/aocs/algebra.h>
#include <canopus/subsystem/aocs/pointing.h>
#include <canopus/subsystem/aocs/aocs.h>
#include <canopus/nvram.h>

retval_t preliminary_feedback(const vectorf_t b, const vectorf_t u, vectorf_t m_coils)
{
	cross(m_coils, b, u);

	smult(sqnorm(b), m_coils);

	return RV_SUCCESS;
}

retval_t lovera_prefeedback(const vectorf_t q, const vectorf_t w, vectorf_t u)
{
	vectorf_t d_term;
	const matrixf_t inertia = AOCS_INERTIA;

	VCOPY(u, q)
	smult(nvram.aocs.lovera_prefeed_conf.epsilon * nvram.aocs.lovera_prefeed_conf.epsilon
			* nvram.aocs.lovera_prefeed_conf.kp, u);

	FUTURE_HOOK_1(modify_u, &u);

	VCOPY(d_term, w)
	applym(inertia, d_term);
	smult(nvram.aocs.lovera_prefeed_conf.kv, d_term);

    d_term[0] = SATURATE(d_term[0],nvram.aocs.lovera_prefeed_conf.beta);
    d_term[1] = SATURATE(d_term[1],nvram.aocs.lovera_prefeed_conf.beta);
    d_term[2] = SATURATE(d_term[2],nvram.aocs.lovera_prefeed_conf.beta);

	smult(nvram.aocs.lovera_prefeed_conf.epsilon, d_term);

	FUTURE_HOOK_1(modify_d_term, &d_term);

	u[0] += d_term[0];
	u[1] += d_term[1];
	u[2] += d_term[2];

	smult(-1., u);

	return RV_SUCCESS;
}

retval_t lovera_mtq_dipole(const vectorf_t q, const vectorf_t w, const vectorf_t b, vectorf_t dipole)
{
	retval_t rv;
	vectorf_t u;

	rv = lovera_prefeedback(q, w, u);
	if(rv != RV_SUCCESS) return rv;

	rv = preliminary_feedback(b, u, dipole);
	if(rv != RV_SUCCESS) return rv;

	return RV_SUCCESS;
}

/*
    Slides Veh'iculos Espaciales y Misiles
 Tema 4: Control y determinacion de la actitud
        Parte I: Estimacion de actitud
          Rafael Vazquez Valenzuela

   or Chapter 4 Chris Hall.

   t4p1.pdf or attde.pdf respectively in AOCS Dropbox.
   Variable names are according to Vazquez Valenzuela.
   W is body, V is inertial.
*/
retval_t triad(const vectorf_t W_1, const vectorf_t V_1, const vectorf_t W_2,  const vectorf_t V_2, matrixf_t A)
{
	vectorf_t r1, r2, r3, s1, s2, s3;
	matrixf_t Mref, Mobs;

	VCOPY(r1, V_1);

	cross(r2, V_1, V_2);
	smult(1./norm(r2), r2);

	cross(r3, V_1, r2);
	smult(1./norm(r3), r3);


	VCOPY(s1, W_1);

	cross(s2, W_1, W_2);
	smult(1./norm(s2), s2);

	cross(s3, W_1, s2);
	smult(1./norm(s3), s3);


	Mref[0][0] = r1[0]; Mref[0][1] = r2[0]; Mref[0][2] = r3[0];
	Mref[1][0] = r1[1]; Mref[1][1] = r2[1]; Mref[1][2] = r3[1];
	Mref[2][0] = r1[2]; Mref[2][1] = r2[2]; Mref[2][2] = r3[2];

	Mobs[0][0] = s1[0]; Mobs[0][1] = s2[0]; Mobs[0][2] = s3[0];
	Mobs[1][0] = s1[1]; Mobs[1][1] = s2[1]; Mobs[1][2] = s3[1];
	Mobs[2][0] = s1[2]; Mobs[2][1] = s2[2]; Mobs[2][2] = s3[2];

	transp(Mref);

	mprod(Mobs, Mref, A);

	return RV_SUCCESS;
}

inline float SIGN(float x) {
	return (x >= 0.0f) ? +1.0f : -1.0f;
}

inline float NORM(float a, float b, float c, float d) {
	return sqrt(a * a + b * b + c * c + d * d);
}

// Cuentas: http://www.cg.info.hiroshima-cu.ac.jp/~miyazaki/knowledge/teche52.html
retval_t rotmat2quat(matrixf_t mat, vectorf_t quatv, float *quats)
{
	float r;

#define r11 mat[0][0]
#define r12 mat[0][1]
#define r13 mat[0][2]

#define r21 mat[1][0]
#define r22 mat[1][1]
#define r23 mat[1][2]

#define r31 mat[2][0]
#define r32 mat[2][1]
#define r33 mat[2][2]

#define q0 (*quats)
#define q1 quatv[0]
#define q2 quatv[1]
#define q3 quatv[2]

	q0 = ( r11 + r22 + r33 + 1.0f) / 4.0f;
	q1 = ( r11 - r22 - r33 + 1.0f) / 4.0f;
	q2 = (-r11 + r22 - r33 + 1.0f) / 4.0f;
	q3 = (-r11 - r22 + r33 + 1.0f) / 4.0f;
	if(q0 < 0.0f) q0 = 0.0f;
	if(q1 < 0.0f) q1 = 0.0f;
	if(q2 < 0.0f) q2 = 0.0f;
	if(q3 < 0.0f) q3 = 0.0f;
	q0 = sqrt(q0);
	q1 = sqrt(q1);
	q2 = sqrt(q2);
	q3 = sqrt(q3);
	if(q0 >= q1 && q0 >= q2 && q0 >= q3) {
	    q0 *= +1.0f;
	    q1 *= SIGN(r32 - r23);
	    q2 *= SIGN(r13 - r31);
	    q3 *= SIGN(r21 - r12);
	} else if(q1 >= q0 && q1 >= q2 && q1 >= q3) {
	    q0 *= SIGN(r32 - r23);
	    q1 *= +1.0f;
	    q2 *= SIGN(r21 + r12);
	    q3 *= SIGN(r13 + r31);
	} else if(q2 >= q0 && q2 >= q1 && q2 >= q3) {
	    q0 *= SIGN(r13 - r31);
	    q1 *= SIGN(r21 + r12);
	    q2 *= +1.0f;
	    q3 *= SIGN(r32 + r23);
	} else if(q3 >= q0 && q3 >= q1 && q3 >= q2) {
	    q0 *= SIGN(r21 - r12);
	    q1 *= SIGN(r31 + r13);
	    q2 *= SIGN(r32 + r23);
	    q3 *= +1.0f;
	} else {
		return RV_ERROR;
	}

	r = NORM(q0, q1, q2, q3);
	q0 /= r;
	q1 /= r;
	q2 /= r;
	q3 /= r;

	return RV_SUCCESS;

#undef r11
#undef r12
#undef r13

#undef r21
#undef r22
#undef r23

#undef r31
#undef r32
#undef r33

#undef q0
#undef q1
#undef q2
#undef q3
}
