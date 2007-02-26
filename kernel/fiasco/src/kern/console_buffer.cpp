INTERFACE:

#include "console.h"
#include "types.h"

/**
 * @brief A output console that stores the output in a buffer.
 *
 * This buffer can be usefull for accessing older the debugging 
 * output without a serial console.
 */
class Console_buffer : public Console
{
public:
  
  /// ctor.
  Console_buffer();

  /// dtor.
  ~Console_buffer();

  /**
   * @brief Allocates a buffer of the given size.
   * @param size the buffer size in bytes.
   */
  void alloc( size_t size );

  // Write to the buffer
  int write( char const *str, size_t len );

  // empty (output only)
  int getchar( bool blocking = true );

  /**
   * @brief Prints the buffer to the standard I/O.
   * @param lines the number of lines to skip.
   * This method prints the buffer contents to the normal I/O.
   * Before doing this the buffer is disabled, that no recursion 
   * appers even if the buffer is part of the muxed I/O.
   */
  int print_buffer( size_t lines );
  
  /**
   * @brief Enables the buffer (recoring on).
   */
  void enable();

  /**
   * @brief Disables the buffer (recording off).
   */
  void disable();

  /**
   * @brief Get the current buffer state (on or off).
   * @return true, if the buffer is enabled, false else.
   */
  bool state();

private:
  
  bool _enabled;
  size_t out_buf_size;
  size_t out_buf_len;
  char *out_buf;
  char *out_buf_w;
};


IMPLEMENTATION:

#include <cstdio>
#include <cstring>

#include "config.h"
#include "kernel_console.h"
#include "kmem_alloc.h"

IMPLEMENT
Console_buffer::Console_buffer()
  : _enabled(false),
    out_buf_size(0), out_buf_len(0), out_buf(0), out_buf_w(0)
{
}

IMPLEMENT
void Console_buffer::alloc( size_t size)
{
  if(!out_buf)
    {
      out_buf_size = (size + Config::PAGE_SIZE - 1);
      if(out_buf_size)
	out_buf = (char*)Kmem_alloc::allocator()->
	  unaligned_alloc(out_buf_size >> Config::PAGE_SHIFT);

      out_buf_w = out_buf;
    }
}

IMPLEMENT
Console_buffer::~Console_buffer()
{
  if(out_buf)
    Kmem_alloc::allocator()->
      unaligned_free(out_buf_size >> Config::PAGE_SHIFT, out_buf);
}

IMPLEMENT inline
void Console_buffer::enable()
{
  _enabled = true;
}

IMPLEMENT inline
void Console_buffer::disable()
{
  _enabled = false;
}

IMPLEMENT inline
bool Console_buffer::state()
{
  return _enabled;
}

template< typename T >
class State_guard 
{
private:
  bool const o_state;
  T *o;
public:
  State_guard(T *_o) : o_state(_o->state()), o(_o) { o->disable(); }
  ~State_guard() { if(o_state) o->enable(); }
};

IMPLEMENT
int Console_buffer::write( char const *str, size_t len )
{
  
  if(_enabled && out_buf) 
    {
      while(len)
	{
	  size_t s;
	  s = out_buf_size - (out_buf_w - out_buf); 
	  if (s>len)
	    s = len;
	  memcpy( out_buf_w, str, s );
	  if (out_buf_w + s >= out_buf + out_buf_size)
	    out_buf_w = out_buf;
	  else
	    out_buf_w += s;
	  len -= s;
	  out_buf_len += s;
	  if (out_buf_len > out_buf_size)
	    out_buf_len = out_buf_size;
	}
    }
  return len;
}

PRIVATE inline
void Console_buffer::inc_out_ptr(char **c)
{
  if (++*c >= out_buf + out_buf_size)
    *c = out_buf;
}

PRIVATE inline
void Console_buffer::dec_out_ptr(char **c)
{
  if (--*c < out_buf)
    *c += out_buf_size;
}

IMPLEMENT
int Console_buffer::getchar(bool)
{
  return -1;
}

IMPLEMENT
int Console_buffer::print_buffer(size_t lines)
{
  if (out_buf)
    {
      State_guard<Console_buffer> guard(this);
      bool page  = (lines == 0);
      char   *c  = out_buf_w;
      size_t len = out_buf_len;

      if (out_buf_len == 0)
	{
	  puts("<empty>\n");
	  return 1;
	}

      // go back <lines> lines
      if (lines)
	{
	  size_t l = out_buf_len;

	  // skip terminating 0x00, 0x0a ...
	  while ((*c == '\0' || *c == '\r') && (l > 0))
	    {
	      dec_out_ptr(&c);
	      l--;
	    }

	  while (lines-- && (l > 0))
	    {
	      dec_out_ptr(&c);
	      l--;

	      while ((*c != '\n') && (l > 0))
		{
		  dec_out_ptr(&c);
		  l--;
		}
	    }

	  if (*c == '\n')
	    {
	      inc_out_ptr(&c);
	      l++;
	    }

	  len = out_buf_len - l;
	}
      else
	{
	  c -= out_buf_len;
	  if (c < out_buf)
	    c += out_buf_size;
	}

      lines = 0;
      while (len > 0)
	{
	  putchar(*c);
	  if (*c == '\n') 
	    {
	      if (page && (++lines > 20))
		{
		  printf("--- CR: line, SPACE: page, ESC: abort ---");
		  int c = Kconsole::console()->getchar();
		  printf("\r\033[K");
		  switch (c)
		    {
		    case 0x1b:
		    case 'q':
		      // esc -- abort
		      putchar('\n');
		      return 1;
		    case 0x0d:
		      // enter -- one more line
		      lines--;
		      break;
		    default:
		      // anything else -- one more page
		      lines=0;
		      break;
		    }
		}
	    }
	  inc_out_ptr(&c);
	  len--;
	}
      
      putchar('\n');
      return 1;
    }

  printf("use -out_buf=<size> to enable output buffer\n");
  return 0;
}

PUBLIC
char const *Console_buffer::next_attribute( bool restart = false ) const
{
  static char const *attribs[] = { "buffer", "out", 0 };
  static unsigned pos = 0;
  if(restart)
    pos = 0;
  if(pos < 2)
    return attribs[pos++];
  else
    return 0;
}
