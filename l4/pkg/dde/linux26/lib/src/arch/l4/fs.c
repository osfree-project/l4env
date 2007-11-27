#include "local.h"

#include <linux/fs.h>
#include <linux/backing-dev.h>

struct backing_dev_info default_backing_dev_info = { };

int seq_printf(struct seq_file *m, const char *f, ...)
{
	WARN_UNIMPL;
	return 0;
}

int generic_writepages(struct address_space *mapping,
                       struct writeback_control *wbc)
{
	WARN_UNIMPL;
	return 0;
}

/**************************************
 * Filemap stuff                      *
 **************************************/
struct page * find_get_page(struct address_space *mapping, unsigned long offset)
{
	WARN_UNIMPL;
	return NULL;
}

void fastcall unlock_page(struct page *page)
{
	WARN_UNIMPL;
}

int test_set_page_writeback(struct page *page)
{
	WARN_UNIMPL;
	return 0;
}

void end_page_writeback(struct page *page)
{
	WARN_UNIMPL;
}

void do_invalidatepage(struct page *page, unsigned long offset)
{
	WARN_UNIMPL;
}

int redirty_page_for_writepage(struct writeback_control *wbc, struct page *page)
{
	WARN_UNIMPL;
	return 0;
}
