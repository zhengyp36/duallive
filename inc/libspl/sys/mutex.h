#ifndef _WRP_SYS_MUTEX_H
#define _WRP_SYS_MUTEX_H

#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct kmutex {
	pthread_mutex_t		m_lock;
	pthread_t		m_owner;
} kmutex_t;

#define	MUTEX_DEFAULT		0
#define	MUTEX_NOLOCKDEP		MUTEX_DEFAULT
#define	MUTEX_HELD(mp)		pthread_equal((mp)->m_owner, pthread_self())
#define	MUTEX_NOT_HELD(mp)	!MUTEX_HELD(mp)

void mutex_init(kmutex_t *mp, char *name, int type, void *cookie);
void mutex_destroy(kmutex_t *mp);
void mutex_enter(kmutex_t *mp);
void mutex_exit(kmutex_t *mp);
int mutex_tryenter(kmutex_t *mp);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // _WRP_SYS_MUTEX_H
