#ifndef L4_SYS_KIP_H__
#define L4_SYS_KIP_H__

/* THIS IS A C++ HEADER NOT INTENDED FOR C */

namespace L4 
{
  namespace Kip
  {
    class Mem_desc
    {
    public:
      enum Mem_type
      {
	Undefined    = 0x0,
	Conventional = 0x1,
	Reserved     = 0x2,
	Dedicated    = 0x3,
	Shared       = 0x4,

	Bootloader   = 0xe,
	Arch         = 0xf
      };
    private:
      unsigned long _l, _h;

      static unsigned long &memory_info(void *kip)
      { return *((unsigned long *)kip + 21); }

    public:
      static Mem_desc *first(void *kip)
      {
	return (Mem_desc*)((char *)kip
	    + (memory_info(kip) >> ((sizeof(unsigned long)/2)*8)));
      }
      
      static unsigned long count(void *kip)
      {
	return memory_info(kip) & (1UL << ((sizeof(unsigned long)/2)*8)) - 1;
      }
      
      static void count(void *kip, unsigned c)
      {
	unsigned long &mi = memory_info(kip);
	mi = (mi & ~((1UL << ((sizeof(unsigned long)/2)*8)) - 1)) | c;
      }


      Mem_desc(unsigned long start, unsigned long end, 
	  Mem_type t, unsigned char st = 0, bool virt = false)
      : _l((start & ~0x3ffUL) | (t & 0x0f) | ((st << 4) & 0x0f0)
	  | (virt?0x0200:0x0)), _h(end)
      {}

      unsigned long start() const { return _l & ~0x3ffUL; }
      unsigned long end() const { return _h | 0x3ffUL; }
      unsigned long size() const { return end() + 1 - start(); }
      Mem_type type() const { return (Mem_type)(_l & 0x0f); }
      unsigned char sub_type() const { return (_l >> 4) & 0x0f; }
      unsigned is_virtual() const { return _l & 0x200; }

      void set(unsigned long start, unsigned long end, 
	  Mem_type t, unsigned char st = 0, bool virt = false)
      {
	_l = (start & ~0x3ffUL) | (t & 0x0f) | ((st << 4) & 0x0f0)
	  | (virt?0x0200:0x0);

	_h = end | 0x3ffUL;
      }

    };
  };
};


#endif
