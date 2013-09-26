#include <canopus/subsystem/aocs/css.h>
#include <canopus/subsystem/aocs/algebra.h>

vectord_t sunsensor_positive_nominal = {2.7, 2.7, 2.7};
vectord_t sunsensor_negative_nominal = {2.7, 2.7, 2.7};

void volts2sunvec(vectord_t sun, const vectord_t positive_xyz_in, const vectord_t negative_xyz_in)
{
	vectord_t positive_xyz, negative_xyz;
	int i = 0;
	double norm;

	VCOPY(positive_xyz, positive_xyz_in);
	VCOPY(negative_xyz, negative_xyz_in);

	for(i = 0; i < 3; i++) {
		SETZ(positive, i);
		COSINE_WITH_NORMAL(positive, i);
		SETZ(negative, i);
		COSINE_WITH_NORMAL(negative, i);

		sun[i] = positive_xyz[i]>negative_xyz[i]?positive_xyz[i]:-negative_xyz[i];
	}

	norm = sqrt(pow(sun[0],2) + pow(sun[1],2) + pow(sun[2],2));

	for(i = 0; i < 3; i++) sun[i] /= norm;
}

void fill_up_sun_t(sun_data_t *data, const css_adc_measurement_volts_t *measurement) {
	vectord_t sun_vector;
	vectord_t volts_pos;
	vectord_t volts_neg;

	if(&data->css_adc_measurement_volts != measurement)
		data->css_adc_measurement_volts = *measurement;

	volts_pos[0] = measurement->x_pos;
	volts_pos[1] = measurement->y_pos;
	volts_pos[2] = measurement->z_pos;

	volts_neg[0] = measurement->x_neg;
	volts_neg[1] = measurement->y_neg;
	volts_neg[2] = measurement->z_neg;

	volts2sunvec(sun_vector, volts_pos, volts_neg);

	data->sun_vector.x = sun_vector[0];
	data->sun_vector.y = sun_vector[1];
	data->sun_vector.z = sun_vector[2];

	/*  Use 2<<13 to avoid two' s complement FFFF = -1 border case
	 * without substracting in conversion.
	 */
	data->sun_vector_bits.x = data->sun_vector.x * (2 << 13);
	data->sun_vector_bits.y = data->sun_vector.y * (2 << 13);
	data->sun_vector_bits.z = data->sun_vector.z * (2 << 13);
}
