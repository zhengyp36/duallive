#include <stdarg.h>
#include <sys/debug.h>
#include "arbitrary_msg.h"

void
arb_msg_encoder_init(arb_msg_encoder_t *enc, uint16_t msg_type)
{
	enc->msg_type = msg_type;
	enc->dl_num = 0;

	switch (msg_type) {
		case ARB_REQ_SEL_MS:
			enc->arbitrary_ip = 0;
			vec_init(&enc->v_dli, sizeof(arb_dli_t));
			vec_init(&enc->v_ver, sizeof(dl_ver_t));
			strvec_init(&enc->v_str);
			break;

		case ARB_RSP_SEL_MS:
			vec_init(&enc->v_ver, sizeof(dl_ver_t));
			break;

		case ARB_REQ_RM_DLI:
			enc->arbitrary_ip = 0;
			vec_init(&enc->v_dli, sizeof(arb_dli_t));
			strvec_init(&enc->v_str);
			break;

		case ARB_RSP_RM_DLI:
			break;

		default:
			break;
	}
}

static inline uint32_t
encode_arb_ip(arb_msg_encoder_t *enc, dl_desc_t *desc)
{
	return (enc->dl_num > 0 ? enc->arbitrary_ip :
	    (enc->arbitrary_ip = strvec_add(&enc->v_str, desc->arbitrary_ip)));
}

static void
encode_dl_info(arb_msg_encoder_t *enc, dl_desc_t *desc)
{
	uint32_t arb_ip = encode_arb_ip(enc, desc);
	uint32_t lds = strvec_add(&enc->v_str, desc->local_dataset);
	uint32_t rds = strvec_add(&enc->v_str, desc->remote_dataset);

	arb_dli_t dli = {
		.uuid           = desc->uuid,
		.local_hostid   = desc->local_hostid,
		.remote_hostid  = desc->remote_hostid,
		.local_dataset  = lds,
		.remote_dataset = rds,
		.arbitrary_ip   = arb_ip,
		.arbitrary_port = desc->arbitrary_port,
	};

	vec_add(&enc->v_dli, &dli);
}

void
arb_msg_encoder_add(arb_msg_encoder_t *enc, int argc, ...)
{
	va_list ap;
	va_start(ap, argc);

	switch (enc->msg_type) {
		case ARB_REQ_SEL_MS:
			VERIFY(argc == 2);
			encode_dl_info(enc, va_arg(ap, dl_desc_t *));
			vec_add(&enc->v_ver, va_arg(ap, dl_ver_t *));
			enc->dl_num++;
			break;

		case ARB_RSP_SEL_MS:
			VERIFY(argc == 1);
			vec_add(&enc->v_ver, va_arg(ap, dl_ver_t *));
			enc->dl_num++;
			break;

		case ARB_REQ_RM_DLI:
			VERIFY(argc == 1);
			encode_dl_info(enc, va_arg(ap, dl_desc_t *));
			enc->dl_num++;
			break;

		case ARB_RSP_RM_DLI:
			VERIFY(argc == 1);
			enc->dl_num += va_arg(ap, uint32_t);
			break;

		default:
			break;
	}

	va_end(ap);
}

static inline arb_msg_head_t *
msg_alloc(arb_msg_encoder_t *enc, uint32_t msg_len)
{
	arb_msg_head_t *msg = kmem_alloc(msg_len, KM_SLEEP);
	msg->msg_type = enc->msg_type;
	msg->dl_num = enc->dl_num;
	msg->msg_len = msg_len;
	return (msg);
}

static inline void
encode_buf(void **pbuf, void *content, size_t size)
{
	memcpy(*pbuf, content, size);
	*pbuf += size;
}

static arb_msg_head_t *
encode_req_sel_ms(arb_msg_encoder_t *enc)
{
	uint32_t msg_len = sizeof(arb_msg_head_t);
	VERIFY(enc->v_dli.pos == sizeof(arb_dli_t) * enc->dl_num);
	VERIFY(enc->v_ver.pos == sizeof(dl_ver_t) * enc->dl_num);
	msg_len += enc->v_dli.pos;
	msg_len += enc->v_ver.pos;
	msg_len += enc->v_str.pos;

	arb_msg_head_t *msg = msg_alloc(enc, msg_len);
	void *buf = &msg[1];
	encode_buf(&buf, enc->v_dli.buf, enc->v_dli.pos);
	encode_buf(&buf, enc->v_ver.buf, enc->v_ver.pos);
	encode_buf(&buf, enc->v_str.str, enc->v_str.pos);

	return (msg);
}

static inline int
str_valid(uint32_t str_pos, uint32_t str_pos_limit)
{
	return (str_pos > 0 && str_pos < str_pos_limit);
}

static int
check_dli_str(arb_dli_t *dli, uint16_t dl_num, uint32_t str_pos_limit)
{
	for (uint16_t i = 0; i < dl_num; i++) {
		if (!str_valid(dli[i].arbitrary_ip, str_pos_limit))
			return (-1);
		if (!str_valid(dli[i].local_dataset, str_pos_limit))
			return (-1);
		if (!str_valid(dli[i].remote_dataset, str_pos_limit))
			return (-1);
	}
	return (0);
}

static int
check_msg_req_sel_ms(arb_msg_head_t *msg)
{
	size_t size = sizeof(*msg);
	size += sizeof(arb_dli_t) * msg->dl_num;
	size += sizeof(dl_ver_t) * msg->dl_num;
	if (msg->msg_len <= size) {
		return (-1);
	}

	arb_dli_t *dli = (void*)&msg[1];
	return (check_dli_str(dli, msg->dl_num, msg->msg_len - size));
}

static arb_msg_head_t *
encode_rsp_sel_ms(arb_msg_encoder_t *enc)
{
	uint32_t msg_len = sizeof(arb_msg_head_t);
	VERIFY(enc->v_ver.pos == sizeof(dl_ver_t) * enc->dl_num);
	msg_len += enc->v_ver.pos;

	arb_msg_head_t *msg = msg_alloc(enc, msg_len);
	void *buf = &msg[1];
	encode_buf(&buf, enc->v_ver.buf, enc->v_ver.pos);

	return (msg);
}

static inline int
check_msg_rsp_sel_ms(arb_msg_head_t *msg)
{
	uint32_t size = sizeof(*msg) + sizeof(dl_ver_t) * msg->dl_num;
	return (msg->msg_len == size ? 0 : -1);
}

static arb_msg_head_t *
encode_req_rm_dli(arb_msg_encoder_t *enc)
{
	uint32_t msg_len = sizeof(arb_msg_head_t);
	VERIFY(enc->v_dli.pos == sizeof(arb_dli_t) * enc->dl_num);
	msg_len += enc->v_dli.pos;
	msg_len += enc->v_str.pos;

	arb_msg_head_t *msg = msg_alloc(enc, msg_len);
	void *buf = &msg[1];
	encode_buf(&buf, enc->v_dli.buf, enc->v_dli.pos);
	encode_buf(&buf, enc->v_str.str, enc->v_str.pos);

	return (msg);
}

static int
check_msg_req_rm_dli(arb_msg_head_t *msg)
{
	size_t size = sizeof(*msg) + sizeof(arb_dli_t) * msg->dl_num;
	if (msg->msg_len <= size) {
		return (-1);
	}

	arb_dli_t *dli = (void*)&msg[1];
	return (check_dli_str(dli, msg->dl_num, msg->msg_len - size));
}

static arb_msg_head_t *
encode_rsp_rm_dli(arb_msg_encoder_t *enc)
{
	return (msg_alloc(enc, sizeof(arb_msg_head_t)));
}

arb_msg_head_t *
arb_msg_encoder_output(arb_msg_encoder_t *enc)
{
	if (enc->dl_num == 0)
		return (NULL);

	switch (enc->msg_type) {
		case ARB_REQ_SEL_MS:
			return (encode_req_sel_ms(enc));

		case ARB_RSP_SEL_MS:
			return (encode_rsp_sel_ms(enc));

		case ARB_REQ_RM_DLI:
			return (encode_req_rm_dli(enc));

		case ARB_RSP_RM_DLI:
			return (encode_rsp_rm_dli(enc));

		default:
			return (NULL);
	}
}

void
arb_msg_encoder_fini(arb_msg_encoder_t *enc)
{
	switch (enc->msg_type) {
		case ARB_REQ_SEL_MS:
			vec_fini(&enc->v_dli);
			vec_fini(&enc->v_ver);
			strvec_fini(&enc->v_str);
			break;

		case ARB_RSP_SEL_MS:
			vec_fini(&enc->v_ver);
			break;

		case ARB_REQ_RM_DLI:
			vec_fini(&enc->v_dli);
			strvec_fini(&enc->v_str);
			break;

		case ARB_RSP_RM_DLI:
			break;

		default:
			break;
	}
}

static int
check_msg(arb_msg_head_t *msg, size_t size)
{
	if (size < sizeof(*msg) || size != msg->msg_len)
		return (-1);

	switch (msg->msg_type) {
		case ARB_REQ_SEL_MS:
			return (check_msg_req_sel_ms(msg));

		case ARB_RSP_SEL_MS:
			return (check_msg_rsp_sel_ms(msg));

		case ARB_REQ_RM_DLI:
			return (check_msg_req_rm_dli(msg));

		case ARB_RSP_RM_DLI:
			return (msg->msg_len == sizeof(*msg) ? 0 : -1);

		default:
			return (-1);
	}
}

static inline void *
decode_buf(void **pbuf, uint32_t size)
{
	void *buf = *pbuf;
	*pbuf += size;
	return (buf);
}

int
arb_msg_decoder_init(arb_msg_decoder_t *dec, arb_msg_head_t *msg, size_t size)
{
	if (check_msg(msg, size))
		return (-1);

	dec->msg_head = msg;
	void *pbuf = (void*)&msg[1];

	switch (msg->msg_type) {
		case ARB_REQ_SEL_MS:
			dec->dl_info = decode_buf(&pbuf,
			    sizeof(dec->dl_info[0]) * msg->dl_num);
			dec->dl_ver = decode_buf(&pbuf,
			    sizeof(dec->dl_ver[0]) * msg->dl_num);
			dec->str = pbuf;
			break;

		case ARB_RSP_SEL_MS:
			dec->dl_info = NULL;
			dec->dl_ver = pbuf;
			dec->str = NULL;
			break;

		case ARB_REQ_RM_DLI:
			dec->dl_info = decode_buf(&pbuf,
			    sizeof(dec->dl_info[0]) * msg->dl_num);
			dec->dl_ver = NULL;
			dec->str = pbuf;
			break;

		case ARB_RSP_RM_DLI:
			break;

		default:
			break;
	}

	return (0);
}

int
arb_msg_decoder_get_dl_num(arb_msg_decoder_t *dec)
{
	return (dec->msg_head->dl_num);
}

static void
decode_dl_info(arb_dli_t *dli, const char *str, dl_desc_t *desc)
{
	desc->uuid           = dli->uuid;
	desc->local_hostid   = dli->local_hostid;
	desc->remote_hostid  = dli->remote_hostid;
	desc->local_dataset  = &str[dli->local_dataset];
	desc->remote_dataset = &str[dli->remote_dataset];
	desc->arbitrary_ip   = &str[dli->arbitrary_ip];
	desc->arbitrary_port = dli->arbitrary_port;
}

void
arb_msg_decoder_get(arb_msg_decoder_t *dec, int dl_ndx, int argc, ...)
{
	VERIFY(dl_ndx >= 0 && dl_ndx < dec->msg_head->dl_num);

	va_list ap;
	va_start(ap, argc);

	switch (dec->msg_head->msg_type) {
		case ARB_REQ_SEL_MS:
			VERIFY(argc == 2);
			decode_dl_info(&dec->dl_info[dl_ndx], dec->str,
			    va_arg(ap, dl_desc_t *));
			*(va_arg(ap, dl_ver_t *)) = dec->dl_ver[dl_ndx];
			break;

		case ARB_RSP_SEL_MS:
			VERIFY(argc == 1);
			*(va_arg(ap, dl_ver_t *)) = dec->dl_ver[dl_ndx];
			break;

		case ARB_REQ_RM_DLI:
			VERIFY(argc == 1);
			decode_dl_info(&dec->dl_info[dl_ndx], dec->str,
			    va_arg(ap, dl_desc_t *));
			break;

		case ARB_RSP_RM_DLI:
			break;

		default:
			break;
	}

	va_end(ap);
}
