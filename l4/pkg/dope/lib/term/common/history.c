/*
 * \brief   History buffer handling
 * \date    2003-03-24
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 *
 * This is a set of functions to manage a command history buffer. It organizes the
 * history entries in a ring buffer of a specified size. The history buffer entries
 * can have different sizes. The strings are stored right after each other to safe
 * memory space.
 */

#include <stdlib.h>

#define MAX(a,b) (a>b?a:b)

struct histentry_struct;
struct histentry_struct {
	struct histentry_struct *next;
	struct histentry_struct *prev;
	char str;
};

struct history_struct {
	long size;
	void *buf;
	struct histentry_struct *first;
	struct histentry_struct *last;
};

struct history_struct *term_history_create(void *buf, long buf_size);
int   term_history_add(struct history_struct *hist, char *new_str);
char *term_history_get(struct history_struct *hist, int index);


/************************/
/*** HELPER FUNCTIONS ***/
/************************/

/*** KICK OUT OLDEST HISTORY BUFFER ENTRY ***/
static long free_entry(struct history_struct *hist) {
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
static long entry_size(struct histentry_struct *he) {
	if (!he) return 0;
	if (!he->str) return sizeof(struct histentry_struct);
	return sizeof(struct histentry_struct) + strlen(&he->str);
}


/*** RETURNS THE BIGGEST FREE BLOCK OF THE HISTORY BUFFER ***/
static long get_avail(struct history_struct *hist) {
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
static struct histentry_struct *alloc_entry(struct history_struct *hist, long size) {
	if (!hist) return NULL;
	if (!hist->first || !hist->last) return hist->buf;
	if (hist->first <= hist->last) {
		long beg_avail = (long)hist->first - (long)hist->buf;
		long end_avail = (long)hist->buf + hist->size 
		               - (long)hist->last - entry_size(hist->last);
		if (end_avail >= size) 
			return (struct histentry_struct *)((long)hist->last + entry_size(hist->last));
		if (beg_avail >= size)
			return hist->buf;
	} else {
		long bet_avail = (long)hist->first - (long)hist->last - entry_size(hist->last);
		if (bet_avail >= size)
			return (struct histentry_struct *)((long)hist->last + entry_size(hist->last));
	}
	return NULL;
}


/***************************/
/*** INTERFACE FUNCTIONS ***/
/***************************/

/*** CREATE HISTORY BUFFER WITH THE SPECIFIED SIZE ***/
struct history_struct *term_history_create(void *buf, long buf_size) {
	struct history_struct *new;

	if (buf) {
		new = (struct history_struct *)buf;
	} else {
		new = (struct history_struct *)malloc(buf_size);
	}
	new->size  = buf_size - sizeof(struct history_struct);
	new->buf   = (void *)((long)new + sizeof(struct history_struct));
	new->first = NULL;
	new->last  = NULL;

	return new;
}


/*** ADD STRING TO HISTORY BUFFER ***/
int term_history_add(struct history_struct *hist, char *newstr) {
	long new_len = strlen(newstr) + sizeof(struct histentry_struct);
	struct histentry_struct *new;
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
char *term_history_get(struct history_struct *hist, int index) {
	struct histentry_struct *curr;
	int i;
	
	if (!hist || !hist->last) return NULL;
	
	curr = hist->last;
	for (i=0; i<index; i++) {
		curr = curr->prev;
		if (!curr) return NULL;
	}
	return &curr->str;
}
