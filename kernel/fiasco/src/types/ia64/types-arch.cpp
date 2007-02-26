/* IA-64 Type definitions */

INTERFACE:

/// standard fixed-width types
typedef unsigned char          unsigned8;
typedef signed char            signed8;
typedef unsigned short         unsigned16;
typedef signed short           signed16;
typedef unsigned int           unsigned32;
typedef signed int             signed32;
typedef unsigned long int      unsigned64;
typedef signed long int        signed64;

/// machine word
typedef unsigned64 mword_t; 
#define MWORD_BITS (64)

/// (virtual or physical address) should be addr_t or something
typedef unsigned64 vm_offset_t; 
typedef unsigned64 addr_t;

/// (size type for memory)
typedef unsigned64 vm_size_t; 

typedef unsigned64 cpu_time_t;

IMPLEMENTATION [arch]:

//-
