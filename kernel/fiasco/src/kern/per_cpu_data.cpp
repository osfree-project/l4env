INTERFACE:

#include "static_init.h"

#define DEFINE_PER_CPU __attribute__((section(".per_cpu.data"),init_priority(CPU_LOCAL_INIT_PRIO)))

class Per_cpu_data
{
};

template< typename T >
class Per_cpu : private Per_cpu_data
{
private:
  T _d;

public:
  T const &cpu(unsigned) const;
  T &cpu(unsigned);

  Per_cpu();
  Per_cpu(bool);
};


//---------------------------------------------------------------------------
IMPLEMENTATION [!mp]:

IMPLEMENT inline
template< typename T >
T const &Per_cpu<T>::cpu(unsigned) const { return _d; }

IMPLEMENT inline
template< typename T >
T &Per_cpu<T>::cpu(unsigned) { return _d; }

IMPLEMENT
template< typename T >
Per_cpu<T>::Per_cpu()
{}

IMPLEMENT
template< typename T >
Per_cpu<T>::Per_cpu(bool) : _d(0)
{}

//---------------------------------------------------------------------------
INTERFACE [mp]:

#include <cstddef>

EXTENSION
class Per_cpu_data
{
protected:
  enum { Max_num_cpus = 4 };
  static unsigned long _offsets[Max_num_cpus] asm ("PER_CPU_OFFSETS");
  static unsigned long _num_cpus;
};


//---------------------------------------------------------------------------
IMPLEMENTATION [mp]:

unsigned long Per_cpu_data::_offsets[Max_num_cpus];
unsigned long Per_cpu_data::_num_cpus;

inline void *operator new (size_t, void *p) { return p; }
inline void *operator new [] (size_t, void *p) { return p; }

IMPLEMENT inline template< typename T >
T const &Per_cpu<T>::cpu(unsigned cpu) const
{ return *reinterpret_cast<T const *>(((char const*)&_d) + _offsets[cpu]); }

IMPLEMENT inline template< typename T >
T &Per_cpu<T>::cpu(unsigned cpu)
{ return *reinterpret_cast<T*>(((char*)&_d) + _offsets[cpu]); }

IMPLEMENT
template< typename T >
Per_cpu<T>::Per_cpu()
{
  for (unsigned i = 0; i < _num_cpus; ++i)
    new (&cpu(i)) T();
}

IMPLEMENT
template< typename T >
Per_cpu<T>::Per_cpu(bool) : _d(0)
{
  for (unsigned i = 0; i < _num_cpus; ++i)
    new (&cpu(i)) T(i);
}
