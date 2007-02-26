#ifndef __L4QWS_KEY_H
#define __L4QWS_KEY_H

#define L4QWS_INVALID_KEY -1

typedef int l4qws_key_t;

#ifdef __cplusplus
extern "C" {
#endif

extern l4qws_key_t l4qws_key(const char *fn, char c);

#ifdef __cplusplus
}
#endif

#endif /* __L4QWS_KEY_H */

