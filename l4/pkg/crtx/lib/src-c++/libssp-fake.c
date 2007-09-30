/* Some distributions ship their libs with the stack protector feature
 * enabled. So when linking against those libs, we need provide the symbols.
 */

void
__stack_chk_fail_local (void);

void
__stack_chk_fail_local (void)
{
}

