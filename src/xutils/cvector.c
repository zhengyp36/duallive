#include <sys/kmem.h>
#include <xutils/cvector.h>

#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif

static inline size_t
calc_expand_size(size_t orig_size, size_t used_size, size_t need_size)
{
	size_t size = orig_size * 2;
	while (used_size + need_size > size)
		size *= 2;
	return (size);
}

static void
expand(void **pbuf, size_t *size, size_t used_size, size_t need_size)
{
	size_t new_size = calc_expand_size(*size, used_size, need_size);
	void *buf = kmem_alloc(new_size, KM_SLEEP);

	memcpy(buf, *pbuf, used_size);
	kmem_free(*pbuf, *size);

	*pbuf = buf;
	*size = new_size;
}

void
vec_init(vec_t *v, size_t elem_size)
{
	v->buf = kmem_alloc(PAGE_SIZE, KM_SLEEP);
	v->size = PAGE_SIZE;
	v->elem_size = elem_size;
	v->pos = 0;
}

void
vec_add(vec_t *v, void *elem)
{
	if (v->pos + v->elem_size > v->size)
		expand(&v->buf, &v->size, v->pos, v->elem_size);

	memcpy(v->buf + v->pos, elem, v->elem_size);
	v->pos += v->elem_size;
}

void
vec_fini(vec_t *v)
{
	kmem_free(v->buf, v->size);
}

void
strvec_init(strvec_t *v)
{
	v->str = kmem_alloc(PAGE_SIZE, KM_SLEEP);
	v->str[0] = '\0';
	v->size = PAGE_SIZE;
	v->last_pos = 0;
	v->pos = 1;
}

uint32_t
strvec_add(strvec_t *v, const char *str)
{
	size_t len = strlen(str) + 1;
	if (v->last_pos + len == v->pos &&
	    memcmp(&v->str[v->last_pos], str, len) == 0)
		return (v->last_pos);

	if (v->pos + len > v->size)
		expand((void**)&v->str, &v->size, v->pos, len);

	memcpy(&v->str[v->pos], str, len);
	v->last_pos = v->pos;
	v->pos += len;

	return (v->last_pos);
}

void
strvec_fini(strvec_t *v)
{
	kmem_free(v->str, v->size);
}
