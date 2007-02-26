/*
 * \brief   Interface of the thread abstraction of DOpE
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

#if !defined(THREAD)
#define THREAD void 
#endif

#if !defined(MUTEX)
#define MUTEX void 
#endif

struct thread_services {
	THREAD *(*create_thread) (void (*entry)(void *),void *arg);
	MUTEX  *(*create_mutex)  (int init_locked);
	void    (*destroy_mutex) (MUTEX *);
	void    (*mutex_down)    (MUTEX *);
	void    (*mutex_up)      (MUTEX *);
	s8      (*mutex_is_down) (MUTEX *);
	int     (*ident2thread)  (u8 *thread_ident, THREAD *dst);
};
