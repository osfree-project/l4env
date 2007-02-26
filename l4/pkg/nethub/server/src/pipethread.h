/* -*- c++ -*- */
#ifndef PIPETHREAD_H__
#define PIPETHREAD_H__

#include <l4/cxx/thread.h>
#include <l4/sys/types.h>

// prevent oskit10 from defining wchar_t
#define _WCHAR_T
#include <stddef.h>

class PipeThread : public L4::Thread
{
 private:
  l4_threadid_t _rcv,_snd;
  l4_umword_t stack[1024];
  char msg_buffer[4096];

 public:
  
  void *operator new( size_t );
  
  PipeThread( unsigned tid);
  void run();
  void rcv( l4_threadid_t i ) { _rcv = i; }
  void snd( l4_threadid_t i ) { _snd = i; }
  bool is_ready() 
  { 
    return !l4_is_invalid_id(_rcv) && !l4_is_invalid_id(_snd); 
  }

  bool has_sender() { return !l4_is_invalid_id(_snd); }
  bool has_receiver() { return !l4_is_invalid_id(_rcv); }
};


#endif  //PIPETHREAD_H__
