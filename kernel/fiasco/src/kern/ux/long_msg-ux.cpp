IMPLEMENTATION[ux]:

#include "thread.h"

IMPLEMENT inline NEEDS ["thread.h"]
Mword
Long_msg::msg_word (void *_msg, unsigned index) const
{
  if (index < num_reg_words())
    return Sys_ipc_frame::msg_word (index);

  Mword *msg = (Mword *) _msg + 3;	// skip header

  return current_thread()->peek_user (msg + index);
}

IMPLEMENT inline NEEDS ["thread.h"]
void
Long_msg::set_msg_word (void *_msg, unsigned index, Mword value)
{
  if (index < num_reg_words())
    Sys_ipc_frame::set_msg_word (index, value);

  Mword *msg = (Mword *) _msg + 3;	// skip header

  current_thread()->poke_user (msg + index, value);
}
