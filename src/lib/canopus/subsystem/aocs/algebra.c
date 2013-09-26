#include <canopus/subsystem/aocs/algebra.h>

#include <math.h>

void smult(float s, vectorf_t v) {
    int i;
    for(i=0;i<3;i++) v[i] *= s;
}

void vsubst(vectorf_t l, const vectorf_t r) {
    int i;
    for(i=0;i<3;i++) l[i] -= r[i];
}

void deadzone(vectorf_t v, float deadzone_limit) {
    int i = 0;
    for(;i<3;i++)
        if(!(v[i] > deadzone_limit || v[i] < -deadzone_limit)) v[i] = 0.;
}

/*
 * m 3x3 matrix, v column vector
 */
void applym(const matrixf_t m, vectorf_t v) {
    int i;
    vectorf_t t;

    VCOPY(t, v);

    for(i=0;i<3;i++) {
        v[i] = t[0]*m[i][0] + t[1]*m[i][1] + t[2]*m[i][2];
    }
}

void transp(matrixf_t m) {
	float swap;
	int i, j;

    for(i=0; i<3; i++) {
    	for(j=0; j<i; j++) {
    		swap = m[i][j];
    		m[i][j] = m[j][i];
    		m[j][i] = swap;
    	}
    }
}

void mprod(matrixf_t a, matrixf_t b, matrixf_t res)
{
	int j, k, l;
	float sum;

	for (j = 0; j < 3; j++) for (k = 0; k < 3; k++)
	{
		sum = 0;
		for (l = 0; l < 3; l++)
		{
			sum += a[j][l] * b[l][k];
			res[j][k] = sum;
        }
    }
}

/*
 * m 3x3 matrix, v column vector
 */
void applym_raw(const matrixf_t m, vectori16_t v) {
    int i;
    vectori16_t t;

    VCOPY(t, v);

    for(i=0;i<3;i++) {
        v[i] = (int16_t)((float)t[0])*m[i][0] + ((float)t[1])*m[i][1] + ((float)t[2])*m[i][2];
    }
}

void cross(vectorf_t res, const vectorf_t a, const vectorf_t b) {
    res[0] = a[1]*b[2] - a[2]*b[1];
    res[1] = -(a[0]*b[2] - a[2]*b[0]);
    res[2] = a[0]*b[1] - a[1]*b[0];
}

float norm(const vectorf_t v) {
	return sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
}

float sqnorm(const vectorf_t v) {
	return v[0]*v[0] + v[1]*v[1] + v[2]*v[2];
}
