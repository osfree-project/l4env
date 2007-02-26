#ifndef L4_SIGMA0_CONFIG_H__
#define L4_SIGMA0_CONFIG_H__

#ifdef CPUTYPE_sa
enum {
  Ram_base = 0xc0000000,
};
#endif
#ifdef CPUTYPE_pxa
enum {
  Ram_base = 0xa0000000,
};
#endif


#endif // L4_SIGMA0_CONFIG_H__

