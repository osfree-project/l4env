INTERFACE:

#include "config.h"
#include "types.h"

// Scheduling parameters
class sched_t
{
public:
  Unsigned64 get_total_cputime() const;

private:
  unsigned short _prio;
  unsigned short _mcp;
  unsigned short _timeslice, _ticks_left;
  unsigned long  _total_ticks;
  unsigned long _start_cputime;
  unsigned long long _total_cputime;
protected:
  sched_t *_prev;
  sched_t *_next;
};

IMPLEMENTATION:

#include "cpu.h"

PUBLIC
sched_t::sched_t(void)
{
  _prev = _next = 0;
}

PUBLIC inline
sched_t *
sched_t::next()
{
  return _next;
}

PUBLIC inline
sched_t *
sched_t::prev()
{
  return _prev;
}

PUBLIC inline
void
sched_t::set_next(sched_t *next)
{
  _next = next;
}

PUBLIC inline
void
sched_t::set_prev(sched_t *prev)
{
  _prev = prev;
}

PUBLIC inline
void
sched_t::enqueue_before(sched_t *sibling)
{
  _next = sibling;
  _prev = sibling->prev();
  sibling->set_prev(this);
  _prev->set_next(this);
}

PUBLIC inline
void
sched_t::dequeue()
{
  _prev->set_next(_next);
  _next->set_prev(_prev);

  _next /* = _prev */ = 0;

}

PUBLIC inline 
unsigned short 
sched_t::prio() const
{ 
  return _prio; 
}

PUBLIC inline 
void
sched_t::set_prio(unsigned short prio)
{ 
  _prio = prio; 
}

PUBLIC inline 
unsigned short 
sched_t::mcp() const
{ 
  return _mcp; 
}

PUBLIC inline 
void
sched_t::set_mcp(unsigned short mcp)
{ 
  _mcp = mcp; 
}

PUBLIC inline 
unsigned short 
sched_t::timeslice() const
{ 
  return _timeslice; 
}

PUBLIC inline 
void
sched_t::set_timeslice(unsigned short timeslice)
{ 
  _timeslice = timeslice; 
}

PUBLIC inline 
unsigned short 
sched_t::ticks_left() const
{ 
  return _ticks_left; 
}

PUBLIC inline 
void
sched_t::set_ticks_left(unsigned short ticks_left)
{ 
  _ticks_left = ticks_left; 
}

PUBLIC inline
void
sched_t::reset_cputime(void)
{
  if (Config::fine_grained_cputime)
    _total_cputime = 0;
  else
    _total_ticks = 0;
}

PUBLIC inline
void
sched_t::update_cputime(void)
{
  _total_ticks++;
}

PUBLIC inline
void
sched_t::start_thread(unsigned long tsc_32)
{
  _start_cputime = tsc_32;
}

PUBLIC inline
void
sched_t::stop_thread(unsigned long tsc_32)
{
  _total_cputime += tsc_32 - _start_cputime;
}


