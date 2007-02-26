/* -*- c++ -*- */
/*
 * Copyright (C) 2004  Alexander Warg  <alexander.warg@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the nethub package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */
#ifndef IP_SEC_L4CRYPTO_H__
#define IP_SEC_L4CRYPTO_H__

#include "ip_sec.h"
#include <l4/cryptoserver/l4crypto.h>

namespace L4 {
  class BasicOStream;
}

class Crypt_l4_crypto : public Crypt_algo
{
public:
  Crypt_l4_crypto(char const *key, size_t key_len, long algo, long mode);
  virtual ~Crypt_l4_crypto();
  void encrypt(unsigned long &len, void *pay_load, void *out, char np);
  void decrypt(unsigned long &len, void *pay_load, void *out);
  char const *const name() const;
  void print(L4::BasicOStream &s) const;
private:
  // As we don't know if this module ist going to en or decrypt we have to 
  // Lazy-initialize the plugin. Therefore we need the following 3 entries.
  enum e_mode 
  { 
    INVALID, 
    INIT, 
    ENCRYPT, 
    DECRYPT
  };
  
  CRYPTO_STATUS open(long _mode);
  
  unsigned char key[MAX_KEY_LEN];
  unsigned char iv[MAX_IV_LEN];
  size_t key_len;
  enum e_mode status; // mode which we are using

  CRYPTO_HANDLE *ctx;
  int mode, algo, blklen;
};


class Auth_l4_crypto : public Auth_algo
{
public:

  Auth_l4_crypto(char const *key, size_t keylen, long algo);
  virtual ~Auth_l4_crypto();
  unsigned auth(unsigned long &len, void *pay_load, bool sign = false);
  char const *const name() const;
  void print(L4::BasicOStream &s) const;

private:
  long algo;
  CRYPTO_STATUS open(char const *_key, size_t _keylen, long _algo);
  CRYPTO_HANDLE *ctx;
  CRYPTO_HANDLE *ctxHash;
  size_t auth_len_trunc;	
};


#endif // IP_SEC_L4CRYPTO_H__

