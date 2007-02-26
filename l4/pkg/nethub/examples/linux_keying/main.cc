/*
 * Copyright (C) 2004  Alexander Warg  <alexander.warg@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the nethub package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#include <l4/sys/types.h>
#include <l4/sys/syscalls.h>
#include <l4/sys/ipc.h>

#include <l4/log/l4log.h>
#include <l4/names/libnames.h>
#include <l4/util/util.h>

#include <l4/cxx/l4types.h>

#include <l4/cxx/iostream.h>
#include <l4/cxx/l4iostream.h>
#include <l4/nethub/client.h>
#include <l4/nethub/cfg.h>

#include <unistd.h>
#include <getopt.h>
#include <netinet/in.h>
#include <netdb.h>
#include <cstring>

#include <stddef.h>
#include <cstdlib>

char LOG_tag[9]=LOG_TAG;

enum {
  opt_sa_add  = 1,
  opt_sa_del,
  opt_flush,
  opt_dump,
  opt_dump_stats,
  opt_dst,
  opt_src,
  opt_iface,
  opt_oface,
  opt_auth_alg,
  opt_auth_key,
  opt_enc_alg,
  opt_enc_key,
  opt_mode,
  opt_spd_add,
  opt_spd_del,
  opt_sa,
  opt_if_mode,
  opt_help,
};

struct option opts[] =
{
  {"sa-add",   1, 0, opt_sa_add },
  {"sa-del",   1, 0, opt_sa_del },
  {"sa",       1, 0, opt_sa },
  {"flush",    0, 0, opt_flush },
  {"dump",     0, 0, opt_dump },
  {"dump-stats", 1, 0, opt_dump_stats },
  {"iface",    1, 0, opt_iface },
  {"oface",    1, 0, opt_oface },
  {"dst",      1, 0, opt_dst },
  {"src",      1, 0, opt_src },
  {"auth-alg", 1, 0, opt_auth_alg },
  {"auth-key", 1, 0, opt_auth_key },
  {"enc-alg",  1, 0, opt_enc_alg },
  {"enc-key",  1, 0, opt_enc_key },
  {"mode",     1, 0, opt_mode },
  {"spd-add",  0, 0, opt_spd_add },
  {"spd-del",  0, 0, opt_spd_del },
  {"if-mode",  1, 0, opt_if_mode },
  {"help",     0, 0, opt_help },

  {0, 0, 0, 0},
};

struct Str_val
{
  char const *str; 
  unsigned long val;
};

static Str_val proto_map[] = 
{
    {"esp", NH_SA_TYPE_ESP},
    {"ah" , NH_SA_TYPE_AH},
    {"pas", NH_SA_TYPE_PAS},
    {0,0}
};

static Str_val if_modes[] = 
{
    {"encrypt", NH_SADB_IFACE_OUTBOUND},
    {"any",     NH_SADB_IFACE_INBOUND},
    {0,0}
};

static Str_val mode_map[] =
{
    {"tunnel",    NH_SA_TYPE_TUN_MASK},
    {"transport", 0},
    {0,0}
};

static Str_val ealgs[] =
{
    {"null",     NH_EALG_NONE },
    {"3des-cbc", NH_EALG_3DESCBC },
    {0,0}
};

static Str_val aalgs[] =
{
    {"null",     NH_AALG_NONE },
    {"hmac-md5", NH_AALG_MD5HMAC },
    {0,0}
};

static int op;
static char *dst_ip;
static char *src_ip;
static char *auth_alg;
static char *enc_alg;
static char *auth_key;
static char *enc_key;
static char *mode;
static char *iface;
static char *oface;
static char *sa_key;
static char *op_arg;

static void usage(char const *name)
{
  L4::cout << "SYNTAX: " << name << " <operation> <options>\n\n"
              "OPERATIONS:\n"
	      "  --dump                    Dump SADB and SPD (to L4-Log)\n"
	      "  --dump-stats <iface>      Dump statisticts of <iface>\n"
	      "  --flush                   Flush complete SADB and SPD\n"
	      "  --if-mode <iface>/<mode>  Set mode of interface\n"
	      "  --sa-add <sa>             Add a security association\n"
	      "  --sa-del <sa>             Delete a security association\n"
	      "  --spd-add                 Add a security policy\n"
	      "  --spd-del                 Delete a security policy\n"
	      "  --help                    Show detailed help\n\n"
	      "OPTIONS:\n"
	      "  --auth-alg <aalg>         Authentication algorithm\n"
	      "  --auth-key <akey>         Authentication key\n"
	      "  --dst <dst>               Destination net\n"
	      "  --enc-alg <ealg>          En/Decryption algorithm\n"
	      "  --enc-key <ekey>          En/Decryption key\n"
	      "  --iface <iface>           Input interface or just interface\n"
	      "  --mode [tunnel|transport] Mode of IPSec protocol\n"
	      "  --oface <oface>           Output interface\n"
	      "  --sa <sa>                 Security association key\n"
	      "  --src <src>               Source IP or Source net\n\n";

  exit(1);
}


static void help(char const *name)
{
  L4::cout << "SYNTAX\n"
           << name << " <operation> <options>\n\n"
           "OPERATIONS (<operation>)\n"
           "--sa-add <sa> --oface <oface>\n"
           "      [--mode 'tunnel'|'transport'] [--src <src>]\n"
           "      [--auth-alg <aalg> --auth-key <key>]\n"
           "      [--enc-alg <ealg> --enc-key <key>]\n"
           "    Add security association. --src must be specified for tunnel\n"
           "    mode. The given <oface> is the interface where packets "
	   "traveling\n    through this SA go to.\n\n"
           "--sa-del <sa>\n"
           "    Delete securety association.\n\n"
           "--spd-add --src <src-net> --dst <dst-net> --sa <sa>\n"
           "    Add security policy.\n\n"
           "--spd-del --src <src-net> --dst <dst-net>\n"
           "    Delete security policy.\n"
	   "--if-mode <iface>/[encrypt|any]\n"
	   "    Set the mode of <iface> to either 'any' (process ipsec "
	   "packets)\n    or 'encrypt' (use the security policy, even for "
	   "ipsec packets).\n\n"
           "--flush\n"
           "    Flush complete sadb and reset inbound policyto 'drop'.\n\n"
           "--dump\n"
           "    Dump sadb to log.\n\n"
	   "--dump-stats <iface>\n"
	   "    Dump the statistics of the given interface.\n\n"
           "OPTION SYNTAX\n"
           "  <sa>    ::=   <proto>/<spi>/<dst>\n"
           "  <proto> ::=   'esp'|'ah'|'pas'\n\n"
           "  <spi> ....... numeric security policy index\n"
           "  <dst> ....... destination ip address or host name\n"
           "  <src> ....... source ip address or host name for tunnel mode\n"
           "  <dst-net> ... destination network (<addr>/<prefix>)\n"
           "  <src-net> ... source network (<addr>/<prefix>)\n"
           "  <aalg> ...... authenitaction algorithm\n"
           "  <ealg> ...... encryption algorithm\n"
           "  <key> ....... hex or ascii key\n"
           "  <oface> ..... interface number\n"
           "  <iface> ..... interface number\n"
           ""
           "" ;

  exit(1);
}

static void fail(char const *e)
{
  L4::cerr << "error: " << e << " -- exit\n";
  exit(1);
}

// utility functions

static unsigned long resolve_ip( char *in );
static unsigned long scan_ip_net( char *in );
static unsigned tokenize( char *in, char **out[] );
static unsigned long strtovalue( char *in, Str_val map[], int *err );
static int hextochar( char c );
static int strtochar( char const *s );

static int flush( l4_threadid_t h )
{
  int err1 = nh_sadb_flush_sp(h);
  int err2 = nh_sadb_flush_sa(h);
  
  if (err1 == NH_OK && err2 == NH_OK)
    return 0;
  else
    {
      L4::cerr << "FLUSH: returned EINVAL\n";
      return 1;
    }
}

static int dump( l4_threadid_t h )
{
  int err1 = nh_sadb_dump_sa(h);
  int err2 = nh_sadb_dump_sp(h);
  if (err1 == NH_OK && err2 == NH_OK)
    return 0;
  else
    {
      L4::cerr << "DUMP: returned EINVAL\n";
      return 1;
    }
}

static int parse_sa_key( char *sa_key, Nh_sadb_sa_key *k )
{
  if (!sa_key)
    {
      L4::cerr << "missing SA\n";
      return -1;
    }

  char *proto, *spi, *edst_ip;

  if (sa_key)
    {
      char **part[] = {&proto, &spi, &edst_ip, 0};
      unsigned x = tokenize(sa_key, part );
      if (x!=3)
	{
	  L4::cerr << "error: parsing short SA\n";
	  return -1;
	}
    }
  
  k->spi = htonl(strtoul(spi,0,0));
  k->dst_ip = resolve_ip(edst_ip);
  if (k->dst_ip == (uint32_t)-1)
    return -1;
  
  int err = 0;
  k->satype = strtovalue( proto, proto_map, &err );
  if (err)
    {
      L4::cerr << "error: unknown protocol: " << proto << '\n';
      return -1;
    }

  if (!mode)
    return 0;

  k->satype |= strtovalue( mode, mode_map, &err );
  if (err)
    {
      L4::cerr << "error: unknown mode: " << mode << '\n';
      return -1;
    }

  return 0;
}

static int parse_key(unsigned keymax, char const *in, char *key)
{
  if (strncmp(in,"0x",2)==0)
    {
      in += 2;

      unsigned i = 0;
      unsigned o = 0;
      for (; i<strlen(in); ++i)
	{
	  if (o>=keymax)
	    return -1;
	  
	  int x = strtochar(in + i);
	  if (x==-1 && in[i]=='_')
	    {
	      continue;
	    }
	  else if (x==-1)
	    {
	      return -1;
	    }
	  else
	    {
	      key[o++]=strtochar(in + (i++));
	    }
	}
      return o;
    }
  else
    {
      if (strlen(in)>keymax)
	return -1;

      memcpy(key,in,strlen(in));
      return strlen(in);
    }
}

static int parse_aalg( Nh_sadb_sa_data *d, unsigned keymax )
{

  if (!auth_alg)
    {
      L4::cerr << "error: missing --auth-alg\n";
      return -1;
    }
  int err = 0;
  d->auth = strtovalue( auth_alg, aalgs, &err );

  if (err)
    {
      L4::cerr << "error: unknown algorithm: " << auth_alg << '\n';
      return -1;
    }

  d->auth_key_bits = 0;

  if (d->auth == NH_AALG_NONE)
    return 0;

  if (!auth_key)
    {
      L4::cerr << "error: missing auth key\n";
      return -1;
    }

  char *keys = (char*)(d+1);
  int bytes = parse_key(keymax, auth_key, keys);
  if (bytes < 0)
    {
      L4::cerr << "error: can't parse auth key\n";
      return -1;
    }

  d->auth_key_bits = 8*bytes;
  return 0;
}

static int parse_ealg( Nh_sadb_sa_data *d, unsigned keymax )
{
  if (!enc_alg)
    {
      L4::cerr << "error: missing --enc-alg\n";
      return -1;
    }

  int err = 0;
  d->encrypt = strtovalue( enc_alg, ealgs, &err );
  if (err)
    {
      L4::cerr << "error: unknown algorithm: " << enc_alg << '\n';
      return -1;
    }

  d->encrypt_key_bits = 0;

  if (d->encrypt == NH_EALG_NONE)
    return 0;

  if (!enc_key)
    {
      L4::cerr << "error: missing encrypt key\n";
      return -1;
    }

  char *keys = (char*)(d+1) + (d->auth_key_bits/8);
  int bytes = parse_key(keymax-(d->auth_key_bits/8), enc_key, keys);
  if (bytes < 0)
    {
      L4::cerr << "error: can't parse encrypt key\n";
      return -1;
    }

  d->encrypt_key_bits = 8*bytes;
  return 0;
}
static int parse_tun_data( Nh_sadb_sa_data *d )
{
  if (!src_ip)
    {
      L4::cerr << "error: missing source address for tunnel mode\n";
      return -1;
    }

  d->src_ip = resolve_ip(src_ip);
  if (d->src_ip == (unsigned long)-1)
    return -1;

  return 0;
}

static int sa_add( l4_threadid_t h )
{

  struct Sadb_add_sa_msg 
  {
    Nh_sadb_sa_msg m;
    char keys[800];
  } m;

  m.m.data.state = NH_SA_STATE_MATURE;
  m.m.data.src_ip = 0;


  if (!oface)
    return -1;

  m.m.data.dst_if = strtoul(oface,0,0);
  
  if (parse_sa_key( op_arg, &m.m.key ) != 0)
    return -1;

  unsigned type = m.m.key.satype & ~NH_SA_TYPE_TUN_MASK;
  if (type == NH_SA_TYPE_ESP || type == NH_SA_TYPE_AH)
    if (parse_aalg( &m.m.data, sizeof(m.keys) ) != 0)
      return -1;

  if (type == NH_SA_TYPE_ESP)
    if (parse_ealg( &m.m.data, sizeof(m.keys) ) != 0)
      return -1;

  if (m.m.key.satype & NH_SA_TYPE_TUN_MASK)
    if (parse_tun_data( &m.m.data ) != 0)
      return -1;

  if (nh_sadb_add_sa(h, &m.m, sizeof(m)) != NH_OK)
    {
      L4::cerr << "error: sadb_add returened EINVAL\n";
      return 1;
    }

  return 0;
}

static int sa_del( l4_threadid_t h )
{
  struct Nh_sadb_sa_key m;
  if (parse_sa_key( op_arg, &m ) != 0)
    return -1;
  
  if (nh_sadb_del_sa(h, &m) != NH_OK)
    {
      L4::cerr << "error: sadb_del returened EINVAL\n";
      return 1;
    }

  return 0;
}

static int spd_add( l4_threadid_t h )
{
  struct Nh_sadb_sa_key sa;
  struct Nh_sp_address src, dst;

  unsigned long src_if;
  
  if (parse_sa_key( sa_key, &sa ) != 0)
    return -1;

  if (!src_ip || !dst_ip)
    {
      L4::cerr << "error: SPD needs source and dst addresses\n";
      return -1;
    }

  if (iface)
    src_if = strtoul(iface,0,0);
  else
    src_if = -1UL;

  dst.prefix_len = scan_ip_net(dst_ip);
  src.prefix_len = scan_ip_net(src_ip);
  dst.address = resolve_ip(dst_ip);
  src.address = resolve_ip(src_ip);
  src.port = 0;
  dst.port = 0;
  src.proto = 0;

  if (dst.address == (uint32_t)-1 || src.address == (uint32_t)-1)
    return -1;
  
  if (nh_sadb_add_sp(h, src_if, &src, &dst, &sa) != NH_OK)
    {
      L4::cerr << "error: sadb_add_spd returened EINVAL\n";
      return -1;
    }

  return 0;
}

static int spd_del( l4_threadid_t h )
{
  struct Nh_sp_address src, dst;
  unsigned long src_if;

  if (!src_ip || !dst_ip)
    {
      L4::cerr << "error: SPD needs source and dst addresses\n";
      return -1;
    }
  
  if (iface)
    src_if = strtoul(iface,0,0);
  else
    src_if = -1UL;

  dst.prefix_len = scan_ip_net(dst_ip);
  src.prefix_len = scan_ip_net(src_ip);
  dst.address = resolve_ip(dst_ip);
  src.address = resolve_ip(src_ip);
  src.proto = 0;
  src.port = 0;
  dst.port = 0;
  
  if (dst.address == (uint32_t)-1 || src.address == (uint32_t)-1)
    return -1;
  
  if (nh_sadb_del_sp(h, src_if, &src, &dst) != NH_OK)
    {
      L4::cerr << "error: sadb_del_spd returened EINVAL\n";
      return -1;
    }

  return 0;
}

static int set_if_mode( l4_threadid_t h )
{
  int err = 0;
  char *ifa, *mode;
  char **part[] = { &ifa, &mode , 0 };
  unsigned x = tokenize( op_arg, part );
  if (x!=2)
    {
      L4::cerr << "error: expected <ifnace>/<mode>\n";
      return -1;
    }
  
  unsigned p = strtoul(ifa,0,0);
  unsigned m = strtovalue( mode, if_modes, &err );
  if (err)
    {
      L4::cerr << "error: unknown iface mode\n";
      return -1;
    }

  if (nh_sadb_chg_iface(h, p, m) != NH_OK)
    {
      L4::cerr << "error: sadb_set_iface_direction returned EINVAL\n";
      return -1;
    }
  return 0;
}

static int dump_stats( l4_threadid_t h )
{
  unsigned m = strtoul(op_arg, 0, 0);
  unsigned rx,tx,rxd,txd;
  if (nh_if_stats(h, m, &tx, & txd, &rx, &rxd))
    {
      L4::cerr << "error: nh_get_iface_stats\n";
      return -1;
    }

  L4::cout << "Stats for iface " << L4::dec << m << "\n"
              " tx: " << tx << "\t\ttxd: " << txd << "\n"
	      " rx: " << rx << "\t\trxd: " << rxd << '\n';
  return 0;
}

int main(int argc, char *argv[])
{
  l4_threadid_t hub;
  int err;

  opterr = 1;
  
  while ((err=getopt_long(argc, argv, ":", opts, 0)) != -1)
    {
      switch (err)
        {
	case ':':
	  L4::cerr << "error missing argument for option: " << argv[optind-1] 
	           << '\n';
	  usage(argv[0]);
	  exit(1);
	  break;
	case '?':
	  L4::cerr << "error: unkonwn option: " << argv[optind-1] << '\n';
	  usage(argv[0]);
	  exit(1);
	  break;
	case opt_if_mode:
	case opt_sa_add:
	case opt_sa_del:
	case opt_dump_stats:
	  op_arg = optarg;
	case opt_spd_add:
	case opt_spd_del:
	case opt_dump:
	case opt_flush:
	case opt_help:
	  if (op)
	    fail("only one operation allowed");
	  op = err;
	  break;
	case opt_sa:
	  sa_key = optarg;
	  break;
	case opt_dst:
	  dst_ip = optarg;
	  break;
	case opt_src:
	  src_ip = optarg;
	  break;
	case opt_auth_alg:
	  auth_alg = optarg;
	  break;
	case opt_auth_key:
	  auth_key = optarg;
	  break;
	case opt_enc_alg:
	  enc_alg = optarg;
	  break;
	case opt_enc_key:
	  enc_key = optarg;
	  break;
	case opt_mode:
	  mode = optarg;
	  break;
	case opt_iface:
	  iface = optarg;
	  break;
	case opt_oface:
	  oface = optarg;
	  break;
	default:
	  break;
        }
    }

  if (!op)
    {
      L4::cerr << "error: at least one command needed\n";
      usage(argv[0]);
    }

  if (!names_waitfor_name("l4/nethub", &hub, 5000))
    {
      L4::cerr << "error l4-nethub (not found)\n";
      return 1;
    }

  //  L4::cout << "connected to l4-nethub @" << hub << '\n';
  
  switch (op)
    {
    case opt_sa_add:    return sa_add( hub );
    case opt_sa_del:    return sa_del( hub );
    case opt_flush:     return flush( hub );
    case opt_dump:      return dump( hub );
    case opt_spd_add:   return spd_add( hub );
    case opt_spd_del:   return spd_del( hub );
    case opt_if_mode:   return set_if_mode( hub );
    case opt_dump_stats:return dump_stats( hub );
    case opt_help:      help(argv[0]); return -1;
    default:            usage(argv[0]); return -1;
    }

  return 0;
}

//----- utils -----------------------------------------------------------------
static unsigned tokenize( char *in, char **out[] )
{
  if (!out[0])
    return 0;

  unsigned x = 0;
  *out[x++] = in;
  for (unsigned p=0; in[p] && out[x]; ++p)
    if (in[p]=='/')
      {
	in[p] = 0;
	*out[x++] = in + p + 1;
      }
  
  return x;
}

static int hextochar( char c )
{
  if (c>='0' && c<='9')
    return c - '0';

  if (c>='A' && c<='F')
    return c - 'A' + 10;

  if (c>='a' && c<='f')
    return c - 'a' + 10;

  return -1;
}

static int strtochar( char const *s )
{
 int x0 = hextochar(s[0]);
 int x1 = hextochar(s[1]);
 if (x0==-1 || x1==-1)
   return -1;

 return (x0<<4)|x1;
}

static unsigned long strtovalue( char *in, Str_val map[], int *err )
{
  for (unsigned n=0; map[n].str;++n)
    if (strcmp(map[n].str, in)==0)
      {
	if (err) *err = 0;
	return map[n].val;
      }
  if (err) *err = 1;
  return (unsigned long)-1;
}

static unsigned long scan_ip_net( char *in )
{
  char *mask = 0;
  for (unsigned i=0; in[i]; ++i)
    if (in[i]=='/')
      {
	in[i] = 0;
	mask = in + i + 1;
	break;
      }

  if (mask)
    return strtoul(mask, 0, 0);
  
  return 32;
}

static unsigned long resolve_ip( char *in )
{
  struct hostent *he = gethostbyname(in);
  if (!he)
    {
      L4::cerr << "error: '" << in 
	       << "' is either an invalid ip or hostname\n";
      return (unsigned long)-1;
    }
  return *(uint32_t*)(he->h_addr_list[0]);
}
