#ifndef TYPES_ARCH_H__
#define TYPES_ARCH_H__
/// HACK: Prevent <l4/sys/types.h> from redefining these types.
#define __L4_TYPES_H__

#define L4_PTR_FMT "0x%08x"
#define L4_PTR_ARG(a) ((Unsigned32)(a)) 
#define L4_X64_FMT "0x%016llx"


/// standard fixed-width types
typedef unsigned char          Unsigned8;
typedef signed char            Signed8;
typedef unsigned short         Unsigned16;
typedef signed short           Signed16;
typedef unsigned int           Unsigned32;
typedef signed int             Signed32;
typedef unsigned long long int Unsigned64;
typedef signed long long int   Signed64;

/// machine word
typedef Signed32 Smword;
typedef Unsigned32 Mword;
enum {
  MWORD_BITS =32
};

/// (virtual or physical address) should be addr_t or something
typedef Unsigned32 Address;

typedef Unsigned64 Cpu_time;


#endif
