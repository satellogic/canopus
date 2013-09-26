#ifndef _CANOPUS_NVRAM_H_
#define _CANOPUS_NVRAM_H_

#include <canopus/types.h>
#include <canopus/time.h>
#include <unistd.h> // FIXME size_t?

/* TODO replace timeout_t by portTickType */

typedef enum {
	NVRAM_BLOB_INTERNAL	= 0,	/* reserved */
	NVRAM_BLOB_TIME		= 1
} nvram_blob_id;

/**
* @param [in] id	Id
* @param [in] buf	Blob
* @param [in] bufsize	Size of blob
* @param [in] ms	Timeout
*
* @retval RV_SUCCESS, RV_BUSY, RV_TIMEOUT, RV_NOSPACE, RV_NOENT
*/
retval_t nvram_blob_set(const char nvram_blob_id id, const char *buf, size_t bufsize, timeout_t ms);

/**
* @param [in] id         Id
* @param [out] buf       Blob
* @param [in] bufsize    Size of blob
* @param [in] ms         Timeout
*
* @retval RV_SUCCESS, RV_BUSY, RV_TIMEOUT, RV_NOSPACE, RV_NOENT
*/
retval_t nvram_blob_get(const char nvram_blob_id id, char *buf, size_t bufsize, timeout_t ms);

#endif

