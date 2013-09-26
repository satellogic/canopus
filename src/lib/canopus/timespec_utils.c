#include <canopus/types.h>
#include <canopus/time.h>

retval_t
timespec_copy(timespec_t *dest, const timespec_t *src)
{
	dest->tv_sec  = src->tv_sec;
	dest->tv_nsec = src->tv_nsec;

	return RV_SUCCESS;
}

retval_t
timespec_str(char *dest, size_t size, const timespec_t *ts)
{
	/** TODO **/
	return RV_ERROR;
}
