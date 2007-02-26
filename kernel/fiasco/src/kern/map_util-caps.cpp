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
cap_map(Space * from, Address fp_from_task, Mword fp_from_size,
       bool fp_from_grant, bool fp_from_is_whole_space,
       Space * to,   Address fp_to_task,   Mword fp_to_size,
       bool fp_to_is_cappage, bool fp_to_is_whole_space)
{
  // if ordinary memory fpage on receiver but not complete address space
  // then do nothing
  if(!fp_to_is_cappage && !fp_to_is_whole_space)
    return Ipc_err (0);

  // if fp_from == whole address space
  // than try to map the full CAP address space
  if(fp_from_is_whole_space)
    {
      fp_from_task = 0;
      fp_from_size = L4_fpage::Whole_cap_space;
    }

  // compute end of sender window
  Address snd_size = fp_from_size < L4_fpage::Whole_cap_space 
    			? fp_from_size 
			: (Mword)L4_fpage::Whole_cap_space;

  Address snd_end  = fp_from_task+(1L<<snd_size) < L4_fpage::Cap_max
                    	? fp_from_task+(1L<<snd_size) 
			: (Address)L4_fpage::Cap_max;
  Address snd_pos  = fp_from_task;

  if(fp_to_is_cappage)		// valid CAP page for receiver ?
    {				// need to adjust snd_pos & snd_end
      // snd_pos : take the max of fp_from & fp_to
      if(snd_pos < fp_to_task)
	snd_pos = fp_to_task; 

      Address rcv_win =	fp_to_size < L4_fpage::Whole_cap_space
				? (1L << fp_to_size) 
				: (Address)L4_fpage::Cap_max;

      // snd_end : take min of fp_from & fp_to
      if(snd_end > fp_to_task + rcv_win)
	snd_end = fp_to_task + rcv_win;
    }

  assert(snd_pos < L4_fpage::Cap_max);

  return map (cap_mapdb_instance(),
	      from->cap_space(), from->id(), snd_pos, snd_end - snd_pos,
	      to->cap_space(), to->id(), snd_pos, 
	      fp_from_grant, 0, 0);
}

unsigned
cap_fpage_unmap(Space * space, L4_fpage fp, bool me_too, unsigned restriction)
{
  Address task, size;

  if (fp.is_cappage())
    {
      // try to unmap our own fpage
      size = fp.size() < L4_fpage::Whole_cap_space
	? (1L << fp.size()) 
	: (Address)L4_fpage::Cap_max;

      task = fp.task();
    }
  else
    {
      assert (fp.is_whole_space());
      task = 0;
      size = (Address)L4_fpage::Cap_max;;
    }

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
