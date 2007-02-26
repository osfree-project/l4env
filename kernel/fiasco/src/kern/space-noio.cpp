/* Dummy functions if no I/O port protection is used */

IMPLEMENTATION[noio]:

PUBLIC inline
unsigned
Space::get_io_counter (void)
{
  return 0;
}

PUBLIC inline
bool
Space::io_lookup (Address)
{
  return false;
}
