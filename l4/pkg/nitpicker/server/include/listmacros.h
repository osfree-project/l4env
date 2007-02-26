/*
 * \brief   Macros for handling connected lists
 * \date    2005-09-09
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 */

/*
 * Copyright (C) 2002-2005  Norman Feske  <nf2@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the DOpE package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#ifndef _NITPICKER_LISTMACROS_H_
#define _NITPICKER_LISTMACROS_H_


/*** MACRO: ADD ELEMENT TO A CONNECTED LIST ***
 *
 * \param first_elem_ptr  pointer to the head pointer of the list
 * \param next            name of successor element of list element type
 * \param at_elem         element at which the new element should be inserted
 *                        or NULL if it should be inserted at the begin
 * \param new_elem        element to add
 */
#define CHAIN_LISTELEMENT(first_elem_ptr, next, at_elem, new_elem) {          \
	if (at_elem == NULL) {                                                    \
		new_elem->next  = (*first_elem_ptr) ? (*first_elem_ptr)->next : NULL; \
		*first_elem_ptr = new_elem;                                           \
	} else {                                                                  \
		new_elem->next = at_elem->next;                                       \
		at_elem->next  = new_elem;                                            \
	}                                                                         \
}


/*** MACRO: REMOVE ELEMENT FROM A CONNECTED LIST ***
 *
 * \param list_type        type of list elements
 * \param first_elem_ptr   pointer to the head pointer of the list
 * \param next             name of successor element of list element type
 * \param elem_to_remove   guess what?
 */
#define UNCHAIN_LISTELEMENT(list_type, first_elem_ptr, next, elem_to_remove) { \
	list_type *curr_elem;                                                      \
                                                                               \
	curr_elem = *first_elem_ptr;                                               \
	if (curr_elem == elem_to_remove) {                                         \
		*first_elem_ptr = curr_elem->next;                                     \
	} else {                                                                   \
		for (; curr_elem && curr_elem->next; curr_elem = curr_elem->next)      \
			if (curr_elem->next == elem_to_remove) {                           \
				curr_elem->next = elem_to_remove->next;                        \
				break;                                                         \
			}                                                                  \
	}                                                                          \
}                                                                              \


#endif /* _NITPICKER_LISTMACROS_H_ */
