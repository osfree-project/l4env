INTERFACE[ux && segments]:

EXTENSION class Context
{
protected:
  struct { Unsigned64 v[2]; } _gdt_user_entries[3]; // type struct Ldt_user_desc
  Unsigned32                  _es, _fs, _gs;
};

// ---------------------------------------------------------------------
IMPLEMENTATION[ux]:

IMPLEMENT inline
void
Context::switch_fpu (Context *)
{}

// ---------------------------------------------------------------------
IMPLEMENTATION[ux && segments]:

PROTECTED inline
void
Context::switch_gdt_user_entries(Context *to)
{
  Mword *trampoline_page = (Mword *) Kmem::phys_to_virt(Mem_layout::Trampoline_frame);

  for (int i = 0; i < 3; i++)
    if (to == this
	|| _gdt_user_entries[i].v[0] != to->_gdt_user_entries[i].v[0]
        || _gdt_user_entries[i].v[1] != to->_gdt_user_entries[i].v[1])
      {
        memcpy(trampoline_page + 1, &to->_gdt_user_entries[i],
               sizeof(_gdt_user_entries[0]));
        Trampoline::syscall(to->space()->pid(), 243,
                            Mem_layout::Trampoline_page + sizeof(Mword));
      }
}
