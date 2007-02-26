#ifndef HTTP_H
#define HTTP_H

#include <oredev.h>

void http_init(void);
void http_appcall(void);

#define UIP_APPCALL http_appcall

#define HTTP_READY  0
#define HTTP_GET    1
#define HTTP_ABORT  2

#define DEBUG_HTTP  0

struct http_state{
    unsigned char state;
    unsigned char poll_count;
};

#ifndef UIP_APPSTATE_SIZE
#define UIP_APPSTATE_SIZE   (sizeof(struct http_state))
#endif

extern struct http_state    *my_http_state;

#endif
