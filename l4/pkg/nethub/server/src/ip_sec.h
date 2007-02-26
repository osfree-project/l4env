/* -*- c++ -*- */

/*
 * Copyright (C) 2004  Alexander Warg  <alexander.warg@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the nethub package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#ifndef IP_SEC_H__
#define IP_SEC_H__
#if !defined(WITHOUT_CRYPTO) && !defined(USE_L4CRYPTO)
#  include "des.h"
#endif
#include "net.h"
#include "ip.h"
#include "ip_forward.h"

namespace L4 {
  class BasicOStream;
}

enum {
  IP_ESP_PROTO = 50,
  IP_AH_PROTO  = 51,
};

class Crypt_algo
{
public:
  Crypt_algo( unsigned bs )
    : _bs(bs)
  {}
  unsigned blk_size() const { return _bs; }
  virtual void encrypt( unsigned long &len, void *pay_load, void *out, 
                        char np ) = 0;
  virtual void decrypt( unsigned long &len, void *pay_load, void *out ) = 0;
  virtual char const *const name() const = 0;
  virtual void print( L4::BasicOStream &s ) const = 0;

  static Crypt_algo *get( unsigned ealg, unsigned key_bits, char *key );

private:
  unsigned const _bs;
  
};

class Auth_algo
{
public:
  virtual unsigned auth( unsigned long &len, void *pay_load, 
                         bool sign = false ) = 0;
  virtual ~Auth_algo() {};
  virtual char const *const name() const = 0;
  virtual void print( L4::BasicOStream &s ) const = 0;
  static Auth_algo *get( unsigned aalg, unsigned key_bits, char *key );
};

L4::BasicOStream &operator << (L4::BasicOStream &s, Crypt_algo const &a);
L4::BasicOStream &operator << (L4::BasicOStream &s, Auth_algo const &a);

class Crypt_3des : public Crypt_algo
{
public:
  Crypt_3des( char const *key );
  void encrypt( unsigned long &len, void *pay_load, void *out, char np );
  void decrypt( unsigned long &len, void *pay_load, void *out );
  char const *const name() const;
  void print( L4::BasicOStream &s ) const;
 
private:
#if !defined(WITHOUT_CRYPTO) && !defined(USE_L4CRYPTO)
  des_key_schedule key[3];
  des_cblock iv;
#endif
};

class Auth_hmac_md5 : public Auth_algo
{
public:
  enum {
    auth_len_full  = 128/8,
    auth_len_trunc =  96/8,
    auth_blk_len   =  64,
  };

  Auth_hmac_md5( char const *key );
  ~Auth_hmac_md5();
  unsigned auth( unsigned long &len, void *pay_load, bool sign = false );
  char const *const name() const;
  void print( L4::BasicOStream &s ) const;
 
private:
  char key[auth_len_full];
};

unsigned ipsec_copy( Ip_packet *in, Ip_packet *out, 
                     unsigned long channel );

typedef unsigned long (Auth_func)( void *key, void *pay_load, 
                                   unsigned long &len, bool sign );
typedef unsigned long (Crypt_func)( void *key, void *pay_load, 
                                    unsigned long &len, void *out );

class Ipsec_packet : public Ip_packet
{
public:
  class Esp_h 
  {
  public:
    u32 spi() const { return _spi; }
    void spi( u32 s ) { _spi = s; }
    u32 seq() const { return Net::n_to_h(_seq); }
    void seq( u32 s ) { _seq = Net::h_to_n(s); }
  private:
    u32 _spi;
    u32 _seq;
  public:
    u8 data[0];
  } __attribute__((packed));

  struct Esp_t {
    u8 pad_len;
    u8 next_protocol;
  } __attribute__((packed));
  
  struct Ah_h {
    u32 spi() const { return _spi; }  
    u8 next_protocol;
    u8 paylod_len;
    u16 reserved;
    u32 _spi;
    u32 _seq;
    u8 data[0];
  } __attribute__((packed));

  inline u32 spi() const;
  inline u32 seq() const;
  
protected:
  inline Ah_h const *get_ah_head() const;
  inline Esp_h const *get_esp_head() const;
};

class Ip_esp_packet : public Ipsec_packet
{
public:
  inline Esp_h *get_esp_head();
  inline Esp_h const *get_esp_head() const;
  inline Esp_t *get_esp_tail();
  void un_esp();
  
private:
};

class Ip_ah_packet : public Ipsec_packet
{
};

inline Ipsec_packet::Esp_h const *Ipsec_packet::get_esp_head() const
{
  return reinterpret_cast<Esp_h const*>(
    reinterpret_cast<u32 const*>(this) + head_len());
}

inline Ipsec_packet::Ah_h const *Ipsec_packet::get_ah_head() const
{
  return reinterpret_cast<Ah_h const*>(
    reinterpret_cast<u32 const*>(this) + head_len());
}

inline u32 Ipsec_packet::spi() const
{
  switch (protocol())
    {
    case IP_ESP_PROTO: return get_esp_head()->spi();
    case IP_AH_PROTO:  return get_ah_head()->spi();
    default: return 0;
    }
}

inline Ip_esp_packet::Esp_h *Ip_esp_packet::get_esp_head()
{
  return reinterpret_cast<Esp_h*>(reinterpret_cast<u32*>(this) + head_len());
}

inline Ip_esp_packet::Esp_h const *Ip_esp_packet::get_esp_head() const
{
  return Ipsec_packet::get_esp_head();
}

inline Ip_esp_packet::Esp_t *Ip_esp_packet::get_esp_tail()
{
  return reinterpret_cast<Esp_t*>(reinterpret_cast<u8*>(this) + len()) - 1;
}

inline void Ip_esp_packet::un_esp() 
{
  Esp_t *t = get_esp_tail();
  protocol( t->next_protocol );
  len( len() - t->pad_len - sizeof(Esp_t) );
  set_checksum();
}

#endif // IP_SEC_H__

