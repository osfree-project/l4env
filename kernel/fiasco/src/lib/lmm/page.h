
#ifndef	__PAGE_H_
#define __PAGE_H_

//#include <asm/page.h>

#define PAGE_SHIFT 12

#ifndef PAGE_SHIFT
#error asm/page.h needs to define PAGE_SHIFT.
#endif

#ifndef PAGE_SIZE
#define PAGE_SIZE (1 << PAGE_SHIFT)
#endif

#ifndef PAGE_MASK
#define PAGE_MASK (PAGE_SIZE-1)
#endif

/*
 *	Convert addresses to pages and vice versa.
 *	No rounding is used.
 */

#define atop(x)		(((vm_size_t)(x)) >> PAGE_SHIFT)
#define ptoa(x)		((vm_offset_t)((x) << PAGE_SHIFT))

/*
 *	Round off or truncate to the nearest page.  These will work
 *	for either addresses or counts.  (i.e. 1 byte rounds to 1 page
 *	bytes.
 */

#define round_page(x)	((vm_offset_t)((((vm_offset_t)(x)) + PAGE_MASK) & ~PAGE_MASK))
#define trunc_page(x)	((vm_offset_t)(((vm_offset_t)(x)) & ~PAGE_MASK))

/*
 *	Determine whether an address is page-aligned, or a count is
 *	an exact page multiple.
 */

#define	page_aligned(x)	((((vm_offset_t) (x)) & PAGE_MASK) == 0)

#endif	
