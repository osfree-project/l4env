INTERFACE:

#include "paging.h"
#include "kern_types.h"
#include "mapped_alloc.h"

class Page_table //: public Page_table_defs
{
public:

  enum Status {
    E_OK = 0,
    E_NOMEM,
    E_EXISTS,
    E_UPGRADE,
    E_INVALID,
  };


  void * operator new( size_t );
  void operator delete( void * );

  static void init();

  Page_table();
  
  Status insert( P_ptr<void>, void*, size_t, 
		 Page::Attribs s = Page::USER_NO | Page::CACHEABLE);

  Status insert_invalid( void *, size_t, Mword );
  Mword lookup_invalid( void *va ) const;

  Status replace( P_ptr<void>, void*, size_t, 
		  Page::Attribs s = Page::USER_NO | Page::CACHEABLE);

  Status change( void*, Page::Attribs );

  Status remove( void* );

  P_ptr<void> lookup( void *, size_t *, Page::Attribs *a ) const;

  void copy_in( void *my_base, Page_table *o, 
		void *base, size_t size = 0 );

  static Page_table *current();

  static size_t const num_page_sizes();
  static size_t const * const page_sizes();
  static size_t const * const page_shifts();

  /**
   * Activate a page table.
   *
   * This means that the P_ptr to the page table is activated and
   * a virtual pointer to is returned. The returned virtual pointer 
   * may be uses til the the deactivate clause.
   */
  static Page_table *activate( P_ptr<Page_table> page_table );

  static void set_allocator( Mapped_allocator * );
  static Mapped_allocator *alloc();

private:
  static Mapped_allocator *allocator;

};

IMPLEMENTATION:

Mapped_allocator *Page_table::allocator = 0;


IMPLEMENT 
void Page_table::set_allocator( Mapped_allocator *a )
{
  allocator = a;
}


IMPLEMENT 
Mapped_allocator *Page_table::alloc()
{
  return allocator;
}

