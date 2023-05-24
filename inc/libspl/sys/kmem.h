#ifndef _WRP_SYS_KMEM_H
#define _WRP_SYS_KMEM_H

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#define	KM_SLEEP	0x00000000	/* same as KM_SLEEP */
#define	KM_NOSLEEP	0x00000001	/* same as KM_NOSLEEP */

#ifdef DL_DEBUG
void * kmem_alloc(size_t, int);
void kmem_free(void *, size_t);
#else // !DL_DEBUG
#define	kmem_alloc(size, flags)		((void) sizeof (flags), malloc(size))
#define	kmem_free(ptr, size)		((void) sizeof (size), free(ptr))
#endif // !DL_DEBUG

#ifndef __KERNEL__
#include <string.h>
#endif // __KERNEL__

#ifdef __cplusplus
} // extern "C"
#endif

#endif // _WRP_SYS_KMEM_H
