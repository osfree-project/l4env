/*
 * \brief   Interface of hash table module
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

struct hashtab_services {
	HASHTAB *(*create)      (u32 tab_size,u32 max_hash_length);
	void     (*destroy)     (HASHTAB *h);
	void     (*add_elem)    (HASHTAB *h,char *ident,void *value);
	void    *(*get_elem)    (HASHTAB *h,char *ident);
	void     (*remove_elem) (HASHTAB *h,char *ident);
};
