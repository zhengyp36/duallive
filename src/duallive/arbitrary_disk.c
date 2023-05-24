#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/kmem.h>
#include <sys/file.h>
#include <sys/debug.h>
#include <xutils/shell.h>
#include "duallive_impl.h"
#include "arbitrary_disk.h"
#include "arbitrary_disk_impl.h"

static inline arb_disk_head_t *
fm2head(file_map_t *fm)
{
	return (fm->buf);
}

static inline arb_disk_head_t *
disk2head(arb_disk_inst_t *disk)
{
	return (disk->fm.buf);
}

static inline arb_disk_dli_t *
fm2dli(file_map_t *fm)
{
	return ((void*)&fm2head(fm)[1]);
}

static inline uint32_t
arb_disk_stroff(file_map_t *fm)
{
	return (sizeof(arb_disk_head_t) +
	    sizeof(arb_disk_dli_t) * fm2head(fm)->dl_num_max);
}

static inline uint32_t
arb_disk_used_space(file_map_t *fm)
{
	return (arb_disk_stroff(fm) + fm2head(fm)->str_pos);
}

static inline uint32_t
arb_disk_idle_space(file_map_t *fm)
{
	uint32_t used = arb_disk_used_space(fm);
	return (used < fm->size ? fm->size - used : 0);
}

static inline void
arb_disk_expand(file_map_t *fm, size_t expand_size)
{
	VERIFY(!file_map_expand(fm, ALIGN_TO_PAGE(expand_size)));
}

static inline char *
arb_disk_strbuf(file_map_t *fm)
{
	return ((char*)fm->buf + arb_disk_stroff(fm));
}

static void
arb_disk_strbuf_shift(file_map_t *fm, uint32_t off)
{
	uint32_t len = fm2head(fm)->str_pos - arb_disk_stroff(fm);
	char *orig_strbuf = arb_disk_strbuf(fm);

	char *tmpbuf = kmem_alloc(len, KM_SLEEP);
	memcpy(tmpbuf, orig_strbuf, len);
	memcpy(orig_strbuf + off, tmpbuf, len);
	kmem_free(tmpbuf, len);
}

static uint32_t
arb_disk_add_str(file_map_t *fm, const char *str)
{
	uint32_t len = strlen(str) + 1;
	if (arb_disk_used_space(fm) + len > fm->size)
		arb_disk_expand(fm, len);

	char *strbuf = arb_disk_strbuf(fm);
	arb_disk_head_t *head = fm2head(fm);
	uint32_t last_len = head->str_pos - head->last_str_pos;

	if (len != last_len || memcmp(str, &strbuf[head->last_str_pos], len)) {
		memcpy(&strbuf[head->str_pos], str, len);
		head->last_str_pos = head->str_pos;
		head->str_pos += len;
	}

	return (head->last_str_pos);
}

static void
arb_disk_format(file_map_t *fm)
{
	file_map_set_size(fm, ALIGN_TO_PAGE(ARB_DISK_MIN_SIZE));
	memset(fm->buf, 0, fm->size);

	arb_disk_head_t *head = fm2head(fm);
	head->magic = ARB_DISK_MAIGC;
	head->dl_num_max = ARB_DFL_DLI_NUM;

	// The first string is null-string
	*arb_disk_strbuf(fm) = '\0';
	head->str_pos = 1;
	head->last_str_pos = 0;
}

static void
arb_disk_set_comm_info(file_map_t *fm, const dl_desc_t *desc)
{
	arb_disk_head_t *head = fm2head(fm);
	head->arb_ip = arb_disk_add_str(fm, desc->arbitrary_ip);
	head->arb_port = desc->arbitrary_port;
	head->uuid = desc->uuid;
}

static void
arb_disk_expand_dli_space(file_map_t *fm)
{
	size_t dli_size = sizeof(arb_disk_dli_t) * fm2head(fm)->dl_num_max;
	size_t idle_size = arb_disk_idle_space(fm);
	if (idle_size < dli_size)
		arb_disk_expand(fm, dli_size);
	arb_disk_strbuf_shift(fm, dli_size);
	fm2head(fm)->dl_num_max *= 2;
}

static inline int
dli_equ_desc(const arb_disk_dli_t *dli, const dl_desc_t *desc, const char *str)
{
	return (dli->hostid[0] == desc->local_hostid &&
	    dli->hostid[1] == desc->remote_hostid &&
	    !strcmp(&str[dli->dataset[0]], desc->local_dataset) &&
	    !strcmp(&str[dli->dataset[1]], desc->remote_dataset));
}

static const dl_desc_t *
desc_standard(const dl_desc_t *desc, dl_desc_t *tmp)
{
	if (desc->local_hostid < desc->remote_hostid)
		return (desc);

	tmp->local_hostid = desc->remote_hostid;
	tmp->remote_hostid = desc->local_hostid;
	tmp->local_dataset = desc->remote_dataset;
	tmp->remote_dataset = desc->local_dataset;
	return (tmp);
}

static arb_disk_dli_t *
arb_disk_find_dli(file_map_t *fm, const dl_desc_t *desc, uint16_t *ndx)
{
	uint16_t index;
	if (!ndx)
		ndx = &index;

	dl_desc_t tmp;
	const dl_desc_t *standard = desc_standard(desc, &tmp);

	const char *strbuf = arb_disk_strbuf(fm);
	arb_disk_dli_t *dli = fm2dli(fm);
	for (*ndx = 0; *ndx < fm2head(fm)->dl_num; ++*ndx)
		if (dli_equ_desc(&dli[*ndx], standard, strbuf))
			return (&dli[*ndx]);

	*ndx = (uint16_t)-1;
	return (NULL);
}

static arb_disk_dli_t *
arb_disk_insert_dli(file_map_t *fm, const dl_desc_t *desc)
{
	dl_desc_t tmp;
	const dl_desc_t *standard = desc_standard(desc, &tmp);

	arb_disk_dli_t *dli = arb_disk_find_dli(fm, standard, NULL);
	if (dli)
		return (dli);

	arb_disk_head_t *head = fm2head(fm);
	if (head->dl_num == head->dl_num_max) {
		arb_disk_expand_dli_space(fm);
		head = fm2head(fm); // head is changed
	}
	ASSERT(head->dl_num < head->dl_num_max);

	dli = &fm2dli(fm)[head->dl_num++];
	dli->hostid[0] = standard->local_hostid;
	dli->hostid[1] = standard->remote_hostid;
	dli->dataset[0] = arb_disk_add_str(fm, standard->local_dataset);
	dli->dataset[1] = arb_disk_add_str(fm, standard->remote_dataset);

	if (head->dl_num == 1)
		arb_disk_set_comm_info(fm, standard);
	return (dli);
}

static void
arb_disk_remove_dli(file_map_t *fm, const dl_desc_t *desc)
{
	uint16_t ndx;
	if (arb_disk_find_dli(fm, desc, &ndx)) {
		arb_disk_head_t *head = fm2head(fm);
		VERIFY(head->dl_num > 0);
		head->dl_num--;

		arb_disk_dli_t *dli = fm2dli(fm);
		for (uint16_t i = ndx; i < head->dl_num; i++)
			dli[i] = dli[i+1];
	}
}

static int
get_flock(void)
{
#ifndef __KERNEL__
	int fd = open(ARBITRARY_LOCK, O_RDWR);
	if (!flock(fd, LOCK_EX | LOCK_NB))
		return (fd);
	close(fd);
	return (-1);
#else
	return (0);
#endif
}

static void
put_flock(int fd)
{
#ifndef __KERNEL__
	flock(fd, LOCK_UN);
#endif
}

static int
dl_uuid_cmp(const void *_u1, const void *_u2)
{
	const dl_uuid_t *u1 = (const dl_uuid_t*)_u1;
	const dl_uuid_t *u2 = (const dl_uuid_t*)_u2;

	int r;
	return ((r = TREE_CMP(u1->uuid[0], u2->uuid[0])) ? r :
	    TREE_CMP(u1->uuid[1], u2->uuid[1]));
}

int
arb_uuid_mgr_init(arb_uuid_mgr_t *mgr)
{
	int fd = get_flock();
	if (fd < 0)
		return (-1);

	size_t size = sizeof(arb_uuid_lock_t);
	size_t toff = offsetof(arb_uuid_lock_t, tnode);
	size_t loff = offsetof(arb_uuid_lock_t, lnode);

	mutex_init(&mgr->mtx, NULL, MUTEX_DEFAULT, NULL);
	list_create(&mgr->cache, size, loff);

	// As the search-key, uuid must be the first member of the structure
	BUILD_BUG_ON(offsetof(arb_uuid_lock_t, uuid));
	avl_create(&mgr->tree, dl_uuid_cmp, size, toff);

	mgr->arblock = fd;
	return (0);
}

static void cache_cleanup(arb_uuid_mgr_t *, uint32_t);

void
arb_uuid_mgr_fini(arb_uuid_mgr_t *mgr)
{
	cache_cleanup(mgr, 0);
	VERIFY(avl_numnodes(&mgr->tree) == 0);
	list_destroy(&mgr->cache);
	avl_destroy(&mgr->tree);
	put_flock(mgr->arblock);
}

static arb_uuid_lock_t *
uuid_lock_alloc(const dl_uuid_t *uuid)
{
	arb_uuid_lock_t *n = kmem_alloc(sizeof(*n), KM_SLEEP);
	memcpy(&n->uuid, uuid, sizeof(n->uuid));
	mutex_init(&n->mtx, NULL, MUTEX_DEFAULT, NULL);
	n->disk = NULL;
	n->ref = 0;
	n->cached = 0;
	return (n);
}

static void
uuid_lock_free(arb_uuid_lock_t *n)
{
	mutex_destroy(&n->mtx);
	kmem_free(n, sizeof(*n));
}

static inline int
uuid_lock_cachable(arb_uuid_lock_t *n)
{
	arb_disk_inst_t *disk = n->disk;
	return (disk && disk2head(disk) && disk2head(disk)->dl_num);
}

static void arb_disk_unload(arb_disk_inst_t *);

static void
cache_cleanup(arb_uuid_mgr_t *mgr, uint32_t limit)
{
	while (avl_numnodes(&mgr->tree) > limit &&
	    !list_is_empty(&mgr->cache)) {
		arb_uuid_lock_t *n = list_remove_head(&mgr->cache);
		avl_remove(&mgr->tree, n);
		arb_disk_unload(n->disk);
		uuid_lock_free(n);
	}
}

static arb_uuid_lock_t *
arb_uuid_lock(arb_uuid_mgr_t *mgr, const dl_uuid_t *uuid)
{
	avl_index_t where;

	mutex_enter(&mgr->mtx);
	arb_uuid_lock_t *n = avl_find(&mgr->tree, uuid, &where);
	if (!n) {
		n = uuid_lock_alloc(uuid);
		avl_insert(&mgr->tree, n, where);
		cache_cleanup(mgr, ARB_MAX_UUID_NUM);
	} else if (n->cached) {
		list_remove(&mgr->cache, n);
		n->cached = 0;
	}
	n->ref++;
	mutex_exit(&mgr->mtx);

	mutex_enter(&n->mtx);
	return (n);
}

static void
arb_uuid_unlock(arb_uuid_mgr_t *mgr, arb_uuid_lock_t *n)
{
	mutex_exit(&n->mtx);

	mutex_enter(&mgr->mtx);
	VERIFY(n->ref > 0);
	n->ref--;

	if (n->ref == 0) {
		if (uuid_lock_cachable(n)) {
			VERIFY(!n->cached);
			n->cached = 1;
			list_insert_tail(&mgr->cache, n);
			cache_cleanup(mgr, ARB_MAX_UUID_NUM);
		} else {
			avl_remove(&mgr->tree, n);
			arb_disk_unload(n->disk);
			uuid_lock_free(n);
		}
	}

	mutex_exit(&mgr->mtx);
}

static arb_disk_inst_t *
arb_disk_load(const dl_uuid_t *uuid, int create)
{
	char fname[1024];
	snprintf(fname, sizeof(fname), "%s/%lx.%lx.dat",
	    ARBITRARY_WORKSPACE, uuid->uuid[0], uuid->uuid[1]);
	if (!create && access(fname, F_OK))
		return (NULL);

	file_map_t fm;
	file_map_init(&fm, fname);
	if (file_map_open(&fm)) {
		file_map_fini(&fm);
		return (NULL);
	}

	uint32_t min_size = ALIGN_TO_PAGE(ARB_DISK_MIN_SIZE);
	if (fm2head(&fm)->magic != ARB_DISK_MAIGC || fm.size < min_size) {
		if (!create) {
			file_map_fini(&fm);
			shell("rm -f %s", fname);
			return (NULL);
		}
		arb_disk_format(&fm);
	}

	arb_disk_inst_t *disk = kmem_alloc(sizeof(*disk), KM_SLEEP);
	disk->fm = fm;
	disk->mgr = NULL;
	disk->lock = NULL;
	disk->desc = NULL;
	disk->dli = NULL;

	return (disk);
}

static void
arb_disk_unload(arb_disk_inst_t *disk)
{
	if (disk) {
		if (disk2head(disk) && disk2head(disk)->dl_num == 0)
			shell("rm -rf %s", disk->fm.path);
		file_map_fini(&disk->fm);
		kmem_free(disk, sizeof(*disk));
	}
}

static arb_disk_inst_t *
arb_disk_open_impl(arb_uuid_mgr_t *mgr, const dl_desc_t *desc, int create)
{
	arb_uuid_lock_t *lock = arb_uuid_lock(mgr, &desc->uuid);
	if (lock->disk) {
		ASSERT(lock->disk->mgr == mgr);
		ASSERT(lock->disk->lock == lock);
		return (lock->disk);
	}

	lock->disk = arb_disk_load(&desc->uuid, create);
	if (lock->disk) {
		lock->disk->mgr = mgr;
		lock->disk->lock = lock;
		return (lock->disk);
	}

	arb_uuid_unlock(mgr, lock);
	return (NULL);
}

arb_disk_inst_t *
arb_disk_open(arb_uuid_mgr_t *mgr, const dl_desc_t *desc)
{
	arb_disk_inst_t *disk = arb_disk_open_impl(mgr, desc, 1);
	VERIFY(disk);

	disk->dli = arb_disk_insert_dli(&disk->fm, desc);
	VERIFY(disk->dli);

	disk->desc = desc;
	return (disk);
}

void
arb_disk_close(arb_disk_inst_t *disk)
{
	disk->dli = NULL;
	disk->desc = NULL;
	arb_uuid_unlock(disk->mgr, disk->lock);
}

void
arb_disk_remove(arb_uuid_mgr_t *mgr, const dl_desc_t *desc)
{
	arb_disk_inst_t *disk = arb_disk_open_impl(mgr, desc, 0);
	if (disk) {
		arb_disk_remove_dli(&disk->fm, desc);
		arb_disk_close(disk);
	}
}

static boolean_t
find_ver_in_dli(arb_disk_dli_t *dli, const dl_ver_t *ver, uint16_t *ndx)
{
	*ndx = 0;
	for (uint16_t i = 0; i < ARRAY_SIZE(dli->version); i++) {
		if (dli->version[i].detail.version == ver->detail.version) {
			*ndx = i;
			return (B_TRUE);
		} else if (dli->version[i].detail.version <
		    dli->version[*ndx].detail.version)
			*ndx = i;
	}
	return (B_FALSE);
}

void
arb_disk_sel_ms(arb_disk_inst_t *disk, const dl_ver_t *req, dl_ver_t *rsp)
{
	uint16_t ndx;
	if (find_ver_in_dli(disk->dli, req, &ndx)) {
		*rsp = disk->dli->version[ndx];
		return;
	}

	dl_ver_t sel = *req;
	sel.detail.state = disk->desc->local_hostid < disk->desc->remote_hostid ?
	    DL_STATE_MS_LITTLE : DL_STATE_MS_BIG;
	disk->dli->version[ndx] = sel;

	*rsp = file_map_sync(&disk->fm) == 0 ? sel : *req;
}

void
arb_disk_query_ms(arb_disk_inst_t *disk, const dl_ver_t *req, dl_ver_t *rsp)
{
	uint16_t ndx;
	if (find_ver_in_dli(disk->dli, req, &ndx))
		*rsp = disk->dli->version[ndx];
	else
		memset(rsp, 0, sizeof(*rsp));
}

static arb_uuid_mgr_t arb_uuid_mgr;

int
arb_disk_init(void)
{
	if (shell("mkdir -p %s", ARBITRARY_WORKSPACE))
		return (-1);

	return (arb_uuid_mgr_init(&arb_uuid_mgr));
}

void
arb_disk_fini(void)
{
	arb_uuid_mgr_fini(&arb_uuid_mgr);
}

static inline void
arb_select_ms_impl(const dl_desc_t *desc,
    const dl_ver_t *orig_ver, dl_ver_t *new_ver,
    void (*op)(arb_disk_inst_t *, const dl_ver_t *, dl_ver_t *))
{
	arb_uuid_mgr_t *mgr = &arb_uuid_mgr;
	arb_disk_inst_t *disk = arb_disk_open(mgr, desc);
	VERIFY(disk);

	op(disk, orig_ver, new_ver);
	arb_disk_close(disk);
}

void
arb_select_master_slave(const dl_desc_t *desc,
    const dl_ver_t *orig_ver, dl_ver_t *new_ver)
{
	arb_select_ms_impl(desc, orig_ver, new_ver, arb_disk_sel_ms);
}

void
arb_query_master_slave(const dl_desc_t *desc,
    const dl_ver_t *orig_ver, dl_ver_t *new_ver)
{
	arb_select_ms_impl(desc, orig_ver, new_ver, arb_disk_query_ms);
}

void arb_remove_duallive(const dl_desc_t *desc)
{
	arb_disk_remove(&arb_uuid_mgr, desc);
}
