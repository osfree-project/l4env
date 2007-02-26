typedef struct stringlist_node {
	char* content;
	struct stringlist_node* next;
} stringlist_node_t;

typedef struct {
	stringlist_node_t* head;
} stringlist_t;


int fifo_in(stringlist_t* list, char* string);

char* fifo_out(stringlist_t* list);
