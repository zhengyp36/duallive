#ifndef _XUTILS_CVECTOR_H
#define _XUTILS_CVECTOR_H

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	void *	buf;
	size_t	size;
	size_t	elem_size;
	size_t	pos;
} vec_t;

typedef struct {
	char *	str;
	size_t	size;
	size_t	last_pos;
	size_t	pos;
} strvec_t;

void vec_init(vec_t *, size_t elem_size);
void vec_add(vec_t *, void *elem);
void vec_fini(vec_t *);

void strvec_init(strvec_t *);
uint32_t strvec_add(strvec_t *, const char *);
void strvec_fini(strvec_t *);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // _XUTILS_CVECTOR_H
