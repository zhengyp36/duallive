#include <errno.h>
#include <string.h>
#include <sys/kmem.h>
#include <sys/debug.h>
#include <sys/mutex.h>

#ifdef DL_DEBUG

static uint32_t alloc_cnt = 0;
static uint32_t free_cnt = 0;

__attribute__((used, destructor))
static void
kmem_leak_check(void)
{
	ASSERT(alloc_cnt == free_cnt);
}

void *
kmem_alloc(size_t size, int flags)
{
	void *ptr = malloc(size + sizeof(size_t));
	ASSERT(ptr || flags != KM_SLEEP);
	if (ptr) {
		*(size_t*)(ptr + size) = size;
		__sync_fetch_and_add(&alloc_cnt, 1);
	}
	return (ptr);
}

void
kmem_free(void *ptr, size_t size)
{
	ASSERT(ptr && size);
	ASSERT(*(size_t*)(ptr + size) == size);
	__sync_fetch_and_add(&free_cnt, 1);
	free(ptr);
}
#endif // DL_DEBUG

void
mutex_init(kmutex_t *mp, char *name, int type, void *cookie)
{
	(void) name, (void) type, (void) cookie;
	VERIFY0(pthread_mutex_init(&mp->m_lock, NULL));
	memset(&mp->m_owner, 0, sizeof (pthread_t));
}

void
mutex_destroy(kmutex_t *mp)
{
	VERIFY0(pthread_mutex_destroy(&mp->m_lock));
}

void
mutex_enter(kmutex_t *mp)
{
	VERIFY0(pthread_mutex_lock(&mp->m_lock));
	mp->m_owner = pthread_self();
}

int
mutex_tryenter(kmutex_t *mp)
{
	int error = pthread_mutex_trylock(&mp->m_lock);
	if (error == 0) {
		mp->m_owner = pthread_self();
		return (1);
	} else {
		VERIFY3S(error, ==, EBUSY);
		return (0);
	}
}

void
mutex_exit(kmutex_t *mp)
{
	memset(&mp->m_owner, 0, sizeof (pthread_t));
	VERIFY0(pthread_mutex_unlock(&mp->m_lock));
}
