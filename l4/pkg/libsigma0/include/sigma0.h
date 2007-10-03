#ifndef __L4_SIGMA0_SIGMA0_H
#define __L4_SIGMA0_SIGMA0_H

#include <l4/sys/compiler.h>
#include <l4/sys/types.h>

#define SIGMA0_ID (l4_threadid_t){.id = {.lthread = 0, .task = 2}}

#undef SIGMA0_REQ_MAGIC
#undef SIGMA0_REQ_MASK

# define SIGMA0_REQ_MAGIC		~0xFFUL
# define SIGMA0_REQ_MASK		~0xFFUL

/* Starting with 0x60 allows to detect components which still use the old
 * constants (0x00 ... 0x50) */
#define SIGMA0_REQ_ID_MASK		  0xF0
#define SIGMA0_REQ_ID_FPAGE_RAM		  0x60
#define SIGMA0_REQ_ID_FPAGE_IOMEM	  0x70
#define SIGMA0_REQ_ID_FPAGE_IOMEM_CACHED  0x80
#define SIGMA0_REQ_ID_FPAGE_ANY		  0x90
#define SIGMA0_REQ_ID_KIP		  0xA0
#define SIGMA0_REQ_ID_TBUF		  0xB0
#define SIGMA0_REQ_ID_DEBUG_DUMP	  0xC0

#define SIGMA0_IS_MAGIC_REQ(d1)	\
  ((d1 & SIGMA0_REQ_MASK) == SIGMA0_REQ_MAGIC)

#define SIGMA0_REQ(x) \
  (SIGMA0_REQ_MAGIC + SIGMA0_REQ_ID_ ## x)

/* Use these constants in your code! */
#define SIGMA0_REQ_FPAGE_RAM		  (SIGMA0_REQ(FPAGE_RAM))
#define SIGMA0_REQ_FPAGE_IOMEM		  (SIGMA0_REQ(FPAGE_IOMEM))
#define SIGMA0_REQ_FPAGE_IOMEM_CACHED	  (SIGMA0_REQ(FPAGE_IOMEM_CACHED))
#define SIGMA0_REQ_FPAGE_ANY		  (SIGMA0_REQ(FPAGE_ANY))
#define SIGMA0_REQ_KIP			  (SIGMA0_REQ(KIP))
#define SIGMA0_REQ_TBUF			  (SIGMA0_REQ(TBUF))
#define SIGMA0_REQ_DEBUG_DUMP	  	  (SIGMA0_REQ(DEBUG_DUMP))

EXTERN_C_BEGIN

/** \defgroup sigma0 Sigma0 protocol bindings */

/** 
 * @brief Request a memory mapping from sigma0.
 * @ingroup sigma0
 * @param pager is usually the sigma0 thread id (2.0), however also roottask 
 *              talks sigma0 protocol.
 * @param phys  the physical address of the requested page (must be at least
 *              aligned to the minimum page size).
 * @param virt  the virtual address where the paged should be mapped in the 
 *              local address space (must be at least aligned to the minimum
 *              page size).
 * @param size  the size of the requested page, this must be a multiple of
 *              the minimum page size.
 *
 * @return the error code of the operation, 0 means OK. (See 
 *         l4sigma0_map_errstr() for details on the other return codes.)
 */
int l4sigma0_map_mem(l4_threadid_t pager,
		     l4_addr_t phys, l4_addr_t virt, l4_addr_t size);

/**
 * @brief Request IO memory from sigma0. 
 *
 * This function is similar to l4sigma0_map_mem(), the difference is that
 * it requests IO memory. IO memory is everything that is not known
 * to be normal RAM. Also ACPI tables or the BIOS memory is treated as IO
 * memory.
 * @ingroup sigma0
 *
 * @param pager usually the thread id of sigma0.
 * @param phys  the physical address to be requested (page aligned).
 * @param virt  the virtual address where the memory should be mapped to
 *              (page aligned).
 * @param size  the size of the IO memory area to be mapped (multiple of
 *              page size)
 * @param cached requests cacheable IO memory if 1, and uncached if 0.
 *
 * @return the error code, where 0 is OK (see l4sigma0_map_errstr()).
 */
int l4sigma0_map_iomem(l4_threadid_t pager, l4_addr_t phys,
		       l4_addr_t virt, l4_addr_t size, int cached);
/**
 * @brief Request an arbitrary free page of RAM.
 * @ingroup sigma0
 *
 * This function requests arbitrary free memory from sigma0. It should be used
 * whenwever spare memory is needed, instead of requesting specific physical
 * memory with l4sigma0_map_mem().
 *
 * @param pager    unsually the thread id of sigma0.
 * @param map_area the base address of the local virtual memory area where the
 *                 page should be mapped.
 * @param log2_map_size the sizf of the requested page log 2 (the size in 
 *                      bytes is 2^log2_map_size. This must be at least the 
 *                      minimal page size. By specifing larger sizes the 
 *                      largest possible hardware page size will be used.
 * @retval base    physical address of the page received (i.e., the send base
 *                 of the received mapping if any).
 * 
 * @return 0 if OK, error code else (see l4sigma0_map_errstr()).
 */
int l4sigma0_map_anypage(l4_threadid_t pager, l4_addr_t map_area,
			 unsigned log2_map_size, l4_addr_t *base);

/**
 * @brief Request Fiasco trace buffer.
 * @ingroup sigma0
 *
 * This is a Fiasco specific feature. Where you can request the kernel 
 * internal trace buffer for user-level evaluation. This is for special
 * debugging tools, such as Ferret.
 *
 * @param pager as usual the sigma0 thread id.
 * @param virt the virtual address where the trace buffer should be mapped,
 *
 * @return 0 on success, !0 else (see l4sigma0_map_errstr()).
 */
int l4sigma0_map_tbuf(l4_threadid_t pager, l4_addr_t virt);

/**
 * @brief Request sigma0 to dump internal debug information.
 * @ingroup sigma0
 *
 * The debug information, such as internal memory maps, as well as
 * statistics about the internal allocators is dumped to the kernel debugger.
 *
 * @param pager the sigmao thread id.
 */
void l4sigma0_debug_dump(l4_threadid_t pager);

/**
 * @brief Get a user readable error messages for the return codes.
 * @ingroup sigma0
 *
 * @param err the error code rported by the *map* functions.
 * @return a string containing the error message.
 */
static inline const char *l4sigma0_map_errstr(int err)
{
  switch (err)
    {
    case  0: return "No error";
    case -1: return "Phys, virt or size not aligned";
    case -2: return "IPC error";
    case -3: return "No fpage received";
#ifndef SIGMA0_REQ_MAGIC
    case -4: return "Bad physical address (old protocol only)";
#endif
    case -6: return "Superpage requested but smaller flexpage received";
    case -7: return "Cannot map I/O memory cacheable (old protocol only)";
    default: return "Unknown error";
    }
}

EXTERN_C_END

#endif /* ! __L4_SIGMA0_SIGMA0_H */
