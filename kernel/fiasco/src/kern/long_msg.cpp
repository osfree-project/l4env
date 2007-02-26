INTERFACE:

#include "entry_frame.h"

class Long_msg : public Sys_ipc_frame
{
public:
  Mword	msg_word (unsigned index) const;
  Mword	msg_word (Mword *msg, unsigned index) const;
  void	set_msg_word (unsigned index, Mword value);
  void	set_msg_word (Mword *msg, unsigned index, Mword value);
};

//---------------------------------------------------------------------------
IMPLEMENTATION:

#include "space.h"

IMPLEMENT inline
Mword
Long_msg::msg_word (unsigned index) const
{
  return Sys_ipc_frame::msg_word (index);
}

IMPLEMENT inline
void
Long_msg::set_msg_word (unsigned index, Mword value)
{
  Sys_ipc_frame::set_msg_word (index, value);
}

IMPLEMENT inline NEEDS ["space.h"]
Mword
Long_msg::msg_word (Mword *msg, unsigned index) const
{
  if (index < num_snd_reg_words())
    return Sys_ipc_frame::msg_word (index);

  return current_mem_space()->peek_user (msg + index + 3);
}

IMPLEMENT inline NEEDS ["space.h"]
void
Long_msg::set_msg_word (Mword *msg, unsigned index, Mword value)
{
  if (index < num_rcv_reg_words())
    Sys_ipc_frame::set_msg_word (index, value);

  current_mem_space()->poke_user (msg + index + 3, value);
}
