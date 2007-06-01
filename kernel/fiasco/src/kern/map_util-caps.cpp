INTERFACE[caps]:

#include "simple_mapdb.h"
#include "mapdb.h"
	// Needed for Mapdb::Frame, which appears in the signature of
	// save_access_attribs(...Cap_space...).
	

IMPLEMENTATION[caps]:

#include "assert.h"
#include "cap_space.h"
#include "obj_space.h"
#include "l4_types.h"
#include "mappable.h"

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

  
inline
void
save_access_attribs (Simple_mapdb* /*mapdb*/, 
    const Simple_mapdb::Frame& /*mapdb_frame*/,
    Mapping* /*mapping*/, Obj_space* /*space*/, 
    unsigned /*page_rights*/, 
    Address /*virt*/, Mappable * /*phys*/, Address /*size*/,
    bool /*me_too*/)
{}

Ipc_err
obj_map(Space * from, L4_fpage const &fp_from,
          Space * to, L4_fpage const &fp_to, Address offs)
{
  Address rcv_addr = Map_traits<Obj_space>::get_addr(fp_to);
  Address snd_addr = Map_traits<Obj_space>::get_addr(fp_from);
  size_t snd_size = fp_from.size();
  size_t rcv_size = fp_to.size();

  // calc size in bytes from power of tows
  Map_traits<Obj_space>::prepare_fpage(snd_addr, snd_size);
  Map_traits<Obj_space>::prepare_fpage(rcv_addr, rcv_size);
  Map_traits<Obj_space>::constraint(snd_addr, snd_size, rcv_addr, rcv_size, 
      offs);

  if (snd_size == 0)
    {
      if (Config::conservative)
	kdb_ke("fpage transfer = nop");
      return Ipc_err(0);
    }

  static Simple_mapdb mdb;

  return map (&mdb,
	      from->obj_space(), from->id(), snd_addr, snd_size,
	      to->obj_space(), to->id(), rcv_addr, 
	      fp_from.grant(), 0, 0);
}

unsigned
obj_fpage_unmap(Space * space, L4_fpage fp, bool me_too, 
    unsigned restriction)
{
  size_t size = fp.size();
  Address addr = Map_traits<Obj_space>::get_addr(fp);
  Map_traits<Obj_space>::prepare_fpage(addr, size);
  Map_traits<Obj_space>::calc_size(size);

  // XXX prevent unmaps when a task has no caps enabled
  static Simple_mapdb mdb;

  return unmap (&mdb, space->obj_space(), space->id(),
		restriction, addr, size, me_too, Unmap_full);
}

bool
map (Mappable *o, Obj_space* to, unsigned to_id, Address rcv_addr,
     unsigned attribs)
{
  typedef Obj_space SPACE;
  if (!to->is_mappable(rcv_addr, 1))
    return 0;

  // Receiver lookup.  
  Obj_space::Phys_addr r_phys;
  Address r_size;
  unsigned r_attribs;

  // Also, look up mapping database entry.  Depending on whether
  // we can overmap, either look up the destination mapping first
  // (and compute the sender mapping from it) or look up the
  // sender mapping directly.
  static Simple_mapdb mapdb;

  if (to->v_lookup(rcv_addr, &r_phys, &r_size, &r_attribs))
    unmap (&mapdb, to, to_id, 0, rcv_addr, r_size,
	true, Unmap_full);

  // Do the actual insertion.
  Obj_space::Status status 
    = to->v_insert(o, rcv_addr, 1, attribs);

  switch (status)
    {
    case SPACE::Insert_warn_exists:
    case SPACE::Insert_warn_attrib_upgrade:
    case SPACE::Insert_ok:

      if (status == SPACE::Insert_ok)
	{
	  if (! o->insert(0, to_id, rcv_addr))
	    {
	      to->v_delete(rcv_addr, 1);
	      return 0;
	    }
	}
      break;

    case SPACE::Insert_err_nomem:
      return 0;
      break;

    case SPACE::Insert_err_exists:
      if (Config::conservative)
	kdb_ke("existing mapping");
      // Do not flag an error here -- because according to L4
      // semantics, it isn't.
    }

  return 1;
}
