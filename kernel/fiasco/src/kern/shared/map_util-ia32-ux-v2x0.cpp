INTERFACE:

IMPLEMENTATION[ia32-ux-v2x0]:

/** Flexpage mapping.
    divert to mem_map (for memory fpages) or io_map (for IO fpages)
    @param from source address space
    @param fp_from flexpage descriptor for virtual-address space range
	in source address space
    @param to destination address space
    @param fp_to flexpage descriptor for virtual-address space range
	in destination address space
    @param offs sender-specified offset into destination flexpage
    @pre page_aligned(offs)
    @return IPC error code that describes the status of the operation
*/
inline NEEDS ["config.h"]
L4_msgdope fpage_map(Space *from, L4_fpage fp_from, Space *to, 
		     L4_fpage fp_to, Address offs)

{
  L4_msgdope result(0);

  if(Config::enable_io_protection &&
     (fp_from.is_iopage()       // an IO flex page ?? or everything ??
      || fp_from.is_whole_space()))
    {
      result.combine(io_map(from, 
			    fp_from.iopage(),
			    fp_from.size(),
			    fp_from.grant(),
			    fp_from.is_whole_space(),
			    to, 
			    fp_to.iopage(),
			    fp_to.size(),
			    fp_to.is_iopage(),
			    fp_to.is_whole_space()));
    }

  if(!Config::enable_io_protection || !fp_from.is_iopage())
    {
      result.combine (mem_map (from, 
			       fp_from.page(), 
			       fp_from.size(),
			       fp_from.write(),
			       fp_from.grant(), to, 
			       fp_to.page(),
			       fp_to.size(), offs)
		      );
    }
  return result;
}

