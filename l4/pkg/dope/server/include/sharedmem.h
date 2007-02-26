/*
 * \brief   Interface of shared memory abstraction of DOpE
 * \date    2002-11-13
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 */

/*
 * Copyright (C) 2002-2003  Norman Feske  <nf2@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the DOpE package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#if !defined(SHAREDMEM)
#define SHAREDMEM struct shared_memory
#endif

#if !defined(THREAD)
#define THREAD void 
#endif

struct shared_memory;

struct sharedmem_services {
	SHAREDMEM *(*alloc)       (long size);
	void       (*destroy)     (SHAREDMEM *sm);
	void      *(*get_address) (SHAREDMEM *sm);
	void       (*get_ident)   (SHAREDMEM *sm, u8 *dst_ident_buf);
	s32        (*share)       (SHAREDMEM *sm, THREAD *dst_thread);
};
