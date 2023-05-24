#ifndef _XUTILS_SIMPLE_SOCK_H
#define _XUTILS_SIMPLE_SOCK_H

#ifdef __cplusplus
extern "C" {
#endif

int sock_start_server(int port,
    void (*handler)(int sockfd, void *ctx), void *ctx);
int sock_connect_server(const char *hostname, int port);

int sock_send(int sockfd, const void *buf, int size);
int sock_recv(int sockfd, void **buf, int *size);
void sock_free_buf(void *buf, int size);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // _XUTILS_SIMPLE_SOCK_H
