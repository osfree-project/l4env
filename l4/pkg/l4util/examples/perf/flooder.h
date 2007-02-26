#ifndef __FLOODER_H
#define __FLOODER_H


/* following macros must be defined:

CACHELINESIZE_L2	- len (in bytes) of a L2-Cacheline
CACHELINESIZE_L1	- same for L1
ASSOCIATIVITY_L2	- associativity of L2
ASSOCIATIVITY_L1	- same for L1
CACHESIZE_L2		- size of L2 Cache in Bytes
CACHESIZE_L1		- same for L1

data_array must be defined extern and should point to a L2size-aligned
memory area.
*/

#ifndef CACHELINESIZE_L2
#error please define CACHELINESIZE_L2 accordingly.
#endif
#ifndef CACHELINESIZE_L1
#error please define CACHELINESIZE_L1 accordingly.
#endif
#ifndef ASSOCIATIVITY_L2
#error please define ASSOCIATIVITY_L2 accordingly.
#endif
#ifndef ASSOCIATIVITY_L1
#error please define ASSOCIATIVITY_L1 accordingly.
#endif
#ifndef CACHESIZE_L2
#error please define CACHESIZE_L2 accordingly.
#endif
#ifndef CACHESIZE_L1
#error please define CACHESIZE_L1 accordingly.
#endif

#define ADDRSIZE_L1	(CACHESIZE_L1/ASSOCIATIVITY_L1)
#define ADDRSIZE_L2	(CACHESIZE_L2/ASSOCIATIVITY_L2)

#define SERIALIZE() {asm ("cpuid"::"a" (0):"eax","ebx","ecx","edx");}

extern unsigned char data_array[];

inline void flooder(void);

inline void flooder(void){
  int i;
  
  #if 1
  asm (".fill 8192 ,1,0x90");
  #endif
  
  #if 0
  for(i=0;i<CACHESIZE_L2+CACHESIZE_L1;i+=CACHELINESIZE_L2){
    asm (""
         : "=a" (*((l4_umword_t*)&(
             data_array[CACHESIZE_L2 + i ])))
         ) ;
  }
  #endif
  

  /* now, it may be we have values in L1 at data_array[CELL_OFFSET+xx]
     which are not in L2. Access to an other address which
     maps to the same L1-Adress forces the oldest L1-Adress (at CELL_OFFSET)
     to go out, forcing write of modified L1-line to L2-line and memory
     to make that one free. */
       
  /* Uah, it turns out to be a bit complicated. Try the following:
     precondition: l1_ass = 2, l2_ass = 1
    
     wr(l1_size), wr(l2_size), wr(0), wr(2*l1_size).
       
     then the access to (l1_size+l2_size) should do it.
  */

  #if 1

  for(i=0;i<ADDRSIZE_L1;i+=CACHELINESIZE_L1){
    data_array[0*ADDRSIZE_L2 + i ] = 1;
    #if ASSOCIATIVITY_L1 == 4
    data_array[5*ADDRSIZE_L2 + i ] = 1;
    data_array[6*ADDRSIZE_L2 + i ] = 1;
    #endif
    SERIALIZE();
    data_array[1*ADDRSIZE_L2 + i ] = 1;
    SERIALIZE();
  }
  for(i=0;i<ADDRSIZE_L1;i+=CACHELINESIZE_L1){
    data_array[0*ADDRSIZE_L2 + i ] = 1;
    #if ASSOCIATIVITY_L1 == 4
    data_array[5*ADDRSIZE_L2 + i ] = 1;
    data_array[6*ADDRSIZE_L2 + i ] = 1;
    #endif
    SERIALIZE();
    data_array[2*ADDRSIZE_L2 + i ] = 1;
    SERIALIZE();
  }
  #if ASSOCIATIVITY_L2 == 4
    for(i=0;i<ADDRSIZE_L1;i+=CACHELINESIZE_L1){
      data_array[0*ADDRSIZE_L2 + i ] = 1;
      #if ASSOCIATIVITY_L1 == 4
      data_array[5*ADDRSIZE_L2 + i ] = 1;
      data_array[6*ADDRSIZE_L2 + i ] = 1;
      #endif
      SERIALIZE();
      data_array[3*ADDRSIZE_L2 + i ] = 1;
      SERIALIZE();
    }
    for(i=0;i<ADDRSIZE_L1;i+=CACHELINESIZE_L1){
      data_array[0*ADDRSIZE_L2 + i ] = 1;
      #if ASSOCIATIVITY_L1 == 4
      data_array[5*ADDRSIZE_L2 + i ] = 1;
      data_array[6*ADDRSIZE_L2 + i ] = 1;
      #endif
      SERIALIZE();
      data_array[4*ADDRSIZE_L2 + i ] = 1;
      SERIALIZE();
    }
  #endif
  
  #endif
}

#endif
