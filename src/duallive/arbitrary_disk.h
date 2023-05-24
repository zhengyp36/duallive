#ifndef _ARBITRARY_DISK_H
#define _ARBITRARY_DISK_H

#include "duallive_impl.h"

#ifdef __cplusplus
extern "C" {
#endif

int arb_disk_init(void);
void arb_disk_fini(void);
void arb_select_master_slave(const dl_desc_t *, const dl_ver_t *, dl_ver_t *);
void arb_query_master_slave(const dl_desc_t *, const dl_ver_t *, dl_ver_t *);
void arb_remove_duallive(const dl_desc_t *);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // _ARBITRARY_DISK_H
