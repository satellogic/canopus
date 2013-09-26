#ifndef _CANOPUS_NVRAM_KEYS_H_
#define _CANOPUS_NVRAM_KEYS_H_

#include <canopus/types.h>
#include <canopus/time.h>
#include <unistd.h> // FIXME size_t?

/* TODO replace timeout_t by portTickType */

/**
* @param [in] key        Key
* @param [in] buf        Value
* @param [in] bufsize    Size of value
* @param [in] ms         Timeout
*
* @retval RV_SUCCESS, RV_BUSY, RV_TIMEOUT, RV_NOSPACE
*/
retval_t
nvram_keys_set(const char *key, const char *buf, size_t bufsize, timeout_t ms);

/**
* @param [in] key        Key
* @param [in] buf        Buffer
* @param [out] buf       Value (nul-terminated)
* @param [in] bufsize    Size of buffer
* @param [in] ms         Timeout
*
* @retval RV_SUCCESS, RV_BUSY, RV_TIMEOUT, RV_NOSPACE, RV_NOENT
*/
retval_t
nvram_keys_get(const char *key, char *buf, size_t bufsize, timeout_t ms);

#endif

