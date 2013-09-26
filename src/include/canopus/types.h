#ifndef _CANOPUS_TYPES_H_
#define _CANOPUS_TYPES_H_

#include <canopus/compiler_defs.h>

#include <stdint.h>

#ifdef __TMS470__ /* size_t */
# include <string.h>
#elif __USE_POSIX
# include <unistd.h>
#endif

#ifdef __TMS470__ /* off_t */
  typedef uint32_t off_t;
#else
# include <sys/types.h>
#endif

#define INVALID_PTR		    ((void*)-1)
#define INVALID_PTRC(type)	(type)((void*)-1)

#ifndef ARRAY_COUNT
# define ARRAY_COUNT(x) (sizeof(x) / sizeof((x)[0]))
#endif

#define SUCCESS_OR_RETURN(_rv_)				{retval_t _____rv = _rv_; if (RV_SUCCESS != _____rv) return _____rv;}
#define SUCCESS_PARTIAL_OR_RETURN(_rv_)		{retval_t _____rv = _rv_; if ((RV_SUCCESS != _____rv) && (RV_PARTIAL != _____rv)) return _____rv;}

#define ARRAY_INITIALIZE_DEFAULT_VALUE(arraycount, type) \
    [0 ... (arraycount - 1)] = (type)INVALID_PTR

#define IS_PTR_VALID(ptr) (INVALID_PTR != (ptr) && NULL != (ptr))

#define COMPILER_ASSERT(x)	char __attribute__((unused)) __compile_assert__[(x) - 1];

typedef enum endianess {
	ENDIAN_LITTLE,
	ENDIAN_BIG
} endian_t;

/* commonly used for I/O operations */
typedef enum retval {
	RV_SUCCESS = 0,
	RV_TIMEOUT = 1,             /* timer expired */
	RV_BUSY = 2,                /* queue full, bus locked for contention, etc... */
	RV_ILLEGAL = 3,             /* illegal/invalid call arguments */
	RV_NOSPACE = 4,             /* space not available */
	RV_ERROR = 5,               /* any other error */
	RV_EXIST = 6,               /* resource exists already */
	RV_NOENT = 7,               /* entry not available */
	RV_PERM = 8,                /* call not permitted */
	RV_PARTIAL = 9,				/* partial operation successfully accomplished */
	RV_NACK = 10, 				/* everything was alright, except the answer was negative */
    RV_NOTIMPLEMENTED = 11
} retval_t;

#ifdef BOOL_TYPE_MISSING
typedef enum bool_e {
    false	= 0x00,
    true	= 0x01
} bool;
#endif

typedef retval_t future_hook_0(void);
typedef retval_t future_hook_1(void *);
typedef retval_t future_hook_2(void *, void *);
typedef retval_t future_hook_3(void *, void *, void *);
typedef retval_t future_hook_4(void *, void *, void *, void *);
typedef retval_t future_hook_5(void *, void *, void *, void *, void *);
typedef retval_t future_hook_6(void *, void *, void *, void *, void *, void *);

/* objdump -x or nm will show the address of static variables and functions */
#define FUTURE_HOOK_0(_name_)							\
	do {												\
		__attribute__ ((section (".fhooks"))) static future_hook_0 *fh0_##_name_ = (future_hook_0 *)INVALID_PTR;			\
		if (IS_PTR_VALID(fh0_##_name_)) fh0_##_name_();	\
	} while(0)
#define FUTURE_HOOK_1(_name_, _arg_1)				\
	do {													\
		__attribute__ ((section (".fhooks"))) static future_hook_1 *fh1_##_name_ = (future_hook_1 *)INVALID_PTR;			\
		if (IS_PTR_VALID(fh1_##_name_)) fh1_##_name_((void *)_arg_1);			\
	} while(0)
#define FUTURE_HOOK_2(_name_, _arg_1, _arg_2)				\
	do {													\
		__attribute__ ((section (".fhooks"))) static future_hook_2 *fh2_##_name_ = (future_hook_2 *)INVALID_PTR;			\
		if (IS_PTR_VALID(fh2_##_name_)) fh2_##_name_((void *)_arg_1, (void *)_arg_2);			\
	} while(0)
#define FUTURE_HOOK_3(_name_, _arg_1, _arg_2, _arg_3)				\
	do {													\
		__attribute__ ((section (".fhooks"))) static future_hook_3 *fh3_##_name_ = (future_hook_3 *)INVALID_PTR;			\
		if (IS_PTR_VALID(fh3_##_name_)) fh3_##_name_((void *)_arg_1, (void *)_arg_2, (void *)_arg_3);			\
	} while(0)
#define FUTURE_HOOK_4(_name_, _arg_1, _arg_2, _arg_3, _arg_4) \
    do { \
    	__attribute__ ((section (".fhooks"))) static future_hook_4 *fh4_##_name_ = (future_hook_4 *)INVALID_PTR; \
        if (IS_PTR_VALID(fh4_##_name_)) fh4_##_name_((void *)_arg_1, (void *)_arg_2, (void *)_arg_3, (void *)_arg_4); \
    } while(0)
#define FUTURE_HOOK_5(_name_, _arg_1, _arg_2, _arg_3, _arg_4, _arg_5) \
    do { \
    	__attribute__ ((section (".fhooks"))) static future_hook_5 *fh5_##_name_ = (future_hook_5 *)INVALID_PTR; \
        if (IS_PTR_VALID(fh5_##_name_)) fh5_##_name_((void *)_arg_1, (void *)_arg_2, (void *)_arg_3, (void *)_arg_4, (void *)_arg_5); \
    } while(0)
#define FUTURE_HOOK_6(_name_, _arg_1, _arg_2, _arg_3, _arg_4, _arg_5, _arg_6) \
    do { \
    	__attribute__ ((section (".fhooks"))) static future_hook_6 *fh6_##_name_ = (future_hook_6 *)INVALID_PTR; \
        if (IS_PTR_VALID(fh6_##_name_)) fh6_##_name_((void *)_arg_1, (void *)_arg_2, (void *)_arg_3, (void *)_arg_4, (void *)_arg_5, (void *)_arg_6); \
    } while(0)

#endif
