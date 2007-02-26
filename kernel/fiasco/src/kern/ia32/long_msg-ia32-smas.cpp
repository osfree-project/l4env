/** For small address spaces this class assumes that the pointers in
 *  the long message refer to addresses in user space. Therefore they
 *  have to be accessed via FS segment.
 */
IMPLEMENTATION[ia32-smas]:

IMPLEMENT inline
Mword
Long_msg::msg_word (void *_msg, unsigned index) const
{
  if (index < num_reg_words())
    return Sys_ipc_frame::msg_word (index);

  register Mword ret;

  asm volatile ("movl %%fs:(%1), %%eax"
                : "=a" (ret)  
                : "r" ((Mword *) _msg + 3 + index * sizeof (Mword)));
  return ret;
}

// is never used so do not warn anymore just don't implement it
#if 0
IMPLEMENT inline
void
Long_msg::set_msg_word (void */*_msg*/, unsigned /*index*/, Mword /*value*/)
{
# warning Missing implementation for accessing user space
}
#endif
