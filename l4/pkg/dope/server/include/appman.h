/*
 * \brief   Interface of DOpE application manager module
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

#if !defined(HASHTAB)
#define HASHTAB void
#endif

#if !defined(THREAD)
#define THREAD void
#endif


struct appman_services {
	s32      (*reg_app)        (const char *app_name);
	s32      (*unreg_app)      (u32 app_id);
	HASHTAB *(*get_widgets)    (u32 app_id);
	HASHTAB *(*get_variables)  (u32 app_id);
	void     (*reg_listener)   (s32 app_id,THREAD *listener);
	THREAD  *(*get_listener)   (s32 app_id);
	char    *(*get_app_name)   (s32 app_id);
	void     (*reg_app_thread) (s32 app_id,THREAD *app_thread);
	THREAD  *(*get_app_thread) (s32 app_id);
};
