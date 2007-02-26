/*
 * Copyright (C) 2004  Alexander Warg  <alexander.warg@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the nethub package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */
#include "ip_sec.h"
#include "ip_sec_l4crypto.h"
#include "ip.h"

#include <l4/cxx/iostream.h>


void memcpy(void *i, void *o, unsigned len)
{
  char *ii = (char *)i;
  char *oo = (char *)o;
  while (len--)
    *ii++ = *oo++;
}

static int memcmp(void *i, void const *o, unsigned len)
{
  char const *ii = (char const *)i;
  char const *oo = (char const *)o;
  while (len && *(ii++) == *(oo++))
    --len;

  if (len)
    return 1;
  else
    return 0;
}


Auth_l4_crypto::Auth_l4_crypto(char const *_key, size_t _keylen, long _algo)
: algo(_algo), auth_len_trunc(96/8)
{
  open(_key, _keylen, _algo);
}

Auth_l4_crypto::~Auth_l4_crypto()
{
  l4CryptoClose(ctx);
  l4CryptoClose(ctxHash);
  free(ctx);
  free(ctxHash);
  ctx = 0;
  ctxHash = 0;
}

char const *const Auth_l4_crypto::name() const
{
  CryptoAlgInfo info;
  if (CRYPTO_OK != l4CryptoLibGetPluginInfo(&info, algo, CRYPTO_TYPE_HASH, 0)) 
    return "l4crypt-INVALID";

  return info.name;

}

void Auth_l4_crypto::print( L4::BasicOStream &s ) const
{
  CryptoAlgInfo info;
  if (CRYPTO_OK != l4CryptoLibGetPluginInfo(&info, algo, CRYPTO_TYPE_HASH, 0))
    s << "l4crypt-INVALID";

  s << info.name << "-HMAC";
}

CRYPTO_STATUS Auth_l4_crypto::open(char const *_key, size_t _keylen,
    long _algo ) 
{
  CRYPTO_STATUS ret = CRYPTO_OK;
  size_t ctxsize;

  ctxsize = l4CryptoGetCTXSize( _algo, CRYPTO_TYPE_HASH, 0 );
  if (ctxsize < sizeof(CRYPTO_HANDLE)) 
    {
      L4::cout << "Error opening Device!\n No such plugin.\n";
      goto errA2;
    }
  ctxHash = (CRYPTO_HANDLE *)malloc(ctxsize);

  ctxsize = l4CryptoGetCTXSize(0, CRYPTO_TYPE_HMAC, 0);
  if (ctxsize < sizeof(CRYPTO_HANDLE)) 
    {
      L4::cout << "Error opening Device!\n No such plugin.\n";
      goto errA2;
    }
  ctx = (CRYPTO_HANDLE *) malloc( ctxsize );

  if (! ctx || ! ctxHash) goto errA2;
  // open hash plugin!
  
  ret = l4CryptoOpen(ctxHash, _algo, CRYPTO_TYPE_HASH, 0 , 0);
  if (ret != CRYPTO_OK) 
    {
      L4::cout << "Error opening Hash plugin!\n" 
	       << l4CryptoErrSring(ctx,ret) << "\n";
      goto errA1;
    }
  
  ret = l4CryptoOpen(ctx, 0, CRYPTO_TYPE_HMAC, 0 , 0);
  if (ret != CRYPTO_OK) 
    {
      L4::cout << "Error opening HMAC plugin!\n" 
	       << l4CryptoErrSring(ctx,ret) << "\n";
      goto errA1;
    }
  
  ret = l4CryptoControl(ctx, CRYPTO_CTL_SET_CTX, ctxHash);
  if (ret != CRYPTO_OK) 
    {
      L4::cout << "Error connecting HASH plugin to HMAC plugin!\n" 
	       << l4CryptoErrSring(ctx,ret) << "\n";
      goto errA1;
    }
  
  ret = l4CryptoControl(ctx, CRYPTO_CTL_SET_KEY, _key, _keylen);
  if (ret != CRYPTO_OK) 
    {
      L4::cout << "Error setting key!\n";
      goto errA1;
    }
  L4::cout << "Open Hash Hmac - done \n";

  return CRYPTO_OK;
errA1:
  free(ctx);
  free(ctxHash);
  ctx = 0;
  ctxHash = 0;
errA2:
  return ret;
}

unsigned Auth_l4_crypto::auth(unsigned long &len, void *pay_load, 
                              bool sign )
{ 
  char md[MAX_HASH_LEN];
  size_t outlen = MAX_HASH_LEN;
  CRYPTO_STATUS ret = CRYPTO_OK;

  if (! ctx) return 0;

  if (!sign)
    {
      if (len < auth_len_trunc)
	return 0;
      else
	len -= auth_len_trunc; // subtract authenticator len
    }

  ret = l4CryptoTransform(ctx, (const char*)pay_load, len, md, &outlen,
      (unsigned char *)NULL);
  if (ret != CRYPTO_OK) 
    {
      L4::cout << "Error during Transformation (update)!\n";
      len = 0;
      return 0;
    }

  ret = l4CryptoReOpen(ctx);
  if (ret != CRYPTO_OK) 
    {
      L4::cout << "Error during Reopen!\n";
      len = 0;
      return 0;
    }

  if (!sign)
    {
      if (memcmp( md, ((u8*)pay_load)+len, auth_len_trunc ) == 0)
	return 1;
    }
  else
    {
      memcpy(((unsigned char *)pay_load)+len, md, auth_len_trunc);		
      len += auth_len_trunc;
    }
  return 0;
}


Crypt_l4_crypto::Crypt_l4_crypto(char const *_key, size_t _key_len,
                                 long _algo, long _mode) 
: Crypt_algo(8)
{
  CryptoAlgInfo algoInfo;
  CRYPTO_STATUS ret; 

  algo = _algo;
  mode = _mode;
  // we shall use CRYPTO_TYPE_ENCRYPT or CRYPTO_TYPE_DECRYPT in the next step!
  ret = l4CryptoGetPluginInfo(&algoInfo, algo, CRYPTO_TYPE_ENCRYPT ,mode); 
  ctx = (CRYPTO_HANDLE *) NULL;
  status = INVALID;
  if (ret != CRYPTO_OK) goto err1;

  if (! _key_len) 
    _key_len = algoInfo.keylen;
  key_len = _key_len;

  if (key_len > MAX_KEY_LEN || key < 0) goto err1;
  memcpy(key, const_cast<char*>(_key), key_len);
  status = INIT;

  return;
err1:
  // invalid plugin... we will never be able to encrypt/decrypt
  status = INVALID;
  return;
}

Crypt_l4_crypto::~Crypt_l4_crypto() 
{
  unsigned char * _key = key;

  while(key_len--)
    *_key++ = (char)0;

}

char const *const Crypt_l4_crypto::name() const
{
  CryptoAlgInfo info;
  if (CRYPTO_OK != l4CryptoLibGetPluginInfo(&info, algo, 
	CRYPTO_TYPE_ENCRYPT, mode))
    return "l4crypt-INVALID";

  return info.name;
}

void Crypt_l4_crypto::print( L4::BasicOStream &s ) const
{
  CryptoAlgInfo info;
  if (CRYPTO_OK != l4CryptoLibGetPluginInfo(&info, algo, 
	CRYPTO_TYPE_ENCRYPT, mode)) 
    s << "l4crypt-INVALID";

  s << info.name;
}

CRYPTO_STATUS Crypt_l4_crypto::open( long type )
{
  CRYPTO_STATUS ret;
  size_t ctxsize;

  //lgo = 3;
  //ode = 2;

  L4::cout << (unsigned int)algo <<" ";
  L4::cout << (unsigned int)type <<" ";
  L4::cout << (unsigned int)mode << "\n";
  ctxsize = l4CryptoGetCTXSize( algo, type, mode );
  if (ctxsize < sizeof(CRYPTO_HANDLE)) 
    {
      L4::cout << "Error opening Device!\n No such plugin.\n";
      return CRYPTO_ERR_GENERAL;
      status = INVALID;
    }
  L4::cout << (unsigned int)algo;
  L4::cout << (unsigned int)type;
  L4::cout << (unsigned int)mode << "\n";
  L4::cout << (unsigned int)ctxsize;

  // open the libary in the desired mode and sk it to chunk data into pieces!
  ctx = (CRYPTO_HANDLE *) malloc( ctxsize );
  
  ret = l4CryptoOpen(ctx, algo, type, mode, 0);
  if (ret != CRYPTO_OK) 
    {
      L4::cout << "Error opening Device!\n"  << l4CryptoErrSring(ctx,ret) << "\n";
      L4::cout << (unsigned int)algo;
      L4::cout << (unsigned int)type;
      L4::cout << (unsigned int)mode << "\n";
      return ret;
    }
  
  ret = l4CryptoControl(ctx, CRYPTO_CTL_GET_BLOCK_LEN, &blklen);
  ret = l4CryptoControl(ctx, CRYPTO_CTL_SET_KEY, key, key_len);
  if (ret != CRYPTO_OK) 
    {
      L4::cout << "Error setting key!\n" << l4CryptoErrSring(ctx,ret) << "\n";
      return ret;
    }

  switch (type) 
    {
    case CRYPTO_TYPE_ENCRYPT: 
      status = ENCRYPT;
      break;
    case CRYPTO_TYPE_DECRYPT: 
      status = DECRYPT;
      break;
    default:
      status = INVALID;
      return CRYPTO_ERR_GENERAL;
    }

  return CRYPTO_OK;
}


void Crypt_l4_crypto::encrypt(unsigned long &len, void *pay_load,
                              void *out, char np )
{
  CRYPTO_STATUS ret;
  size_t outlen;

  if ( status == INIT) open( CRYPTO_TYPE_ENCRYPT );
  if ( status != ENCRYPT) 
    {
      // if we still don`t a ctx something is very wrong!!!!!!
      len = 0;
      return;
    }
  char *data = (char*)(((u8*)out) + blklen );
  char trailer[2 * blklen];
  Ip_esp_packet::Esp_t *t;
  unsigned x_blks;
  unsigned blks    = len / blklen;
  unsigned overlap = len - (blks * blklen);

  char *i = (char*)pay_load + blks * blklen;
  char *o = (char*)trailer;


  for (unsigned x=overlap; x; --x)
    *(o++) = *(i++);

  if ((blklen-overlap) < sizeof(Ip_esp_packet::Esp_t))
    {
      x_blks = 2;
      t = (Ip_esp_packet::Esp_t*)(trailer + 2 * blklen) - 1;
    }
  else
    {
      x_blks = 1;
      t = (Ip_esp_packet::Esp_t*)( trailer + blklen )-1;
    }

  t->pad_len = x_blks * blklen - overlap - sizeof(Ip_esp_packet::Esp_t);
  t->next_protocol = np;

  outlen = len;
  ret = l4CryptoTransform(ctx, (const char *)pay_load, blks*blklen,
      data, &outlen, (unsigned char *) iv);
  if (ret != CRYPTO_OK) 
    {
      len = 0;
      return;
    }

  data += blks * blklen;
  outlen = x_blks * blklen;
  ret = l4CryptoTransform(ctx, trailer, x_blks * blklen, data, &outlen, iv);
  if (ret != CRYPTO_OK) 
    {
      len = 0;
      return;
    }

  len = (blks + x_blks) * blklen + sizeof(u64);
}

void Crypt_l4_crypto::decrypt(unsigned long &len, void *pay_load, void *out)
{

  CRYPTO_STATUS ret;
  size_t outlen;

  len -= sizeof(u64); // iv

  L4::cout << "DECRYPT 0\n";

  if ( status != DECRYPT) open( CRYPTO_TYPE_DECRYPT );
  if ( status != DECRYPT) 
    {
      // if we still don`t have a ctx something is very wrong!!!!!!
      len = 0;
      return;
    }
  L4::cout << "DECRYPT 1\n";
  char *data = (char*)(((u8*)pay_load) + blklen);

  memcpy( iv, pay_load, blklen);
  outlen = len;
  ret = l4CryptoTransform(ctx, data, len, (char *)out, &outlen,
      (unsigned char *) iv);
  if (ret != CRYPTO_OK) 
    {
      L4::cout << "Error during Transformation!\n" << l4CryptoErrSring(ctx,ret);
      len = 0;
      return;
    }
  L4::cout << "DECRYPT 2\n";
  len = outlen;

}

