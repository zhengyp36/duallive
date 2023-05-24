#include <stdio.h>
#include <sys/debug.h>
#include "arbitrary_msg.h"
#include "arbitrary_disk.h"
#include "arbitrary_disk_impl.h"

#define DL_DESC(uuid0, uuid1, lhost, rhost, lds, rds, arbip, arbport)	\
	{								\
		.uuid = {.uuid={uuid0, uuid1}},				\
		.local_hostid   = lhost,				\
		.remote_hostid  = rhost,				\
		.local_dataset  = lds,					\
		.remote_dataset = rds,					\
		.arbitrary_ip   = arbip,				\
		.arbitrary_port = arbport,				\
	}

#define DL_VER(_type, _state, _master, _version)			\
	{								\
		.detail = {						\
			.version = _version,				\
			.master  = _master,				\
			.state   = _state,				\
			.type    = _type				\
		},							\
	}

#define DL_AA_VER(state,version)					\
	DL_VER(DL_TYPE_AA, state, DL_HOSTID_LITTLE, version)

#define DL_AP_VER(state,master,version)					\
	DL_VER(DL_TYPE_AP, state, master, version)

static dl_desc_t dl_desc_A[] = {
	DL_DESC(
		0xAB,0x1,
		1,3,
		"poolA/lunA1","poolB/lunB1",
		"192.168.16.248",3060
	),
	DL_DESC(
		0xAB,0x2,
		1,3,
		"poolA/lunA2","poolB/lunB2",
		"192.168.16.248",3060
	),
	DL_DESC(
		0xAB,0x3,
		1,3,
		"poolA/lunA3","poolB/lunB3",
		"192.168.16.248",3060
	),
	DL_DESC(
		0xAB,0x4,
		1,3,
		"poolA/lunA4","poolB/lunB4",
		"192.168.16.248",3060
	),
	DL_DESC(
		0xAB,0x5,
		1,3,
		"poolA/lunA5","poolB/lunB5",
		"192.168.16.248",3060
	),
	DL_DESC(
		0xAB,0x6,
		1,3,
		"poolA/lunA6","poolB/lunB6",
		"192.168.16.248",3060
	),
	DL_DESC(
		0xAB,0x7,
		1,3,
		"poolA/lunA7","poolB/lunB7",
		"192.168.16.248",3060
	),
	DL_DESC(
		0xAB,0x8,
		1,3,
		"poolA/lunA8","poolB/lunB8",
		"192.168.16.248",3060
	),
	DL_DESC(
		0xAB,0x9,
		1,3,
		"poolA/lunA9","poolB/lunB9",
		"192.168.16.248",3060
	),
	DL_DESC(
		0xAB,0xA,
		1,3,
		"poolA/lunAA","poolB/lunBA",
		"192.168.16.248",3060
	),
	DL_DESC(
		0xAB,0xB,
		1,3,
		"poolA/lunAB","poolB/lunBB",
		"192.168.16.248",3060
	),
	DL_DESC(
		0xAB,0xC,
		1,3,
		"poolA/lunAC","poolB/lunBC",
		"192.168.16.248",3060
	),
};

static dl_desc_t dl_desc_B[] = {
	DL_DESC(
		0xAB,0x1,
		3,1,
		"poolB/lunB1","poolA/lunA1",
		"192.168.16.248",3060
	),
	DL_DESC(
		0xAB,0x2,
		3,1,
		"poolB/lunB2","poolA/lunA2",
		"192.168.16.248",3060
	),
	DL_DESC(
		0xAB,0x3,
		3,1,
		"poolB/lunB3","poolA/lunA3",
		"192.168.16.248",3060
	),
	DL_DESC(
		0xAB,0x4,
		3,1,
		"poolB/lunB4","poolA/lunA4",
		"192.168.16.248",3060
	),
	DL_DESC(
		0xAB,0x5,
		3,1,
		"poolB/lunB5","poolA/lunA5",
		"192.168.16.248",3060
	),
	DL_DESC(
		0xAB,0x6,
		3,1,
		"poolB/lunB6","poolA/lunA6",
		"192.168.16.248",3060
	),
	DL_DESC(
		0xAB,0x7,
		3,1,
		"poolB/lunB7","poolA/lunA7",
		"192.168.16.248",3060
	),
	DL_DESC(
		0xAB,0x8,
		3,1,
		"poolB/lunB8","poolA/lunA8",
		"192.168.16.248",3060
	),
	DL_DESC(
		0xAB,0x9,
		3,1,
		"poolB/lunB9","poolA/lunA9",
		"192.168.16.248",3060
	),
	DL_DESC(
		0xAB,0xA,
		3,1,
		"poolB/lunBA","poolA/lunAA",
		"192.168.16.248",3060
	),
	DL_DESC(
		0xAB,0xB,
		3,1,
		"poolB/lunBB","poolA/lunAB",
		"192.168.16.248",3060
	),
	DL_DESC(
		0xAB,0xC,
		3,1,
		"poolB/lunBC","poolA/lunAC",
		"192.168.16.248",3060
	),
};

static dl_ver_t dl_ver[] = {
	DL_AA_VER(DL_STATE_NORMAL,    1),
	DL_AA_VER(DL_STATE_LINKDOWN,  1),
	DL_AA_VER(DL_STATE_MS_LITTLE, 1),
	DL_AA_VER(DL_STATE_MS_BIG,    1),

	DL_AP_VER(DL_STATE_NORMAL,    DL_HOSTID_LITTLE, 2),
	DL_AP_VER(DL_STATE_LINKDOWN,  DL_HOSTID_LITTLE, 2),
	DL_AP_VER(DL_STATE_MS_LITTLE, DL_HOSTID_LITTLE, 2),
	DL_AP_VER(DL_STATE_MS_BIG,    DL_HOSTID_LITTLE, 2),
	DL_AP_VER(DL_STATE_NORMAL,    DL_HOSTID_BIG,    2),
	DL_AP_VER(DL_STATE_LINKDOWN,  DL_HOSTID_BIG,    2),
	DL_AP_VER(DL_STATE_MS_LITTLE, DL_HOSTID_BIG,    2),
	DL_AP_VER(DL_STATE_MS_BIG,    DL_HOSTID_BIG,    2),
};

#define SHOW_TITLE()							\
	do {								\
		printf("----------------------------------------\n");	\
		printf("<<< %s >>>\n", __FUNCTION__);			\
		printf("----------------------------------------\n");	\
	} while (0)

static inline const char *
dl_type_str(const dl_ver_t *dl)
{
	switch (dl->detail.type) {
		case DL_TYPE_AA: return "AA";
		case DL_TYPE_AP: return "AP";
		default:         return "TYPE-ERR";
	}
}

static inline const char *
dl_state_str(const dl_ver_t *dl)
{
	switch (dl->detail.state) {
		case DL_STATE_NORMAL:    return ".normal";
		case DL_STATE_LINKDOWN:  return ".linkdown";
		case DL_STATE_MS_LITTLE: return ".ms0";
		case DL_STATE_MS_BIG:    return ".ms1";
		default:                 return ".STATE-ERR";
	}
}

static inline const char *
dl_master_str(const dl_ver_t *dl)
{
	if (dl->detail.type == DL_TYPE_AA)
		return ("");

	switch (dl->detail.master) {
		case DL_HOSTID_LITTLE: return ".m0";
		case DL_HOSTID_BIG:    return ".m1";
		default:               return ".MASTER-ERR";
	}
}

#define DL_VER_FMT "%lx{%s%s%s.%lx}"
#define DL_VER_VAL(v)							\
	(v)->version, dl_type_str(v), dl_state_str(v), 			\
	dl_master_str(v), (uint64_t)(v)->detail.version

static void
arb_msg_sel_ms_on_single_site(const char *site,
    dl_desc_t *desc_arr, uint16_t dl_num)
{
	printf("### %s: Encode sel_ms req ...\n", site);
	arb_msg_encoder_t encoder;
	ARB_ENC_INIT(&encoder, ARB_REQ_SEL_MS);
	for (uint16_t i = 0; i < dl_num; i++)
		ARB_ENC_REQ_SEL_MS(&encoder, &desc_arr[i], &dl_ver[i]);
	arb_msg_head_t *msg = arb_msg_encoder_output(&encoder);
	ARB_ENC_FINI(&encoder);

	printf("### %s: Decode sel_ms req ...\n", site);
	arb_msg_decoder_t decoder;
	int ret = arb_msg_decoder_init(&decoder, msg, msg->msg_len);
	ASSERT(ret == 0);

	ret = arb_msg_decoder_get_dl_num(&decoder);
	ASSERT(ret == dl_num);
	printf(">>> dl_num = %d\n", ret);

	dl_desc_t desc;
	dl_ver_t dlver;
	for (uint16_t i = 0; i < dl_num; i++) {
		ARB_DEC_REQ_SEL_MS(&decoder, i, &desc, &dlver);
		printf(">>> REQ[%u]\n", i+1);
		printf("    uuid      : %lx-%lx\n",
		    desc.uuid.uuid[0], desc.uuid.uuid[1]);
		printf("    local-ds  : %u.%s\n",
		    desc.local_hostid, desc.local_dataset);
		printf("    remote-ds : %u.%s\n",
		    desc.remote_hostid, desc.remote_dataset);
		printf("    arbitrary : %s:%u\n",
		    desc.arbitrary_ip, desc.arbitrary_port);
		printf("    dl-version: "DL_VER_FMT"\n",
		    DL_VER_VAL(&dlver));
	}
	ARB_DEC_FINI(&decoder);

	arb_msg_free(msg);
}

static void
test_arb_msg_sel_ms(void)
{
	SHOW_TITLE();

	BUILD_BUG_ON(ARRAY_SIZE(dl_desc_A) != ARRAY_SIZE(dl_desc_B));
	BUILD_BUG_ON(ARRAY_SIZE(dl_desc_A) != ARRAY_SIZE(dl_ver));

	uint16_t dl_num = ARRAY_SIZE(dl_desc_A);
	arb_msg_sel_ms_on_single_site("SiteA", dl_desc_A, dl_num);
	arb_msg_sel_ms_on_single_site("SiteB", dl_desc_B, dl_num);
}

void
arbitrary_demo(int firstSelB)
{
	printf("This is an arbitrary demo.\n");
	test_arb_msg_sel_ms();

	printf("### Init arbitrary disk ...\n");
	VERIFY(!arb_disk_init());

	dl_ver_t verA, verB;

	dl_desc_t *first, *second;
	const char *info1, *info2;
	if (firstSelB) {
		first = dl_desc_B;
		second = dl_desc_A;
		info1 = "B->A";
		info2 = "A->B";
	} else {
		first = dl_desc_A;
		second = dl_desc_B;
		info1 = "A->B";
		info2 = "B->A";
	}

	arb_select_master_slave(&first[0], &dl_ver[0], &verB);
	arb_select_master_slave(&second[0], &dl_ver[0], &verA);
	printf("### SelMS[0][%s]: verA{"DL_VER_FMT"}, verB{"DL_VER_FMT"}\n",
	    info1, DL_VER_VAL(&verA), DL_VER_VAL(&verB));

	arb_select_master_slave(&second[1], &dl_ver[1], &verA);
	arb_select_master_slave(&first[1], &dl_ver[1], &verB);
	printf("### SelMS[1][%s]: verA{"DL_VER_FMT"}, verB{"DL_VER_FMT"}\n",
	    info2, DL_VER_VAL(&verA), DL_VER_VAL(&verB));

	printf("#### Fini arbitrary disk ...\n");
	arb_disk_fini();
}
