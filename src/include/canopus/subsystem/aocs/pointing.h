#ifndef POINTING_H_
#define POINTING_H_

retval_t preliminary_feedback(const vectorf_t b,  const vectorf_t u, vectorf_t m_coils);
retval_t lovera_prefeedback(const vectorf_t q,  const vectorf_t w, vectorf_t u);

retval_t lovera_mtq_dipole(const vectorf_t q, const vectorf_t w, const vectorf_t b, vectorf_t dipole);

retval_t rotmat2quat(matrixf_t mat, vectorf_t quatv, float *quats);

retval_t triad(const vectorf_t W_1, const vectorf_t W_2, const vectorf_t V_1, const vectorf_t V_2, matrixf_t A);

#endif /* POINTING_H_ */
