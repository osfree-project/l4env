/****************************************************************************/
/*                                                                          */
/* HMAC.H 03 Apr 2003                                                       */
/*                                                                          */
/* This file is copyright 2003 IBM. See "License" for details               */
/****************************************************************************/
#ifndef HMAC_H
#define HMAC_H

int calcparamdigest(unsigned char *paramdigest,...);
int authhmac(unsigned char *digest, unsigned char *key,
             unsigned int keylen, unsigned char *h1, unsigned char *h2,
             unsigned char h3, ...);
int checkhmac(unsigned char *buffer, unsigned long command,
              unsigned char *ononce, unsigned char *key, int data_len, int data_skip);
int rawhmac(unsigned char *digest, unsigned char *key,
            unsigned int keylen, ...);

void sha1(unsigned char *input, int len, unsigned char *output);

#endif
