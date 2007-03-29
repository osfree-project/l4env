INTERFACE[caps]:

#include "mapdb.h"
	// Needed for Mapdb::Frame, which appears in the signature of
	// save_access_attribs(...Cap_space...).
	

IMPLEMENTATION[caps]:

#include "assert.h"
#include "cap_space.h"
#include "l4_types.h"

Mapdb*
cap_mapdb_instance()
{
  static const size_t cap_page_sizes[] 
    = {Cap_space::Map_superpage_size, Cap_space::Map_page_size};

  static Mapdb mapdb (Cap_space::Map_max_address / cap_page_sizes[0], 
		      cap_page_sizes, 2);

  return &mapdb;
}

Ipc_err
cap_map(Space * from, L4_fpage const &fp_from,
       Space * to, L4_fpage const &fp_to)
{
  Address rcv_pos, snd_pos;
  rcv_pos = Map_traits<Cap_space>::get_addr(fp_to);
  snd_pos = Map_traits<Cap_space>::get_addr(fp_from);
  size_t rcv_size = fp_to.size();
  size_t snd_size = fp_from.size();

  Map_traits<Cap_space>::prepare_fpage(snd_pos, snd_size);
  Map_traits<Cap_space>::prepare_fpage(rcv_pos, rcv_size);
  Map_traits<Cap_space>::constraint(snd_pos, snd_size, rcv_pos, rcv_size,0);

  if (snd_size == 0)
    return Ipc_err(0);

  assert(snd_pos < L4_fpage::Cap_max);

  return map (cap_mapdb_instance(),
	      from->cap_space(), from->id(), snd_pos, snd_size,
	      to->cap_space(), to->id(), snd_pos, 
	      fp_from.grant(), 0, 0);
}

unsigned
cap_fpage_unmap(Space * space, L4_fpage fp, bool me_too, unsigned restriction)
{
  size_t size = fp.size();
  Address task = Map_traits<Cap_space>::get_addr(fp);
  Map_traits<Cap_space>::prepare_fpage(task, size);
  Map_traits<Cap_space>::calc_size(size);


  // XXX prevent unmaps when a task has no caps enabled

  return unmap (cap_mapdb_instance(), space->cap_space(), space->id(),
		restriction, task, size, me_too, Unmap_full);
}

inline
void
save_access_attribs (Mapdb* /*mapdb*/, const Mapdb::Frame& /*mapdb_frame*/,
		     Mapping* /*mapping*/, Cap_space* /*space*/, 
		     unsigned /*page_rights*/, 
		     Address /*virt*/, Address /*phys*/, Address /*size*/,
		     bool /*me_too*/)
{}

