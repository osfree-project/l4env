#ifndef __MY_SOCK_H
#define __MY_SOCK_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define ERR_EXIT(str) \
	do {                \
		printf("(%d)", errno); \
		perror(str);      \
		return 1;         \
	} while (0)

#endif

