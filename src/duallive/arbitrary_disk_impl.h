#ifndef _ARBITRARY_DISK_IMPL_H
#define _ARBITRARY_DISK_IMPL_H

#include <sys/avl.h>
#include <sys/list.h>
#include <sys/mutex.h>
#include <sys/avl_impl.h>
#include <xutils/file_map.h>
#include "duallive_impl.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ARBITRARY_WORKSPACE "/tmp/duallive_arbitrary"
#define ARBITRARY_LOCK ARBITRARY_WORKSPACE "/lock"

#define ARB_DISK_MAIGC 0x30425241 // ARB0

#define ARB_MAX_UUID_NUM 4096
#define ARB_DFL_DLI_NUM  4 // default number of duallive-pairs in single uuid

#define ARB_DISK_MIN_SIZE						\
    sizeof(arb_disk_head_t) + sizeof(arb_disk_dli_t) * ARB_DFL_DLI_NUM + 64

struct arbitrary_disk_inst;

/*
 *  Arbitrary disk format
 *      arb_disk_head_t   : 64bytes
 *      arb_disk_dli_t[N] : N*64bytes, where N=arb_disk_head_t.dl_num_max
 *      str[X]            : X-bytes, where X is length of all of strings,
 *                          and min(X) = 64
 */

typedef struct arbitrary_disk_head {
	uint32_t	magic;
	uint16_t	dl_num;
	uint16_t	dl_num_max;
	uint32_t	arb_ip;
	uint32_t	arb_port;
	dl_uuid_t	uuid;
	uint32_t	str_pos;
	uint32_t	last_str_pos;
	uint64_t	pad[3];
} arb_disk_head_t;

typedef struct arbitrary_disk_duallive_info {
	uint32_t	hostid[2];
	uint32_t	dataset[2];
	dl_ver_t	version[6];
} arb_disk_dli_t;

typedef struct arbitrary_uuid_lock_manager {
	kmutex_t	mtx;
	avl_tree_t	tree;
	list_t		cache;
	int		arblock;
} arb_uuid_mgr_t;

typedef struct arbitrary_uuid_lock_node {
	dl_uuid_t			uuid;
	avl_node_t			tnode;
	list_node_t			lnode;
	kmutex_t			mtx;
	struct arbitrary_disk_inst *	disk;
	uint32_t			ref;
	uint32_t			cached;
} arb_uuid_lock_t;

typedef struct arbitrary_disk_inst {
	file_map_t		fm;
	arb_uuid_mgr_t *	mgr;
	arb_uuid_lock_t *	lock;
	const dl_desc_t *	desc;
	arb_disk_dli_t *	dli;
} arb_disk_inst_t;

// Notes: arb_uuid_mgr_fini() is not implemented
int arb_uuid_mgr_init(arb_uuid_mgr_t *);
void arb_uuid_mgr_fini(arb_uuid_mgr_t *);
arb_disk_inst_t * arb_disk_open(arb_uuid_mgr_t *, const dl_desc_t *);
void arb_disk_sel_ms(arb_disk_inst_t *, const dl_ver_t *, dl_ver_t *);
void arb_disk_query_ms(arb_disk_inst_t *, const dl_ver_t *, dl_ver_t *);
void arb_disk_close(arb_disk_inst_t *);
void arb_disk_remove(arb_uuid_mgr_t *, const dl_desc_t *);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // _ARBITRARY_DISK_IMPL_H
