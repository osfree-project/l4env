/*
 * \brief   History buffer handling of DOpE terminal library
 * \date    2003-03-24
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 *
 * This is a set of functions to manage a command history buffer. It organizes the
 * history entries in a ring buffer of a specified size. The history buffer entries
 * can have different sizes. The strings are stored right after each other to safe
 * memory space.
 */

/*
 * Copyright (C) 2002-2003  Norman Feske  <nf2@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the DOpE package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#include <stdlib.h>
#include <string.h>

#define MAX(a,b) (a>b?a:b)

struct histentry;
struct histentry {
	struct histentry *next;
	struct histentry *prev;
	char str;
};

struct history {
	long size;
	void *buf;
	struct histentry *first;
	struct histentry *last;
};

struct history *term_history_create(void *buf, long buf_size);
int   term_history_add(struct history *hist, char *new_str);
char *term_history_get(struct history *hist, int index);


/************************/
/*** HELPER FUNCTIONS ***/
/************************/

/*** KICK OUT OLDEST HISTORY BUFFER ENTRY ***/
static long free_entry(struct history *hist) {
	if (!hist->first) return -1;
	if (!hist->first->next) {
		hist->first = NULL;
		return 0;
	}
	hist->first = hist->first->next;
	hist->first->prev = NULL;
	return 0;
}


/*** DETERMINE THE SIZE OF A GIVEN ENTRY ***/
static long entry_size(struct histentry *he) {
	if (!he) return 0;
	if (!he->str) return sizeof(struct histentry);
	return sizeof(struct histentry) + strlen(&he->str);
}


/*** RETURNS THE BIGGEST FREE BLOCK OF THE HISTORY BUFFER ***/
static long get_avail(struct history *hist) {
	if (!hist) return 0;
	if (!hist->first || !hist->last) return hist->size;
	if (hist->first <= hist->last) {
		long beg_avail = (long)hist->first - (long)hist->buf;
		long end_avail = (long)hist->buf + hist->size 
		               - (long)hist->last - entry_size(hist->last);
		return MAX(beg_avail, end_avail);
	} else {
		long bet_avail = (long)hist->first - (long)hist->last - entry_size(hist->last);
		return bet_avail;
	}
}


/*** ALLOCATES A NEW HISTORY BUFFER ENTRY ***/
static struct histentry *alloc_entry(struct history *hist, long size) {
	if (!hist) return NULL;
	if (!hist->first || !hist->last) return hist->buf;
	if (hist->first <= hist->last) {
		long beg_avail = (long)hist->first - (long)hist->buf;
		long end_avail = (long)hist->buf + hist->size 
		               - (long)hist->last - entry_size(hist->last);
		if (end_avail >= size) 
			return (struct histentry *)((long)hist->last + entry_size(hist->last));
		if (beg_avail >= size)
			return hist->buf;
	} else {
		long bet_avail = (long)hist->first - (long)hist->last - entry_size(hist->last);
		if (bet_avail >= size)
			return (struct histentry *)((long)hist->last + entry_size(hist->last));
	}
	return NULL;
}


/***************************/
/*** INTERFACE FUNCTIONS ***/
/***************************/

/*** CREATE HISTORY BUFFER WITH THE SPECIFIED SIZE ***/
struct history *term_history_create(void *buf, long buf_size) {
	struct history *new;

	if (buf) {
		new = (struct history *)buf;
	} else {
		new = (struct history *)malloc(buf_size);
	}
	new->size  = buf_size - sizeof(struct history);
	new->buf   = (void *)((long)new + sizeof(struct history));
	new->first = NULL;
	new->last  = NULL;

	return new;
}


/*** ADD STRING TO HISTORY BUFFER ***/
int term_history_add(struct history *hist, char *newstr) {
	long new_len = strlen(newstr) + sizeof(struct histentry);
	struct histentry *new;
	if (new_len > hist->size) return -1;

	while (get_avail(hist) <= new_len) free_entry(hist);
	new = alloc_entry(hist, new_len);
	if (!new) return -1;
	
	new->next = NULL;
	new->prev = hist->last;
	strcpy(&new->str,newstr);
	
	if (!hist->first || !hist->last) {
		hist->first = new;
		hist->last  = new;
	} else {
		hist->last->next = new;
		hist->last = new;
	}
	return 0;
}


/*** RETRIEVE HISTORY ENTRY BY A GIVEN INDEX (0 IS THE LAST ENTRY) ***/
char *term_history_get(struct history *hist, int index) {
	struct histentry *curr;
	int i;
	
	if (!hist || !hist->last) return NULL;
	
	curr = hist->last;
	for (i=0; i<index; i++) {
		curr = curr->prev;
		if (!curr) return NULL;
	}
	return &curr->str;
}
