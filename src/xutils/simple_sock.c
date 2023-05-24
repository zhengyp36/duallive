#include <stdio.h>
#include <netdb.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <xutils/simple_sock.h>

#define MAX_CONNS 128

#define sock_log(fmt, ...) \
	printf(fmt"[%s,%d]\n", ##__VA_ARGS__, __FUNCTION__, __LINE__)

#define sock_loginfo(fmt, ...) sock_log("[INFO]"fmt, ##__VA_ARGS__)
#define sock_logwarn(fmt, ...) sock_log("[WARN]"fmt, ##__VA_ARGS__)
#define sock_logerr(fmt, ...) sock_log("[ERR]"fmt, ##__VA_ARGS__)

#define SIMPLE_SOCK_MSG_MAGIC 0x5351434B // SOCK

typedef struct msg_head {
	uint32_t magic;
	uint32_t length;
} msg_head_t;

typedef struct msg_tail {
	uint32_t length;
	uint32_t magic;
} msg_tail_t;

typedef struct sock_server_ctx {
	int listen_sock;
	int new_sock;
	void (*handler)(int sockfd, void *ctx);
	void *ctx;
} sock_server_ctx_t;

static void
handle_conn_impl(sock_server_ctx_t *ctx, int sync)
{
	const char *type = sync ? "Sync" : "Asyn";

	struct sockaddr_in sa;
	socklen_t len = sizeof(sa);
	if (!getpeername(ctx->new_sock, (struct sockaddr*)&sa, &len))
		sock_loginfo("%s handle connection from %s:%d",
		    type, inet_ntoa(sa.sin_addr), ntohs(sa.sin_port));
	else
		sock_logwarn("%s handle connection from %s:%s",
		    type, "<unknown-host>", "<unknown-port>");

	if (!sync)
		ctx->handler(ctx->new_sock, ctx->ctx);
	else
		sock_logwarn("Create thread error and ignore the connection");

	close(ctx->new_sock);
	free(ctx);
}

static void *
handle_conn_routine(void *arg)
{
	pthread_detach(pthread_self());
	handle_conn_impl(arg, 0);
	return (NULL);
}

static void
handle_conn(sock_server_ctx_t *ctx)
{
	sock_server_ctx_t *new_ctx = malloc(sizeof(*ctx));
	*new_ctx = *ctx;

	pthread_t thrd;
	int ret = pthread_create(&thrd, NULL, handle_conn_routine, new_ctx);
	if (ret)
		handle_conn_impl(new_ctx, 1);
}

static inline int
get_conn(int sockfd, int *new_sock)
{
	struct sockaddr_in cli_addr;
	socklen_t clilen = sizeof(cli_addr);

	*new_sock = accept(sockfd, (struct sockaddr*)&cli_addr, &clilen);
	if (*new_sock < 0)
		sock_logerr("Accept connections error %d", errno);

	return (*new_sock >= 0);
}

static void *
sock_server_routine(void *arg)
{
	sock_server_ctx_t *ctx = arg;
	pthread_detach(pthread_self());

	listen(ctx->listen_sock, MAX_CONNS);
	sock_loginfo("Start to accept connections...");

	while (get_conn(ctx->listen_sock, &ctx->new_sock))
		handle_conn(ctx);
	free(ctx);

	return (NULL);
}

int
sock_start_server(int port, void (*handler)(int sockfd, void *ctx), void *_ctx)
{
	sock_server_ctx_t *ctx = malloc(sizeof(*ctx));
	memset(ctx, 0, sizeof(*ctx));
	ctx->handler = handler;
	ctx->ctx = _ctx;

	ctx->listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (ctx->listen_sock < 0) {
		sock_logerr("Open socket error %d", errno);
		free(ctx);
		return (-1);
	}

	struct sockaddr_in serv_addr = {
		.sin_family = AF_INET,
		.sin_addr = {
			.s_addr = INADDR_ANY,
		},
		.sin_port = htons(port),
	};
	if (bind(ctx->listen_sock,
	    (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
		sock_logerr("Bind port %d error %d", port, errno);
		close(ctx->listen_sock);
		free(ctx);
		return (-1);
	}

	pthread_t thrd;
	if (pthread_create(&thrd, NULL, sock_server_routine, ctx)) {
		sock_logerr("Create thread for server error %d", errno);
		close(ctx->listen_sock);
		free(ctx);
		return (-1);
	}

	return (0);
}

int
sock_connect_server(const char *hostname, int port)
{
	struct hostent *server = gethostbyname(hostname);
	if (!server) {
		sock_logerr("Get host %s error %d", hostname, h_errno);
		return (-1);
	}

	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		sock_logerr("Open socket error %d", errno);
		return (-1);
	}

	struct sockaddr_in serv_addr;
	bzero(&serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy(server->h_addr, &serv_addr.sin_addr.s_addr, server->h_length);
	serv_addr.sin_port = htons(port);

	if (connect(sockfd,
	    (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
		sock_logerr("Connect %s %d error %d", hostname, port, errno);
		close(sockfd);
		return (-1);
	}

	return (sockfd);
}

static int
wr_buf(int sockfd, const void *buf, int size)
{
	while (size > 0) {
		int rc = write(sockfd, buf, size);
		if (rc <= 0)
			return (-1);
		buf += rc;
		size -= rc;
	}

	return (0);
}

int
sock_send(int sockfd, const void *buf, int size)
{
	msg_head_t head = {
		.magic = SIMPLE_SOCK_MSG_MAGIC,
		.length = size,
	};

	msg_tail_t tail = {
		.magic = SIMPLE_SOCK_MSG_MAGIC,
		.length = size,
	};

	int rc = wr_buf(sockfd, &head, sizeof(head));
	if (rc)
		return (-1);

	rc = wr_buf(sockfd, buf, size);
	if (rc)
		return (-1);

	rc = wr_buf(sockfd, &tail, sizeof(tail));
	if (rc)
		return (-1);

	return (0);
}

static int
rd_buf(int sockfd, void *buf, int size)
{
	while (size > 0) {
		int rc = read(sockfd, buf, size);
		if (rc <= 0)
			return (-1);
		buf += rc;
		size -= rc;
	}
	return (0);
}

int
sock_recv(int sockfd, void **buf, int *size)
{
	msg_head_t head;
	int rc = rd_buf(sockfd, &head, sizeof(head));
	if (rc || head.magic != SIMPLE_SOCK_MSG_MAGIC)
		return (-1);

	*buf = malloc(head.length);
	rc = rd_buf(sockfd, *buf, head.length);
	if (rc)
		return (-1);

	msg_tail_t tail;
	rc = rd_buf(sockfd, &tail, sizeof(tail));
	if (rc || tail.magic != SIMPLE_SOCK_MSG_MAGIC) {
		free(*buf);
		*buf = NULL;
		return (-1);
	}

	return (0);
}

void
sock_free_buf(void *buf, int size)
{
	free(buf);
}
