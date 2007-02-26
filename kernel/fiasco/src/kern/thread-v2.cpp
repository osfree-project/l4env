IMPLEMENTATION[v2]:

/**
 * @brief Return nesting level.
 * @return nesting level of thread's clan
 */
PUBLIC inline 
unsigned
Thread::nest() const
{
  return id().nest();
}
