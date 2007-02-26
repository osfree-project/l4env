#ifndef LIB_SOCKET_LINUX_H
#define LIB_SOCKET_LINUX_H

#include <linux/socket.h>

/*** INITIALIZATION ***/
int libsocket_linux_init(void);

/*** SOCKET INTERFACE ***/
int accept(int, struct sockaddr *, int *);
int bind(int, struct sockaddr *, int);
int connect(int, struct sockaddr *, int);
int listen(int, int);
int recvmsg(int, struct msghdr *, int);
int recvfrom(int, void *, size_t, int, struct sockaddr *, int *);
int recv(int, void *, size_t, int);
int sendto(int, void *, size_t, int, struct sockaddr *, int);
int sendmsg(int, const struct msghdr *msg, int);
int send(int, void *, size_t, int);
int shutdown(int, int);
int socket(int, int, int);
int setsockopt (int, int, int, const void *, int);
int getsockname(int, struct sockaddr *, int *);
int socket_ioctl(int, unsigned int, void *);
int socket_read(int, char *, size_t);
int socket_write(int, const char *, size_t);
int socket_close(int);

/*** UTILITY INTERFACE ***/
int socket_set_private_data(int, void *);
void * socket_get_private_data(int);

#endif

