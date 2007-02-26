/*!
 * \file   dietlibc/lib/backends/l4env_base/buddy.c
 * \brief  Buddy implementation
 *
 * \date   08/16/2003
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *
 * This file implements a buddy system for memory allocation. The buddies
 * have a minimum size, L4BUDDY_BUDDY_SIZE. Chunks of smaller sizes are
 * allocated using external slab implementations.
 *
 * Structures for buddy trees are held within unallocated portions of the
 * tree. If all memory of a subtree is occupied, no structures are needed
 * (the fact that everything is alloacted is stored in the upper node of
 * the tree). The only external information is the size of a chunk handed
 * out by the buddy system, which is needed upon freeing a chunk. As no
 * internal data is needed, chunks are always size-aligned and can be
 * allocated dense.
 */

#include <assert.h>
#include <stdlib.h>
#include <l4/util/bitops.h>
#include "buddy.h"
#include "config.h"

#if DEBUG_BUDDY
#include <stdio.h>
#include <string.h>
#endif

#include <l4/log/l4log.h>

#define max(a,b) ((a)>(b)?(a):(b))
#define min(a,b) ((a)<(b)?(a):(b))

struct l4buddy_t{
    l4buddy_t *left, *right;
    unsigned max_free;
};

struct l4buddy_root{
    unsigned char	*base_addr;	/* base address of memory pool	*/
    unsigned		size;		/* the size of the buddy tree	*/
    unsigned		size_ld;	/* log2(size_ld),	 rounded up */
    int			big_pool;	/* 1 if size>=16MB, 0 else	*/
    unsigned char 	*size_map;	/* maps a chunk at addr X to a its
					 * size.
					 * chunk_nr = (X-base_addr)/BUDDY_SIZE
					 *
					 * if !big_pool: 4-bit data:
					 *   size of chunk = 
					 *     1<< ( size_map[chunk_nr/2] )
					 * if big_pool: 8-bit data:
					 *   size of chunk = 
					 *     1<< ( size_map[chunk_nr/2] )
					 */
    l4buddy_t		rootbuddy;	/* the root buddy, having all the
					 * memory in it */
#if DEBUG_BUDDY
    unsigned 		free;
    unsigned 		allocated;
#endif
};

typedef struct{
    unsigned char a:4;
    unsigned char b:4;
} fourbitfield;

extern inline void set_buddy_size_ld(l4buddy_root *root, unsigned off,
				     unsigned size_ld);
extern inline int get_buddy_size_ld(l4buddy_root *root, unsigned off);
extern inline l4buddy_t* new_free_buddy(l4buddy_root*root, unsigned buddynr,
					int index);
extern inline l4buddy_t* new_allocated_buddy(l4buddy_root*root,
					     int index, l4buddy_t *free);
extern inline l4buddy_t *copy_buddy(l4buddy_t *buddy, int index, 
				    l4buddy_t *free);
extern inline void* split_and_alloc(unsigned size, unsigned size_ld,
				    l4buddy_root *root, l4buddy_t *list[]);
extern inline void join_and_move_buddy(l4buddy_t *list[], int index,
				       unsigned char*addr, unsigned size);
extern inline int split_and_free_buddy(void *addr, unsigned off,
				       unsigned size, unsigned size_ld,
				       l4buddy_root *root, l4buddy_t *list[]);
extern inline void join_freed_buddies(l4buddy_root *root,
				      l4buddy_t *list[], int i);

/*!\brief Create a new buddy tree
 *
 * \param base_addr	base address of the memory pool. Should be
 *			aligned to L4BUDDY_BUDDY_SIZE (1KB).
 * \param size		size of the memory pool. Should be at least
 *			L4BUDDY_BUDDY_SIZE*2 (2KB).
 *			Overhead: sizeof(l4buddy_root) + size(size_map).
 *			size_map: if size<32MB: size/2048
 *				  if size>32MB: size/1024
 *
 * The information is stored at the end. As such, allocated data
 * is aligned rel. to the base address
 */
l4buddy_root* l4buddy_create(void*base_addr, unsigned size){
    l4buddy_root *root;
    unsigned overhead;

    overhead = sizeof(l4buddy_root);
    if(overhead>=size) return 0;

    root = (l4buddy_root*)((unsigned char*)base_addr+size-overhead);

    /* ensure base_addr is a multiple of BUDDY_SIZE */
    root->base_addr = (unsigned char*)( ((unsigned)base_addr +
					 L4BUDDY_BUDDY_MASK) &
					~L4BUDDY_BUDDY_MASK);
    size-=root->base_addr - (unsigned char*)base_addr;
    size &= ~L4BUDDY_BUDDY_MASK;

    /* we could use the compressed form for up to 32MB, but we safe
       the 15 for debugging purposes */
    if(size > L4BUDDY_BUDDY_SIZE * (1<<14)) {
	root->big_pool = 1;
	overhead+= (size+L4BUDDY_BUDDY_SIZE-1)/L4BUDDY_BUDDY_SIZE;
    } else {
	root->big_pool = 0;
	overhead+= ((size+L4BUDDY_BUDDY_SIZE-1)/L4BUDDY_BUDDY_SIZE + 1 )/2;
    }
    root->size_map = root->base_addr + size - overhead;

    /* some space left? */
    if(root->size_map <= root->base_addr)
	return 0;

    root->size = ( ((unsigned char*)root->size_map - root->base_addr) &
		   ~L4BUDDY_BUDDY_MASK);
    root->size_ld = l4util_log2(root->size);
    if(root->size & ((1<<root->size_ld)-1)) root->size_ld++;
    root->rootbuddy.max_free = 1 << l4util_log2(root->size);
    root->rootbuddy.right = root->rootbuddy.left = 0;
#if DEBUG_BUDDY
    root->free = root->size;
    root->allocated = 0;
    memset(root->size_map, -1,
	   root->big_pool?((root->size+L4BUDDY_BUDDY_SIZE-1)/
			   L4BUDDY_BUDDY_SIZE + 1)
	   : ((root->size+L4BUDDY_BUDDY_SIZE-1)/L4BUDDY_BUDDY_SIZE + 1)/2);
#endif
    return root;
}

/*!\brief Store the size of a buddy in the size-map */
extern inline void set_buddy_size_ld(l4buddy_root *root, unsigned off,
				     unsigned size_ld){
    if(root->big_pool){
	root->size_map[off>>L4BUDDY_BUDDY_SHIFT]=size_ld-L4BUDDY_BUDDY_SHIFT;
    } else {
	fourbitfield *x = (fourbitfield*)(root->size_map +
					  (off>>(L4BUDDY_BUDDY_SHIFT+1)));
	if(off&L4BUDDY_BUDDY_SIZE) x->a = size_ld-L4BUDDY_BUDDY_SHIFT;
	else x->b = size_ld-L4BUDDY_BUDDY_SHIFT;
    }
}

/*!\brief Get the size of a buddy from the size-map */
extern inline int get_buddy_size_ld(l4buddy_root *root, unsigned off){
    unsigned size_ld;

    if(root->big_pool){
	size_ld = root->size_map[off>>L4BUDDY_BUDDY_SHIFT];
    } else {
	fourbitfield *x = (fourbitfield*)(root->size_map +
					  (off>>(L4BUDDY_BUDDY_SHIFT+1)));
	if(off&L4BUDDY_BUDDY_SIZE) size_ld = x->a;
	else size_ld = x->b;
    }
    return size_ld+L4BUDDY_BUDDY_SHIFT;
}


/*!\brief write a new buddy node
 *
 * \param root		buddy root
 * \param off		offset in pool where the new buddy will be written to
 * \param index		depth-index of the buddy.
 *
 * This function adaptt for non-aligned poolsize.
 */
extern inline l4buddy_t* new_free_buddy(l4buddy_root*root, unsigned off,
					int index){
    l4buddy_t *b;

    LOGd_Enter(LOG_BUDDY_ALLOC,
	       "off=%d, index=%d", off, index);
    b =(l4buddy_t*)(root->base_addr + off) + index;
    LOGd(LOG_BUDDY_ALLOC, "root->base_addr at %p, b at %p", root->base_addr, b);
    b->max_free = 1<< min((root->size_ld-index), l4util_log2(root->size-off));
    
    b->left = b->right = 0;
    return b;
}

/*!\brief write a new allocated buddy node
 *
 * \param root		buddy root
 * \param index		depth-index of the buddy.
 * \param free		location where the buddy-node can be stored
 */
extern inline l4buddy_t* new_allocated_buddy(l4buddy_root*root,
					     int index, l4buddy_t *free){
    l4buddy_t *b;

    LOGd_Enter(LOG_BUDDY_FREE, "index=%d", index);
    b =(l4buddy_t*)((unsigned)free & ~L4BUDDY_BUDDY_MASK) + index;
    b->max_free = 0;
    b->left = b->right = 0;
    return b;
}

/*!\brief Copy a buddy-node from one buddy to another one
 *
 * \param buddy		Buddy-node to copy
 * \param index		depth-index
 * \param free		free buddy, target, must be aligned
 */
extern inline l4buddy_t *copy_buddy(l4buddy_t *buddy, int index,
				    l4buddy_t *free){
    free = (l4buddy_t*)((unsigned)free & ~L4BUDDY_BUDDY_MASK)+index;
    *free = *buddy;
    return free;
}

/*!\brief Split and allocate a buddy
 *
 * \param size		size of the chunk to allocate
 * \param size_ld	log2(size), rounded up
 * \param root		buddy tree
 * \param list	        ptr to a list of buddies, will be filled with the
 *			path from the root to the newly allocated buddy
 * \return		address of the newly allocated chunk.
 */
extern inline void* split_and_alloc(unsigned size, unsigned size_ld,
				    l4buddy_root *root, l4buddy_t *list[]){
    l4buddy_t *b;
    unsigned off=0;	/* offset of chunk in pool */
    int i;		/* depth-index */

    LOGd_Enter(LOG_BUDDY_ALLOC,
	       "size=%d, size_ld=%d, root=%p, list=%p",
	       size, size_ld, root, list);
    b = &root->rootbuddy;

    for(i=0; i < root->size_ld - size_ld; i++){
	LOGd(LOG_BUDDY_ALLOC,"  i=%d/%d, off=%d", i, root->size_ld-size_ld-1, off);
	assert(b->max_free>=size);
	list[i] = b;

	if(b->left){
	    /* left already exists */
	    if(b->left->max_free >= size &&
	       /* optimization for best-fit: */
	       !( b->right && b->right->max_free < b->left->max_free &&
		  b->right->max_free>=size)
	       ){
		/* go left */
		LOGd(LOG_BUDDY_ALLOC, "    going left");
		b = b->left;
		continue;
	    } else {
		/* go right, there must be enough space */
		LOGd(LOG_BUDDY_ALLOC, "    going right");
		assert(b->right);
		assert(b->right->max_free >= size);
		off += 1 << (root->size_ld-i-1);
		b = b->right;
		continue;
	    }
	} else if(b->right){
	    // it must: b->right->max_free >= size
	    /* go right, w/o split */
	    assert(b->right->max_free >= size);
	    b = b->right;
	    off += 1 << (root->size_ld-i-1);
	    LOGd(LOG_BUDDY_ALLOC, "    going right");
	    continue;
	} else {
	    LOGd(LOG_BUDDY_ALLOC, "    split");
	    /* both unset, split and do best_fit */
	    LOGd(LOG_BUDDY_ALLOC, "      generating left buddy");
	    b->left  = new_free_buddy(root, off, i+1);

	    /* create right node only if there is memory for */
	    if(off + (1<<(root->size_ld-i-1)) < root->size){
		LOGd(LOG_BUDDY_ALLOC, "      generating right buddy");
		b->right = new_free_buddy(root,
					  off+ (1 << (root->size_ld-i-1)),
					  i+1);
	    }
	    if(b->right && b->right->max_free < b->left->max_free &&
	       b->right->max_free >= size){
		LOGd(LOG_BUDDY_ALLOC, "    going right");
		off += 1 << (root->size_ld-i-1);
		b = b->right;
		continue;
	    } else {
		LOGd(LOG_BUDDY_ALLOC, "    going left");
		b = b->left;
		continue;
	    }
	}
    }
    /* b contains an empty buddy having exactly the size we need */
    assert(b->max_free == size);
    b->max_free = 0;	// bloat
    list[i] = b;	// bloat due to function separation

    /* fill in the size map */
    set_buddy_size_ld(root, off, size_ld);

    LOGd(LOG_BUDDY_ALLOC, "  returning offset %d (%p)", off, root->base_addr+off);
    return (void*)(root->base_addr + off);
}

/*!\brief Join, adapt max_free and optionally move buddy nodes
 *
 * \param list	        buddies in the tree directed to buddy, 0..index.
 *			last entry points to the newly allocated buddy.
 * \param index		depth-index of the buddy in the tree
 * \param addr		address of the newly allocated chunk
 * \param size		size of the newly allocated chunk
 *
 * We start joining buddies, going up. Once a buddy is not fully occupied,
 * we won't join any longer: If free is 0, then the buddy down the tree (b)
 * is full and we eliminate it.
 * If free is !0, the current buddy has some capacities, and free points to
 * a free buddy within the current subtree. The current buddy-ptr might be
 * in the newly allocated chunk, and we might have to move it. Then, b
 * will contain its new address.
 */
extern inline void join_and_move_buddy(l4buddy_t *list[], int index,
				       unsigned char*addr, unsigned size){
    l4buddy_t *b;	// buddy down the tree
    l4buddy_t *b_old;	// old address of b, for left/right decision
    l4buddy_t *free=0;	// if 0, all buddies are occupied, join them
    l4buddy_t *b1;	// current buddy

    LOGd_Enter(LOG_BUDDY_ALLOC, "index=%d, addr=%p, size=%d", index, addr, size);
    b = list[index];
    b_old = b;
    for(index--; index>=0; index--){
	b1 = list[index];
	if(b1->left == b_old){	/* buddy was left */
	    if(!free){		/* still joining */
		b1->left = 0;
		free = b1->right;
	    } else if(b){
		b1->left = b;
	    }
	} else {		/* buddy was right */
	    if(!free){ 		/* still joining */
		b1->right = 0;
		free = b1->left;
	    } else if(b){
		b1->right = b;
	    }
	}
	if(free || index==0){
	    b1->max_free = max(b1->left?b1->left->max_free:0,
			       b1->right?b1->right->max_free:0);
	}
	LOGd(LOG_BUDDY_ALLOC, "free=%p, b1=%p, addr=%p, b1+size=%p",
	    free, b1, addr, (unsigned char*)b1+size);
	if(free && addr<=(unsigned char*)b1 && (unsigned char*)b1<addr+size){
	    /* b1 is in the allocated chunk, but won't be joined away.
	       move it to the free area */
	    b = copy_buddy(b1, index, free);
	} else  b = 0;
	b_old = b1;
    }
}

/*!\brief Allocate memory from the buddy pool
 */
void* l4buddy_alloc(l4buddy_root*root, unsigned size){
    l4buddy_t **list;
    int size_ld;	// log2(size), rounded up
    void *addr;

    LOGd_Enter(LOG_BUDDY_CALL_ALLOC, "root=%p, size=%d", root, size);
    if(!size) return 0;

    if(size<L4BUDDY_BUDDY_SIZE){
	size = L4BUDDY_BUDDY_SIZE;
	size_ld = L4BUDDY_BUDDY_SHIFT;
    } else {
	/* Get log2(size), adapt size to 2^n */
	size_ld = l4util_log2(size);
	if(size & ((1<<size_ld)-1)) size_ld++;
	size = 1<<size_ld;
    }
    if(root->rootbuddy.max_free<size) return 0;

    list = alloca(sizeof(l4buddy_t*) * (root->size_ld-size_ld+1));

    addr = split_and_alloc(size, size_ld, root, list);
    join_and_move_buddy(list, root->size_ld - size_ld, addr, size);
#if DEBUG_BUDDY
    root->allocated += size;
    root->free-=size;
    {
	int i;
	for(i=0; i<size; i+=8){
	    memcpy((unsigned char*)addr+i, "buddyall", 8);
	}
    }
#endif
    return addr;
}

/*!\brief Split the buddy-tree if necessary and hang in free buddy
 */
extern inline int split_and_free_buddy(void *addr, unsigned off,
				       unsigned size, unsigned size_ld,
				       l4buddy_root *root, l4buddy_t *list[]){
    int i;
    l4buddy_t *b;
    l4buddy_t **bp;

    b= &root->rootbuddy;
    for(i=0; i < root->size_ld - size_ld; i++){
	list[i]=b;
	if(b->max_free<size) b->max_free = size;
	/* The offset determines left/right */
	bp = (off & (1<<(root->size_ld-i-1))) ? &b->right: &b->left;
	if(!*bp){
	    /* allocate a new buddy. */
	    *bp = new_allocated_buddy(root, i, addr);
	}
	b = *bp;
    }
    /* b points to the newly freed buddy */
    list[i]=b;
    b->max_free = size;
    return i;
}

extern inline void join_freed_buddies(l4buddy_root *root,
				      l4buddy_t *list[], int i){
    l4buddy_t *b;
    unsigned max_free = 0;

    for(i--; i>=0; i--){
	b = list[i];
	/* if both left and right are completetly free, join them */
	if(b->left  && b->left->max_free  == (1<<(root->size_ld-i-1)) &&
	   b->right && b->right->max_free == (1<<(root->size_ld-i-1))){
	    b->max_free = max_free = 1<<(root->size_ld-i);
	    b->left = 0;
	    b->right = 0;
	} else if(max_free > b->max_free){
	    b->max_free = max_free;
	}
    }
}

void l4buddy_free(l4buddy_root *root, void*addr){
    unsigned off = (unsigned char*)addr-root->base_addr;
    unsigned size, size_ld, index;
    l4buddy_t **list;

    LOGd_Enter(LOG_BUDDY_CALL_FREE, "addr=%p", addr);
    if(!addr) return;
    size_ld = get_buddy_size_ld(root, off);
    size = 1<<size_ld;
#if DEBUG_BUDDY
    if(size_ld == (root->big_pool?255:15)){
	LOG_Error("%p: Trying to free already freed portion of memory", addr);
	return;
    }
    {
	int i;
	for(i=0; i<size; i+=8){
	    memcpy((unsigned char*)addr+i, "emptybud", 8);
	}
    }
#endif

    LOGd(LOG_BUDDY_FREE, "Freeing %d bytes at %p", size, addr);
    list = alloca(sizeof(l4buddy_t*) * (root->size_ld-size_ld+1));
    index = split_and_free_buddy(addr, off, size, size_ld, root, list);
    join_freed_buddies(root, list, index);
#if DEBUG_BUDDY
    root->allocated -= size;
    root->free+=size;
    set_buddy_size_ld(root, off, root->big_pool?255:15);
#endif
}

#if DEBUG_BUDDY
static void l4buddy_debug_dump_buddy(l4buddy_root *root, l4buddy_t *buddy,
				     int index, int off);
static void l4buddy_debug_dump_buddy(l4buddy_root *root,
				     l4buddy_t *buddy, int index, int off){
    if(buddy){
	fprintf(stderr,
		"  %*soff %d, index=%d, max_free=%d, left=%d, right=%d\n",
		index*2, "", off, index, buddy->max_free,
		buddy->left? ( (unsigned)buddy->left -
			       (unsigned)root->base_addr) : -1,
		buddy->right?( (unsigned)buddy->right -
			       (unsigned)root->base_addr) : -1);
	l4buddy_debug_dump_buddy(root, buddy->left, index+1, off);
	l4buddy_debug_dump_buddy(root, buddy->right, index+1,
				 off+ (1<<(root->size_ld-index-1)));
    } else {
	fprintf(stderr, "  %*s(null)\n", index*2, "");
    }
}

void l4buddy_debug_dump(l4buddy_root*root){
    fprintf(stderr,
	    "Buddy-tree: size=%d, %d elems, log2(size)=%d, %s size-map.\n",
	    root->size, root->size>>L4BUDDY_BUDDY_SHIFT, root->size_ld,
	    root->big_pool?"big":"small");
    fprintf(stderr,
	    "  base_addr %p, size_map %p (off %d)\n",
	    root->base_addr, root->size_map, root->size_map-root->base_addr);
    l4buddy_debug_dump_buddy(root, &root->rootbuddy, 0, 0);
}
int l4buddy_debug_free(l4buddy_root*root){
    return root->free;
}
int l4buddy_debug_allocated(l4buddy_root*root){
    return root->allocated;
}
#endif
