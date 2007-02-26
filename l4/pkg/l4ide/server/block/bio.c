/*
 * Copyright (C) 2001 Jens Axboe <axboe@suse.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public Licens
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-
 *
 */
#include <linux/mm.h>
#include <linux/swap.h>
#include <linux/bio.h>
#include <linux/blkdev.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mempool.h>
#include <linux/workqueue.h>
#include <l4/util/macros.h> // for LOG_Error
#include <l4/l4rm/l4rm.h>

#define get_user_pages(x...) 0
#define BIO_POOL_SIZE 256

static mempool_t *bio_pool;
static kmem_cache_t *bio_slab;

/*
 * Attention! Due to the limitation of the L4 slab size to 4096 bytes (one page)
 * we can't support biovec pools larger than 64 entries.
 */
#define BIOVEC_NR_POOLS 4

struct biovec_pool {
	int nr_vecs;
	char *name; 
	kmem_cache_t *slab;
	mempool_t *pool;
};

/*
 * if you change this list, also change bvec_alloc or things will
 * break badly! cannot be bigger than what you can fit into an
 * unsigned short
 */

#define BV(x) { .nr_vecs = x, .name = "biovec-" #x }
static struct biovec_pool bvec_array[BIOVEC_NR_POOLS] = {
//	BV(1), BV(4), BV(16), BV(64), BV(128), BV(BIO_MAX_PAGES), slab limit
	BV(1), BV(4), BV(16), BV(64),
};
#undef BV

static inline struct bio_vec *bvec_alloc(int gfp_mask, int nr, unsigned long *idx)
{
	struct biovec_pool *bp;
	struct bio_vec *bvl;

	/*
	 * see comment near bvec_array define!
	 */
	switch (nr) {
		case   1        : *idx = 0; break;
		case   2 ...   4: *idx = 1; break;
		case   5 ...  16: *idx = 2; break;
		case  17 ...  64: *idx = 3; break;
		case  65 ... BIO_MAX_PAGES: LOG_Error("BIOs with more than 64 BIO_VECs are not supported (%i)",nr);
//		case  65 ... 128: *idx = 4; break; slab limit
//		case 129 ... BIO_MAX_PAGES: *idx = 5; break; slab limit
		default:
			return NULL;
	}
	/*
	 * idx now points to the pool we want to allocate from
	 */
	bp = bvec_array + *idx;

	bvl = mempool_alloc(bp->pool, gfp_mask);
	if (bvl)
		memset(bvl, 0, bp->nr_vecs * sizeof(struct bio_vec));
	return bvl;
}

/*
 * default destructor for a bio allocated with bio_alloc()
 */
void bio_destructor(struct bio *bio)
{
	const int pool_idx = BIO_POOL_IDX(bio);
	struct biovec_pool *bp = bvec_array + pool_idx;

	BIO_BUG_ON(pool_idx >= BIOVEC_NR_POOLS);

	/*
	 * cloned bio doesn't own the veclist
	 */
	if (!bio_flagged(bio, BIO_CLONED))
		mempool_free(bio->bi_io_vec, bp->pool);

	mempool_free(bio, bio_pool);
}

inline void bio_init(struct bio *bio)
{
	bio->bi_next = NULL;
	bio->bi_flags = 1 << BIO_UPTODATE;
	bio->bi_rw = 0;
	bio->bi_vcnt = 0;
	bio->bi_idx = 0;
	bio->bi_phys_segments = 0;
	bio->bi_hw_segments = 0;
	bio->bi_size = 0;
	bio->bi_max_vecs = 0;
	bio->bi_end_io = NULL;
	atomic_set(&bio->bi_cnt, 1);
	bio->bi_private = NULL;
}

/**
 * bio_alloc - allocate a bio for I/O
 * @gfp_mask:   the GFP_ mask given to the slab allocator
 * @nr_iovecs:	number of iovecs to pre-allocate
 *
 * Description:
 *   bio_alloc will first try it's on mempool to satisfy the allocation.
 *   If %__GFP_WAIT is set then we will block on the internal pool waiting
 *   for a &struct bio to become free.
 **/
struct bio *bio_alloc(int gfp_mask, int nr_iovecs)
{
	struct bio_vec *bvl = NULL;
	unsigned long idx;
	struct bio *bio;

	bio = mempool_alloc(bio_pool, gfp_mask);
	if (unlikely(!bio))
		goto out;

	bio_init(bio);

	if (unlikely(!nr_iovecs))
		goto noiovec;

	bvl = bvec_alloc(gfp_mask, nr_iovecs, &idx);
	if (bvl) {
		bio->bi_flags |= idx << BIO_POOL_OFFSET;
		bio->bi_max_vecs = bvec_array[idx].nr_vecs;
		for (idx = 0; idx < nr_iovecs; idx++) {
		    bvl[idx].bv_ds = L4DM_INVALID_DATASPACE;
		}
noiovec:
		bio->bi_io_vec = bvl;
		bio->bi_destructor = bio_destructor;
out:
		return bio;
	}

	mempool_free(bio, bio_pool);
	bio = NULL;
	goto out;
}

/**
 * bio_put - release a reference to a bio
 * @bio:   bio to release reference to
 *
 * Description:
 *   Put a reference to a &struct bio, either one you have gotten with
 *   bio_alloc or bio_get. The last put of a bio will free it.
 **/
void bio_put(struct bio *bio)
{
	BIO_BUG_ON(!atomic_read(&bio->bi_cnt));

	/*
	 * last put frees it
	 */
	if (atomic_dec_and_test(&bio->bi_cnt)) {
		bio->bi_next = NULL;
		bio->bi_destructor(bio);
	}
}

inline int bio_phys_segments(request_queue_t *q, struct bio *bio)
{
	if (unlikely(!bio_flagged(bio, BIO_SEG_VALID)))
		blk_recount_segments(q, bio);

	return bio->bi_phys_segments;
}

inline int bio_hw_segments(request_queue_t *q, struct bio *bio)
{
	if (unlikely(!bio_flagged(bio, BIO_SEG_VALID)))
		blk_recount_segments(q, bio);

	return bio->bi_hw_segments;
}

/**
 * 	__bio_clone	-	clone a bio
 * 	@bio: destination bio
 * 	@bio_src: bio to clone
 *
 *	Clone a &bio. Caller will own the returned bio, but not
 *	the actual data it points to. Reference count of returned
 * 	bio will be one.
 */
inline void __bio_clone(struct bio *bio, struct bio *bio_src)
{
	bio->bi_io_vec = bio_src->bi_io_vec;

	bio->bi_sector = bio_src->bi_sector;
	bio->bi_bdev = bio_src->bi_bdev;
	bio->bi_flags |= 1 << BIO_CLONED;
	bio->bi_rw = bio_src->bi_rw;

	/*
	 * notes -- maybe just leave bi_idx alone. assume identical mapping
	 * for the clone
	 */
	bio->bi_vcnt = bio_src->bi_vcnt;
	bio->bi_idx = bio_src->bi_idx;
	if (bio_flagged(bio, BIO_SEG_VALID)) {
		bio->bi_phys_segments = bio_src->bi_phys_segments;
		bio->bi_hw_segments = bio_src->bi_hw_segments;
		bio->bi_flags |= (1 << BIO_SEG_VALID);
	}
	bio->bi_size = bio_src->bi_size;

	/*
	 * cloned bio does not own the bio_vec, so users cannot fiddle with
	 * it. clear bi_max_vecs and clear the BIO_POOL_BITS to make this
	 * apparent
	 */
	bio->bi_max_vecs = 0;
	bio->bi_flags &= (BIO_POOL_MASK - 1);
}

/**
 *	bio_clone	-	clone a bio
 *	@bio: bio to clone
 *	@gfp_mask: allocation priority
 *
 * 	Like __bio_clone, only also allocates the returned bio
 */
struct bio *bio_clone(struct bio *bio, int gfp_mask)
{
	struct bio *b = bio_alloc(gfp_mask, 0);

	if (b)
		__bio_clone(b, bio);

	return b;
}

/**
 *	bio_get_nr_vecs		- return approx number of vecs
 *	@bdev:  I/O target
 *
 *	Return the approximate number of pages we can send to this target.
 *	There's no guarantee that you will be able to fit this number of pages
 *	into a bio, it does not account for dynamic restrictions that vary
 *	on offset.
 */
int bio_get_nr_vecs(struct block_device *bdev)
{
	request_queue_t *q = bdev_get_queue(bdev);
	int nr_pages;

	nr_pages = ((q->max_sectors << 9) + PAGE_SIZE - 1) >> PAGE_SHIFT;
	if (nr_pages > q->max_phys_segments)
		nr_pages = q->max_phys_segments;
	if (nr_pages > q->max_hw_segments)
		nr_pages = q->max_hw_segments;

	return nr_pages;
}

/**
 * bio_endio - end I/O on a bio
 * @bio:	bio
 * @bytes_done:	number of bytes completed
 * @error:	error, if any
 *
 * Description:
 *   bio_endio() will end I/O on @bytes_done number of bytes. This may be
 *   just a partial part of the bio, or it may be the whole bio. bio_endio()
 *   is the preferred way to end I/O on a bio, it takes care of decrementing
 *   bi_size and clearing BIO_UPTODATE on error. @error is 0 on success, and
 *   and one of the established -Exxxx (-EIO, for instance) error values in
 *   case something went wrong. Noone should call bi_end_io() directly on
 *   a bio unless they own it and thus know that it has an end_io function.
 **/
void bio_endio(struct bio *bio, unsigned int bytes_done, int error)
{
	if (error)
		clear_bit(BIO_UPTODATE, &bio->bi_flags);

	if (unlikely(bytes_done > bio->bi_size)) {
		printk("%s: want %u bytes done, only %u left\n", __FUNCTION__,
						bytes_done, bio->bi_size);
		bytes_done = bio->bi_size;
	}

	bio->bi_size -= bytes_done;
	bio->bi_sector += (bytes_done >> 9);

	if (bio->bi_end_io)
		bio->bi_end_io(bio, bytes_done, error);
}

static void __init biovec_init_pools(void)
{
	int i, size, megabytes, pool_entries = BIO_POOL_SIZE;
	int scale = BIOVEC_NR_POOLS;

	megabytes = nr_free_pages() >> (20 - PAGE_SHIFT);

	/*
	 * find out where to start scaling
	 */
	if (megabytes <= 16)
		scale = 0;
	else if (megabytes <= 32)
		scale = 1;
	else if (megabytes <= 64)
		scale = 2;
	else if (megabytes <= 96)
		scale = 3;
	else if (megabytes <= 128)
		scale = 4;

	/*
	 * scale number of entries
	 */
	pool_entries = megabytes * 2;
	if (pool_entries > 256)
		pool_entries = 256;

	for (i = 0; i < BIOVEC_NR_POOLS; i++) {
		struct biovec_pool *bp = bvec_array + i;

		size = bp->nr_vecs * sizeof(struct bio_vec);

		bp->slab = kmem_cache_create(bp->name, size, 0,
						SLAB_HWCACHE_ALIGN, NULL, NULL);
		if (!bp->slab)
			panic("biovec: can't init slab cache\n");

		if (i >= scale)
			pool_entries >>= 1;

		bp->pool = mempool_create(pool_entries, mempool_alloc_slab,
					mempool_free_slab, bp->slab);
		if (!bp->pool)
			panic("biovec: can't init mempool\n");
	}
}

static int __init init_bio(void)
{
	bio_slab = kmem_cache_create("bio", sizeof(struct bio), 0,
					SLAB_HWCACHE_ALIGN, NULL, NULL);
	if (!bio_slab)
		panic("bio: can't create slab cache\n");
	bio_pool = mempool_create(BIO_POOL_SIZE, mempool_alloc_slab, mempool_free_slab, bio_slab);
	if (!bio_pool)
		panic("bio: can't create mempool\n");

	biovec_init_pools();

	return 0;
}

module_init(init_bio);
