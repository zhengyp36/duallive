#ifndef _DUALLIVE_IMPL_H
#define _DUALLIVE_IMPL_H

#ifdef __cplusplus
extern "C" {
#endif

#define DL_VER_TYPE_BITS 0x1
#define DL_VER_TYPE_MASK 0x1

typedef enum duallive_type {
	DL_TYPE_AA,
	DL_TYPE_AP,
} dl_type_t;

#define DL_VER_STATE_BITS 0x2
#define DL_VER_STATE_MASK 0x3

typedef enum duallive_state {
	DL_STATE_NORMAL,
	DL_STATE_LINKDOWN,
	DL_STATE_MS_LITTLE, // MS abbr. master_slave
	DL_STATE_MS_BIG,
} dl_state_t;

#define DL_VER_HOSTID_BITS 0x1
#define DL_VER_HOSTID_MASK 0x1

typedef enum duallive_hostid {
	DL_HOSTID_LITTLE,
	DL_HOSTID_BIG,
} dl_hostid_t;

#define DL_VER_VERSION_BITS						\
	(sizeof(uint64_t) * 8 -						\
	    DL_VER_TYPE_BITS - DL_VER_STATE_BITS - DL_VER_HOSTID_BITS)
#define DL_VER_VERSION_MASK ((1ULL << DL_VER_VERSION_BITS) - 1)

typedef union duallive_version {
	struct {
		uint64_t version : DL_VER_VERSION_BITS;
		uint64_t master  : DL_VER_HOSTID_BITS;	// dl_hostid_t
		uint64_t state   : DL_VER_STATE_BITS;	// dl_state_t
		uint64_t type    : DL_VER_TYPE_BITS;	// dl_type_t
	} detail;
	uint64_t version;
} dl_ver_t;

typedef struct duallive_uuid {
	uint64_t uuid[2];
} dl_uuid_t;

typedef struct duallive_desc {
	dl_uuid_t	uuid;
	uint32_t	local_hostid;
	uint32_t	remote_hostid;
	const char *	local_dataset;
	const char *	remote_dataset;
	const char *	arbitrary_ip;
	uint32_t	arbitrary_port;
} dl_desc_t;

#ifdef __cplusplus
} // extern "C"
#endif

#endif // _DUALLIVE_IMPL_H
