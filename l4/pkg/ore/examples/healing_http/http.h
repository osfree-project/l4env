#ifndef HTTP_H
#define HTTP_H

#define HTTP_READY  0
#define HTTP_GET    1
#define HTTP_ABORT  2

struct http_state{
    unsigned char state;
    unsigned char poll_count;
};

#endif
