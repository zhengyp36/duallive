#ifndef _WRP_SYS_DEBUG_H
#define _WRP_SYS_DEBUG_H

#include <assert.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VERIFY(cond) assert(cond)
#define VERIFY0(cond) assert(!(cond))
#define VERIFY3S(x,op,y) assert((int64_t)(x) op (int64_t)(y))

#define EQUIV(x,y) VERIFY(!!(x) == !!(y))

#define ASSERT(cond) assert(cond)
#define ASSERT3P(x,op,y) assert((const uintptr_t)(x) op (const uintptr_t)(y))
#define ASSERT3U(x,op,y) assert((uint32_t)(x) op (uint32_t)(y))

#ifdef __cplusplus
} // extern "C"
#endif

#endif // _WRP_SYS_DEBUG_H
