#ifndef _WRP_SYS_TYPES_H
#define _WRP_SYS_TYPES_H

#include <stdint.h>
#include <stddef.h>
#include_next <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	B_FALSE = 0,
	B_TRUE = 1
} boolean_t;

typedef unsigned char		uchar_t;
typedef unsigned short		ushort_t;
typedef unsigned int		uint_t;
typedef unsigned long		ulong_t;
typedef unsigned long long	u_longlong_t;
typedef long long		longlong_t;

#ifdef __cplusplus
} // extern "C"
#endif

#endif // _WRP_SYS_TYPES_H
