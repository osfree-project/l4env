INTERFACE:

#include "paging.h"
#include "kern_types.h"

class Space;

class Vmem_alloc
{
public:

  enum Zero_fill {
    NO_ZERO_FILL = 0,
    ZERO_FILL,		///< Fill the page with zeroes.
    ZERO_MAP,		///< Allocate nothing but map the global zero page.
  };
  
  static void init();

  /**
   * Allocate a page of kernel memory and insert it into the master
   * page directory.
   *
   * @param address the virtual address where to map the page.
   * @param zf zero fill or zero map.
   * @param pa page attributes to use for the page table entry.
   */
  static void *page_alloc (void *address,
			   Zero_fill zf = NO_ZERO_FILL,
			   Page::Attribs pa = Page::USER_NO);

  /**
   * Free the page at the given virtual address.  If kernel memory was
   * mapped at this page, deallocate the kernel memory as well.
   *
   * @param page Virtual address of the page to free.
   */
  static void page_free (void *page);

  /**
   * Set the page attributes of an already existing page of kernel memory.
   *
   * @param address the virtual address where the page is mapped.
   * @param pa page attributes to use for the page table entry.
   */
  static void *page_attr (void *address, Page::Attribs pa);

private:
  static void page_map (void *address, int order, Zero_fill zf,
                        Address phys);

  static void page_unmap (void *address, int size);
};

