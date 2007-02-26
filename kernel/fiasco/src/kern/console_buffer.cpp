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
  char *out_buf;
  char *out_buf_w;
  char *out_buf_r;
  

};


IMPLEMENTATION:

#include <cstdio>
#include <cstring>

#include "config.h"
#include "kmem_alloc.h"

IMPLEMENT
Console_buffer::Console_buffer()
  : _enabled(false),
    out_buf_size(0), out_buf(0), out_buf_w(0), out_buf_r(0)
{
}

IMPLEMENT
void Console_buffer::alloc( size_t size)
{
  if(!out_buf) {
    out_buf_size = (size + Config::PAGE_SIZE - 1) >> Config::PAGE_SHIFT;
    if(out_buf_size)
      out_buf = static_cast<char*>(
        Kmem_alloc::allocator()->unaligned_alloc(out_buf_size)
      );

    out_buf_w = out_buf;
    out_buf_r = out_buf;
  }
}

IMPLEMENT
Console_buffer::~Console_buffer()
{
  if(out_buf)
    Kmem_alloc::allocator()->unaligned_free(out_buf_size, out_buf); 
}

IMPLEMENT 
void Console_buffer::enable()
{
  _enabled = true;
}

IMPLEMENT
void Console_buffer::disable()
{
  _enabled = false;
}

IMPLEMENT
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
	  s = (out_buf_size<<Config::PAGE_SHIFT) - (out_buf_w - out_buf); 
	  if(s>len)
	    s = len;
	
	  memcpy( out_buf_w, str, s );
	  
	  if( out_buf_w + s >= out_buf + (out_buf_size<<Config::PAGE_SHIFT) )
	    out_buf_w = out_buf;
	  else
	    out_buf_w += s;
	  
	  len -= s;
	}
    }
  return len;
}

PRIVATE inline
void Console_buffer::inc_out_ptr(char **c)
{
  if (++*c >= out_buf + (out_buf_size<<Config::PAGE_SHIFT))
    *c = out_buf;
}

PRIVATE inline
void Console_buffer::dec_out_ptr(char **c)
{
  if (--*c < out_buf)
    *c = out_buf + (out_buf_size<<Config::PAGE_SHIFT) - 1;
}

IMPLEMENT
int Console_buffer::getchar( bool )
{
  return -1;
}

IMPLEMENT
int Console_buffer::print_buffer(size_t lines)
{
  State_guard<Console_buffer> guard(this);

  char *c = out_buf_r;
  bool page = (lines == 0);

  if (out_buf)
    {
      if (out_buf_r == out_buf_w)
	{
	  puts("<empty>\n");
	  return 1;
	}

      // go back <lines> lines
      if (lines)
	{
	  c = out_buf_w;
	 
	  // skip terminating 0x00, 0x0a ...
	  while ((*c == '\0' || *c == '\r') && (c != out_buf_r))
	    dec_out_ptr(&c);
	  
	  while (lines-- && (c != out_buf_r))
	    {
	      dec_out_ptr(&c);
	      
	      while ((*c != '\n') && (c != out_buf_r))
		dec_out_ptr(&c);
	    }

	  if (*c == '\n')
	    inc_out_ptr(&c);
	}
    
      lines = 0;
      while (c != out_buf_w)
	{
	  putchar(*c);
	  if( *c == '\n') 
	    {
	      if (page && (++lines > 20))
		{
		  printf("--- CR: line, SPACE: page, ESC: abort ---");
		  int c =::getchar();
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
	}
      
      putchar('\n');

      return 1;
    }
  
  printf("use -out_buf=<size> to enable output buffer\n");

  return 0;
}
