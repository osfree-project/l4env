INTERFACE:

#include "entry_frame.h"

class Long_msg : public Sys_ipc_frame
{
public:
  Mword	msg_word	(unsigned index) const;
  Mword	msg_word	(void *_msg, unsigned index) const;
  void	set_msg_word	(unsigned index, Mword value);
  void	set_msg_word	(void *_msg, unsigned index, Mword value);
};

IMPLEMENTATION:

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
