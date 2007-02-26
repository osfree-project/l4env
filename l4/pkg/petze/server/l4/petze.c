/*
 * \brief   Petze server
 * \date    2003-07-29
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 *
 * This server can receive log messages of malloc() and free()
 * calls and thus, can provide some statistical information
 * about memory usage of instrumented applications.
 */

/*** GENERAL INCLUDES ***/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*** L4 INCLUDES ***/
#include <l4/log/l4log.h>
#include <l4/names/libnames.h>

/*** LOCAL INCLUDES ***/
#include "petze-server.h"

l4_size_t l4libc_heapsize = 6*1024*1024;
char LOG_tag[] = "petze";

#define u32 unsigned int
#define s32   signed int

#define MAX_PETZ_POOLS   1024
#define MAX_PETZ_CLIENTS 128
#define BLOCKS_PER_CHUNK 256
#define LEN_POOL_IDENT   32

#define PETZ_BLOCK_FREE      0
#define PETZ_BLOCK_ALLOCATED 1

#define PETZ_POOL_FREE      0
#define PETZ_POOL_ALLOCATED 1

struct petz_block;
struct petz_block {
	u32 addr;
	int size;
	int flags;
	struct petz_block *next;
};

struct petz_pool;
static struct petz_pool {
	struct petz_block *first_block;
	struct petz_pool  *next;
	int  flags;
	s32  sum_bytes;
	s32  num_blocks;
	char ident[LEN_POOL_IDENT];
} petz_pool[MAX_PETZ_POOLS];

static struct petz_client {
	l4_threadid_t tid;
	struct petz_pool *first_pool;
} petz_client[MAX_PETZ_CLIENTS];

struct petz_chunk;
struct petz_chunk {
	struct petz_block blocks[BLOCKS_PER_CHUNK];
	struct petz_chunk *next;
	int allocated;
};


static struct petz_chunk *first_chunk;
static int petz_verbose_flag = 0;
static int my_sum_bytes = 0;
static int my_num_blocks = 0;


/*** ALLOCATE A NEW BLOCK ***
 *
 * Create a new block for logging one malloc call.
 */
static struct petz_block *petz_alloc_block(u32 addr, int size) {
	struct petz_chunk *curr_chunk = first_chunk;
	int i;

	/* search a chunk with a free entry */
	while (curr_chunk && (curr_chunk->allocated == BLOCKS_PER_CHUNK)) {
		curr_chunk = curr_chunk->next;
	}

	/* no chunk with free entries found? -> let us create a new one */
	if (!curr_chunk) {
		curr_chunk = (struct petz_chunk *)malloc(sizeof(struct petz_chunk));
		if (curr_chunk) {
			my_sum_bytes += sizeof(struct petz_chunk);
			my_num_blocks++;
			memset(curr_chunk, 0, sizeof(struct petz_chunk));
			curr_chunk->next = first_chunk;
			first_chunk = curr_chunk;
		}
	}
	if (!curr_chunk) {
		printf("Petze(alloc_block): Damn, out of memory. Go and buy some more!\n");
		return NULL;
	}
	
	/* find free entry in chunk */
	for (i=0; i<BLOCKS_PER_CHUNK; i++) {
		if (curr_chunk->blocks[i].flags == PETZ_BLOCK_FREE) {
			curr_chunk->blocks[i].flags = PETZ_BLOCK_ALLOCATED;
			curr_chunk->blocks[i].addr = addr;
			curr_chunk->blocks[i].size = size;
			curr_chunk->allocated++;
			return &curr_chunk->blocks[i];
		}
	}

	printf("Petze(alloc_block): huh? How did you came here?\n");
	return NULL;
}


/*** DEALLOCATE A GIVEN BLOCK FROM ITS CHUNK ***
 *
 * The corresponding chunk is determined via the address range of the
 * chunk.
 */
static void petz_free_block(struct petz_block *block) {
	struct petz_chunk *curr_chunk = first_chunk;
	int i;
	
	if (!block) return;

	/* search chunk that contains the block */
	while (curr_chunk) {
		if (((u32)block >= (u32)curr_chunk) &&
		    ((u32)block <  (u32)curr_chunk + sizeof(struct petz_chunk))) {
			break;
		}
		curr_chunk = curr_chunk->next;
	}

	if (!curr_chunk) {
		printf("Petze(free_block): No chunk containing the block found.\n");
		return;
	}

	/* find block in chunk */
	for (i=0; i<BLOCKS_PER_CHUNK; i++) {
		if (&curr_chunk->blocks[i] == block) {
			block->flags = PETZ_BLOCK_FREE;
			block->size = 0;
			block->addr = 0;
			curr_chunk->allocated--;
			return;
		}
	}

	printf("Petze(free_block): I wonder how did you get here!\n");
}


/*** RETURN BLOCK STRUCTURE BY A GIVEN ADDRESS ***/
static struct petz_block *petz_get_block(struct petz_client *client, u32 addr) {
	struct petz_block *curr_block;
	struct petz_pool  *curr_pool;

	if (!client) return NULL;
	
	curr_pool = client->first_pool;
	while (curr_pool) {
		curr_block = curr_pool->first_block;
		while (curr_block) {
			if (curr_block->addr == addr) return curr_block;
			curr_block = curr_block->next;
		}
		curr_pool = curr_pool->next;
	}
	return NULL;
}


/*** DEQUEUE BLOCK FROM THE LOG STRUCTURE ***/
static void dequeue_block(struct petz_client *client, struct petz_block *block) {
	struct petz_block *curr_block;
	struct petz_pool  *curr_pool;
	s32 block_size;

	if (!client || !block) return;
	
	block_size = block->size;
	curr_pool  = client->first_pool;
	while (curr_pool) {
		curr_block = curr_pool->first_block;
		
		/* is block the first block of the chain? */
		if (curr_block == block) {
			curr_pool->first_block = curr_block->next;
			curr_pool->sum_bytes -= block_size;
			curr_pool->num_blocks--;
			return;
		}
		
		while (curr_block) {
			if (curr_block->next == block) {
				curr_block->next = curr_block->next->next;
				curr_pool->sum_bytes -= block_size;
				curr_pool->num_blocks--;
				return;
			}
			curr_block = curr_block->next;
		}
		curr_pool = curr_pool->next;
	}
	printf("Petze(dequeue block): block not found!\n");
}


/*** RETURN CLIENT STRUCTURE FOR A GIVEN THREADID ***
 *
 * If there exists no client structure for this threadid a new
 * one is created.
 */
static struct petz_client *petz_get_client(l4_threadid_t *tid) {
	int i;
	for (i=0; i<MAX_PETZ_CLIENTS; i++) {
		if (tid->id.task == petz_client[i].tid.id.task) {
			return &petz_client[i];
		}
	}

	/* client is not registered yet - so we have to do that now */
	printf("Petze(get_client): register new client %d.%d\n", tid->id.task, tid->id.lthread);
	for (i=0; i<MAX_PETZ_CLIENTS; i++) {
		if (!petz_client[i].tid.id.task) break;
	}

	if (i >= MAX_PETZ_CLIENTS) {
		printf("Petze(get_client): max. number of clients reached.\n");
	}

	petz_client[i].tid.id.task    = tid->id.task;
	petz_client[i].tid.id.lthread = tid->id.lthread;
	petz_client[i].first_pool     = NULL;

	return &petz_client[i];
}


/*** RETURN POOL STRUCTURE WITH SPECIFIED NAME ***
 *
 * It there exists no such pool for the specified client a new
 * pool is created.
 */
static struct petz_pool *petz_get_pool(struct petz_client *client, const char *poolname) {
	struct petz_pool *curr_pool;
	int i;
	
	if (!client || !poolname) return NULL;
	
	curr_pool = client->first_pool;
	while (curr_pool) {
		if (!strcmp(&curr_pool->ident[0], poolname)) {
			return curr_pool;
		}
		curr_pool = curr_pool->next;
	}

	/* pool does not exist yet - let us create a new one */
	printf("Petze(get_pool): create new pool %s\n",poolname);
	
	for (i=0; i<MAX_PETZ_POOLS; i++) {
		if (petz_pool[i].flags == PETZ_POOL_FREE) break;
	}

	if (i >= MAX_PETZ_POOLS) {
		printf("Petze(get_pool): max. number of pools reached.\n");
		return NULL;
	}

	petz_pool[i].flags = PETZ_POOL_ALLOCATED;
	petz_pool[i].first_block = NULL;
	strncpy(&petz_pool[i].ident[0], poolname, LEN_POOL_IDENT-1);
	petz_pool[i].next = client->first_pool;
	client->first_pool = &petz_pool[i];
	return &petz_pool[i];
}


/********************
 * SERVER FUNCTIONS *
 ********************/

void petze_malloc_component(CORBA_Object _dice_corba_obj,
                            const char* poolname,
                            int address,
                            int size,
                            CORBA_Server_Environment *_dice_corba_env)
{
	struct petz_client *client = petz_get_client(_dice_corba_obj);
	struct petz_pool   *pool   = petz_get_pool(client, poolname);
	struct petz_block  *new;
	
	if (petz_verbose_flag) {
		printf("malloc(size=%d, addr=%x) from %d.%d (%s)\n", 
		       size, address,
		       (int)_dice_corba_obj->id.task,
		       (int)_dice_corba_obj->id.lthread,
		       poolname);
	}

	if (!client || !pool) {
		printf("Petze(malloc_component): invalid pool or client.\n");
		return;
	}
	new = petz_alloc_block(address,size);
	if (!new) {
		printf("Petze(malloc): could not allocate block structure.\n");
		return;
	}
	
	new->next = pool->first_block;
	pool->first_block = new;
	pool->sum_bytes  += size;
	pool->num_blocks++;
}


void petze_free_component(CORBA_Object _dice_corba_obj,
                          const char* poolname,
                          int address,
                          CORBA_Server_Environment *_dice_corba_env)
{
	struct petz_client *client = petz_get_client(_dice_corba_obj);
	struct petz_block  *block  = petz_get_block(client, address);

	if (petz_verbose_flag) {
		printf("free(addr=%x) from %d.%d (%s)\n", 
		       address,
		       (int)_dice_corba_obj->id.task,
		       (int)_dice_corba_obj->id.lthread,
		       poolname);
	}
	
	dequeue_block(client, block);
	petz_free_block(block);
}


/*** DUMP CURRENT STATISTICS VIA PRINTF ***/
void petze_dump_component(CORBA_Object _dice_corba_obj,
                          CORBA_Server_Environment *_dice_corba_env)
{
	int client_idx = 0;
	struct petz_pool *curr_pool;
	
	printf("*** PETZE DUMP ***\n");
	while ((petz_client[client_idx].tid.id.task) && (client_idx<MAX_PETZ_CLIENTS)) {
		printf(" CLIENT %d.%d:\n", petz_client[client_idx].tid.id.task,
				petz_client[client_idx].tid.id.lthread);
		curr_pool = petz_client[client_idx].first_pool;
		while (curr_pool) {
			printf("  %s: %d bytes in %d blocks\n", &curr_pool->ident[0],
			       curr_pool->sum_bytes,
			       curr_pool->num_blocks);
			curr_pool = curr_pool->next;
		}
		client_idx++;
	}
	printf(" %d bytes in %d blocks used to hold statistics\n",
	       my_sum_bytes, my_num_blocks);
	printf("*** END OF PETZE DUMP ***\n");
}


/*** RESET INTERNAL SERVER STATE ***/
void petze_reset_component(CORBA_Object _dice_corba_obj,
                          CORBA_Server_Environment *_dice_corba_env)
{
	struct petz_chunk *curr_chunk, *next_chunk;
	
	/* kill all chunks */
	curr_chunk = first_chunk;
	while (curr_chunk) {
		next_chunk = curr_chunk->next;
		free(curr_chunk);
		my_sum_bytes -= sizeof(struct petz_chunk);
		my_num_blocks--;
		curr_chunk = next_chunk;
	}
	
	memset(&petz_client[0],0,sizeof(struct petz_client)*MAX_PETZ_CLIENTS);
	memset(&petz_pool[0],0,sizeof(struct petz_pool)*MAX_PETZ_POOLS);
	first_chunk = NULL;
}


/****************
 * MAIN PROGRAM *
 ****************/

int main(int argc, char **argv) {
	printf("Petze(main): try to register at names.\n");
	if (!names_register("Petze")) {
		printf("Petze(main): could not register at names - exiting.\n");
		return -1;
	}
	printf("Petze(main): entering server loop...\n");
	petze_server_loop(NULL);
	return 0;
}
