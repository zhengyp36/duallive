#ifndef _WRP_SYS_MACROS_H
#define _WRP_SYS_MACROS_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BUILD_BUG_ON
#define BUILD_BUG_ON(cond) (void)sizeof(char[1-2*!!(cond)])
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr) (sizeof(arr)/sizeof(arr[0]))
#endif

#ifndef offsetof
#define offsetof(s,m) ((size_t)&((s*)0)->m)
#endif

#ifndef membersizeof
#define membersizeof(s,m) (sizeof(((s*)0)->m))
#endif

#ifndef MAX
#define MAX(x,y) ((x) > (y) ? (x) : (y))
#endif

#ifndef MIN
#define MIN(x,y) ((x) < (y) ? (x) : (y))
#endif

#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif

#ifndef ALIGN_TO_PAGE
#define ALIGN_TO_PAGE(size) ((size) + (PAGE_SIZE - 1) / PAGE_SIZE * PAGE_SIZE)
#endif

#ifdef __cplusplus
} // extern "C"
#endif

#endif // _WRP_SYS_MACROS_H
