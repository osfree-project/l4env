/*
 * Copyright (C) 2004  Alexander Warg  <alexander.warg@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the nethub package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#include "ip_sec.h"
#include "ip.h"
#if !defined(WITHOUT_CRYPTO)
#  if defined(USE_L4CRYPTO)
#    include "ip_sec_l4crypto.h"
#  else
#    include "ipsec_md5h.h"
#    include "des.h"
#  endif
#endif

#include <l4/cxx/iostream.h>

#include <l4/nethub/cfg-types.h>


Crypt_algo *Crypt_algo::get( unsigned ealg, unsigned key_bits, char *key )
{
#if defined(WITHOUT_CRYPTO)
  return 0;
#endif
  // trivial
  if (!ealg || !key_bits || !key)
    return 0;

  if (ealg == NH_EALG_3DESCBC && key_bits == 192)
#if defined(USE_L4CRYPTO)
    return new Crypt_l4_crypto(key, (size_t)(key_bits/8), 
	                       CRYPTO_AE_3DES_EDE, 
			       CRYPTO_CM_CBC | CRYPTO_CM_CHUNK);
#else
    return new Crypt_3des( key );
#endif

  return 0;
}

Auth_algo *Auth_algo::get( unsigned aalg, unsigned key_bits, char *key )
{
#if defined(WITHOUT_CRYPTO)
  return 0;
#endif

  if (!aalg || !key_bits || !key)
    return 0;

  if (aalg == NH_AALG_MD5HMAC && key_bits == 128)
#if defined(USE_L4CRYPTO)
    return new Auth_l4_crypto(key, key_bits/8, CRYPTO_AH_MD5);
#else
    return new Auth_hmac_md5( key );
#endif

  return 0;
}

L4::BasicOStream &operator << (L4::BasicOStream &s, Crypt_algo const &a)
{
  s << "Crypto: " << a.name();
  return s;
}

L4::BasicOStream &operator << (L4::BasicOStream &s, Auth_algo const &a)
{
  s << "Auth: " << a.name();
  return s;
}

#if !defined(USE_L4CRYPTO) && !defined(WITHOUT_CRYPTO)
static int memcmp(void const *i, void const *o, unsigned len)
{
  u8 const *ii = (u8 const *)i;
  u8 const *oo = (u8 const *)o;
  while (len && *(ii++) == *(oo++))
    --len;

  if (len)
    return 1;
  else
    return 0;
}

Auth_hmac_md5::Auth_hmac_md5(char const *_key)
{
  for (unsigned i = 0; i<sizeof(key); i++)
    key[i] = _key[i];
}

Auth_hmac_md5::~Auth_hmac_md5()
{}

char const *const Auth_hmac_md5::name() const
{
  return "md5-hmac";
}

void Auth_hmac_md5::print( L4::BasicOStream &s ) const
{
  s << "md5-hmac";
}

unsigned Auth_hmac_md5::auth( unsigned long &len, void *pay_load, 
                              bool sign )
{ 
  MD5_CTX ctx;
  u8 md[auth_len_full];
  

  if (!sign)
    {
      if (len < auth_len_trunc)
	return 0;
      else
        len -= auth_len_trunc; // subtract authenticator len
    }

#if 0
  L4::cout << "Do HMAC MD5 AUTH\n";
  
  L4::cout << L4::hex << "AUTH: @" << pay_load << " len=" << L4::dec 
    << (unsigned)len << "\n";
#endif
  
  unsigned char ipad[auth_blk_len];
  unsigned char opad[auth_blk_len];
  for (unsigned i = 0; i<sizeof(ipad); i++) 
    {
      ipad[i] = 0x36;
      opad[i] = 0x5c;
    }

  for (unsigned i= 0; i<auth_len_full; i++)
    {
      ipad[i] ^= key[i];
      opad[i] ^= key[i];
    }
  
  MD5Init( &ctx );
  MD5Update( &ctx, ipad, auth_blk_len );
  MD5Update( &ctx, (unsigned char*)pay_load, len );
  MD5Final( (unsigned char *)md, &ctx );
  
  MD5Init( &ctx );
  MD5Update( &ctx, opad, auth_blk_len );
  
  if (!sign)
    {
      MD5Update( &ctx, (unsigned char*)md, auth_len_full );
      MD5Final( (unsigned char *)md, &ctx );
      
      if (memcmp( md, ((u8*)pay_load)+len, auth_len_trunc ) == 0)
	return 1;
    }
  else
    {
      MD5Update( &ctx, (unsigned char*)md, auth_len_full );
      MD5Final( ((unsigned char *)pay_load)+len, &ctx );
      len += auth_len_trunc;
    }

  return 0;
}

Crypt_3des::Crypt_3des( char const *_key )
  : Crypt_algo(8)
{
  des_set_key( (des_cblock*)_key + 0, key[0] );
  des_set_key( (des_cblock*)_key + 1, key[1] );
  des_set_key( (des_cblock*)_key + 2, key[2] );
  
  //*((u64*)iv) = random();
}

char const *const Crypt_3des::name() const
{
  return "3des-cbc";
}

void Crypt_3des::print( L4::BasicOStream &s ) const
{
  s << "3des-cbc";
}

void Crypt_3des::encrypt( unsigned long &len, void *pay_load, void *out, char np )
{
  u64 *_iv = (u64*)out;
  char *data = (char*)(((u64*)out) + 1);

  *_iv = *(u64*)iv;

  des_cblock trailer[2];
  Ip_esp_packet::Esp_t *t;

  unsigned x_blks;
  unsigned blks    = len / blk_size();
  unsigned overlap = len - (blks * blk_size());

  char *i = (char*)pay_load + blks * blk_size();
  char *o = (char*)trailer;
  for (unsigned x=overlap; x; --x)
    *(o++) = *(i++);

  if ((blk_size()-overlap) < sizeof(Ip_esp_packet::Esp_t))
    {
      x_blks = 2;
      t = (Ip_esp_packet::Esp_t*)((des_cblock*)trailer+2)-1;
    }
  else
    {
      x_blks = 1;
      t = (Ip_esp_packet::Esp_t*)((des_cblock*)trailer+1)-1;
    }
      
  t->pad_len = x_blks * blk_size() - overlap - sizeof(Ip_esp_packet::Esp_t);
  t->next_protocol = np;

  des_ede3_cbc_encrypt( (des_cblock*)pay_load,
                        (des_cblock*)data,
			blks*blk_size(),
			key[0], key[1], key[2],
			&iv, 1 );
  
  data += blks * blk_size();

  des_ede3_cbc_encrypt( trailer,
                        (des_cblock*)data,
			x_blks * blk_size(),
			key[0], key[1], key[2],
			&iv, 1);

  len = (blks + x_blks) * blk_size() + sizeof(u64);
}

void Crypt_3des::decrypt( unsigned long &len, void *pay_load, void *out )
{
  u64 *iv = (u64*)pay_load;
  u64 *data = (u64*)pay_load + 1;
  len -= sizeof(u64); // iv
  
  des_ede3_cbc_encrypt( (des_cblock*)data,
                        (des_cblock*)out,
			len,
			key[0], key[1], key[2],
			(des_cblock*)iv, 0);
}

#endif

