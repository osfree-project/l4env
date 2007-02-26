#include <malloc.h>
#include <l4/lock/lock.h>
#include "stringlist.h"

l4lock_t lock = L4LOCK_UNLOCKED;

int fifo_in(stringlist_t* list, char* string) {
	stringlist_node_t* p;

	stringlist_node_t* newnode = malloc(sizeof(stringlist_node_t));
	if (! newnode) return 0;

	newnode->content = malloc(strlen(string)+1);
	if (! newnode->content) {
		free(newnode);
		return 0;
	}
	strcpy(newnode->content, string);
	newnode->next=0;

	l4lock_lock(&lock);
	if (! list->head) {
		list->head=newnode;
	} else {
		for (p=list->head; p->next; p=p->next);
		p->next=newnode;
	}
	l4lock_unlock(&lock);
	return 1;
}

char* fifo_out(stringlist_t* list) {
	char* res;

	l4lock_lock(&lock);
	if (! list->head) return 0;

	res=list->head->content;
	list->head=list->head->next;
	l4lock_unlock(&lock);

	return res;
}
