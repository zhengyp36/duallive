#ifndef _ARBITRARY_MSG_H
#define _ARBITRARY_MSG_H

#include <sys/kmem.h>
#include <xutils/cvector.h>
#include "duallive_impl.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 *  Arbitrary message format:
 *
 *  ARB_REQ_SEL_MS:
 *    arb_msg_head_t : 8bytes
 *    arb_dli_t[N]   : N*40bytes, where N=arb_msg_head_t.dl_num
 *    dl_ver_t[N]    : N*8bytes
 *    str[X]         : X-bytes, where X is length of all of strings
 *
 *  ARB_RSP_SEL_MS:
 *    arb_msg_head_t : 8bytes
 *    dl_ver_t[N]    : N*8bytes, where N=arb_msg_head_t.dl_num
 *
 *  ARB_REQ_RM_DLI:
 *    arb_msg_head_t : 8bytes
 *    arb_dli_t[N]   : N*40bytes, where N=arb_msg_head_t.dl_num
 *    str[X]         : X-bytes, where X is length of all of strings
 *
 *  ARB_RSP_RM_DLI:
 *    arb_msg_head_t : 8bytes
 */

enum {
	ARB_REQ_SEL_MS,	// select master-slave
	ARB_RSP_SEL_MS,
	ARB_REQ_RM_DLI,	// remove duallive info
	ARB_RSP_RM_DLI,
};

typedef struct arbitrary_msg_head {
	uint16_t	msg_type;	// ARB_{REQ,RSP}_{SEL_MS,RM}
	uint16_t	dl_num;		// number of duallive-pairs
	uint32_t	msg_len;
} arb_msg_head_t;

typedef struct arbitrary_duallive_info {
	dl_uuid_t	uuid;
	uint32_t	local_hostid;
	uint32_t	remote_hostid;
	uint32_t	local_dataset;
	uint32_t	remote_dataset;
	uint32_t	arbitrary_ip;
	uint32_t	arbitrary_port;
} arb_dli_t;

typedef struct arbitrary_msg_encoder {
	uint16_t	msg_type;
	uint16_t	dl_num;
	uint32_t	arbitrary_ip;
	vec_t		v_dli;
	vec_t		v_ver;
	strvec_t	v_str;
} arb_msg_encoder_t;

typedef struct arbitrary_msg_decoder {
	arb_msg_head_t *	msg_head;
	arb_dli_t *		dl_info;
	dl_ver_t *		dl_ver;
	const char *		str;
} arb_msg_decoder_t;

void arb_msg_encoder_init(arb_msg_encoder_t *, uint16_t msg_type);
void arb_msg_encoder_add(arb_msg_encoder_t *, int argc, ...);
arb_msg_head_t * arb_msg_encoder_output(arb_msg_encoder_t *);
void arb_msg_encoder_fini(arb_msg_encoder_t *);

int arb_msg_decoder_init(arb_msg_decoder_t *, arb_msg_head_t *, size_t msg_len);
int arb_msg_decoder_get_dl_num(arb_msg_decoder_t *);
void arb_msg_decoder_get(arb_msg_decoder_t *, int dl_index, int argc, ...);

static inline void
arb_msg_free(arb_msg_head_t *msg_head)
{
	kmem_free(msg_head, msg_head->msg_len);
}

#define ARB_ENC_INIT(encoder, msg_type)					\
	arb_msg_encoder_init(encoder, msg_type)

#define ARB_ENC_REQ_SEL_MS(encoder, dl_desc, dl_ver)			\
	arb_msg_encoder_add(encoder, 2, dl_desc, dl_ver)

#define ARB_ENC_RSP_SEL_MS(encoder, dl_ver)				\
	arb_msg_encoder_add(encoder, 1, dl_ver)

#define ARB_ENC_REQ_RM_DLI(encoder, dl_desc)				\
	arb_msg_encoder_add(encoder, 1, dl_desc)

#define ARB_ENC_RSP_RM_DLI(encoder, dl_num)				\
	arb_msg_encoder_add(encoder, 1, dl_num)

#define ARB_ENC_OUTPUT(encoder)						\
	arb_msg_encoder_output(encoder)

#define ARB_ENC_FINI(encoder)						\
	arb_msg_encoder_fini(encoder)

#define ARB_DEC_INIT(decoder, msg_head)					\
	arb_msg_decoder_init(decoder, msg_head)

#define ARB_DEC_DL_NUM(decoder)						\
	arb_msg_decoder_get_dl_num(decoder)

#define ARB_DEC_REQ_SEL_MS(decoder, dl_index, dl_desc, dl_ver)		\
	arb_msg_decoder_get(decoder, dl_index, 2, dl_desc, dl_ver)

#define ARB_DEC_RSP_SEL_MS(decoder, dl_index, dl_ver)			\
	arb_msg_decoder_get(decoder, dl_index, 1, dl_ver)

#define ARB_DEC_REQ_RM_DLI(decoder, dl_index, dl_info)			\
	arb_msg_decoder_get(decoder, dl_index, 1, dl_info)

#define ARB_DEC_FINI(decoder) (void)(decoder)

#ifdef __cplusplus
} // extern "C"
#endif

#endif // _ARBITRARY_MSG_H
