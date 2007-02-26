INTERFACE:

#include "types.h"
#include "console.h"

/**
 * @brief Console multiplexer.
 *
 * This implementation of the Console interface can be used to
 * multiplex among some input, output, and in-out consoles.
 */
class Mux_console 
  : public Console
{
public:

  enum Console_state {
    DISABLED   = 0,
    INENABLED  = 1,
    OUTENABLED = 2,
  };


  enum { 
    SIZE = 8  ///< The maximum number of consoles to be multiplexed.
  };

  Mux_console();

  int  write( char const *str, size_t len );
  int  getchar( bool blocking = true );
  void getchar_chance( void );
  int  char_avail() const;

  void change_state( Console *c, Mword mask, Mword bits );

  void change_state( bool has, char const *attribute, Mword mask, Mword bits );

  /**
   * @brief Register a console to be multiplexed.
   * @param cons the Console to add.
   * @param pos the position of the console, normally not needed. It is
   *        1 by default. The Uart console uses 0 to ensure that the
   *        gzip output goes to the right console.
   */
  bool register_console( Console *cons, int pos = 1 );

  /**
   * @brief Unregister a console from the multiplexer.
   * @param cons the console to remove.
   */
  bool unregister_console( Console *cons );

  /**
   * @brief Register a special console which overrides output of
   *        other consoles if active.
   */
  bool register_gzip_console( Console *cons );

  /**
   * @brief Tell if special gzip console is registered.
   */
  bool gzip_available ();

  /**
   * @brief Tell if special gzip mode is enabled.
   */
  bool gzip_enabled();

  /**
   * @brief Start overriding other console's output.
   */
  void gzip_enable ();

  /**
   * @brief Stop overriding other console's output.
   */
  void gzip_disable ();

  /**
   * @brief function is called by gzip module to write the uuencode data
   */
  void gzip_write( char const *str, size_t len );

  char const *next_attribute( bool restart = false ) const;

private:

  
  struct Centry 
  {
    Console *console;
    Mword    state;
  };

  int     _next_getchar;
  int     _items;
  int     _gzip_active;
  Centry _cons[SIZE];
  Console *_gzip;
};


IMPLEMENTATION:

#include <cstdio>
#include "processor.h"

IMPLEMENT
char const *Mux_console::next_attribute( bool restart ) const
{
  static int pos = 0;
  bool rest = false;
  if(restart)
    {
      pos = 0;
      rest = true;
    }

  while(pos<_items)
    {
      if(_cons[pos].console)
        {
          char const *a = _cons[pos].console->next_attribute(rest);
          if(a)
            return a;
          else
            {
              pos++;
              rest = true;
              continue;
            }
        }
      else
        pos++;
    }
  return 0;
}

IMPLEMENT 
Mux_console::Mux_console()
  : _next_getchar(-1), _items(0)
{}

IMPLEMENT
int Mux_console::write( char const *str, size_t len )
{
  if (_gzip_active)
    return _gzip->write(str, len);
      
  for(int i=0; i<_items; ++i) 
    if(_cons[i].console && (_cons[i].state & INENABLED))
      _cons[i].console->write(str,len);

  return len;
}

IMPLEMENT
int Mux_console::getchar( bool blocking )
{
  if (_next_getchar != -1)
    {
      int c = _next_getchar;
      _next_getchar = -1;
      return c;
    }

  int ret = -1;
  do {
    for(int i=0; i<_items; ++i)
      {
	if(_cons[i].console && (_cons[i].state & OUTENABLED))
	  {
	    ret = _cons[i].console->getchar( false );
	    if (ret != -1)
	      return ret;
	  }
      }
    if(blocking)
      Proc::pause();
  } while( blocking && ret==-1 );

  return ret;
}

IMPLEMENT
void Mux_console::getchar_chance ()
{
  if (_gzip_active)
    return;

  for (int i=0; i<_items; ++i)
    {
      if (_cons[i].console->char_avail() == 1)
	{
	  int c = _cons[i].console->getchar(false);
	  if (c != -1 && _next_getchar == -1)
      	    _next_getchar = c;
	}
    }
}

IMPLEMENT
int Mux_console::char_avail() const
{
  int ret = -1;
  for(int i=0; i<_items; ++i) 
    if(_cons[i].console && (_cons[i].state & INENABLED)) 
      {
	int tmp = _cons[i].console->char_avail();
	if(tmp==1) 
	  return 1;
	else if(tmp==0)
	  ret = tmp;
      }
  return ret;
}

IMPLEMENT
bool Mux_console::register_console( Console *c, int pos )
{
  if(_items >= SIZE) 
    return false;

  if(pos>=SIZE || pos<0) 
    return false;

  if(pos>_items)
    pos = _items;

  if(pos<_items) {
    for(int i = _items-1; i>=pos; --i) 
      _cons[i+1] = _cons[i];
  } 
  _items++;
  _cons[pos].console = c;
  _cons[pos].state   = INENABLED | OUTENABLED;

  return true;
}

IMPLEMENT
bool Mux_console::unregister_console( Console *c )
{
  int pos;
  for(pos = 0;pos < _items && _cons[pos].console!=c;++pos)
    ;
  if(pos==_items) 
    return false;
  
  _items--;
  for(int i = pos; i<_items; ++i) 
    _cons[i] = _cons[i+1];

  return true;
}

IMPLEMENT 
void Mux_console::change_state( Console *c, Mword mask, Mword bits )
{
  int pos;
  for(pos = 0; pos<_items; pos++)
    {
      if (_cons[pos].console == c)
	{
	  _cons[pos].state = (_cons[pos].state & mask) | bits;
	  return;
	}
    }
}

IMPLEMENT 
void Mux_console::change_state( bool has, char const *expr, 
				Mword mask, Mword bits )
{
  int pos;
  for(pos = 0; pos < _items; pos++)
    {
      if(_cons[pos].console)
        if(has == _cons[pos].console->check_attributes(expr))
          _cons[pos].state = (_cons[pos].state & mask) | bits;
    }
}

IMPLEMENT inline
bool Mux_console::register_gzip_console( Console *c )
{
  _gzip = c;
  return true;
}

IMPLEMENT inline
bool Mux_console::gzip_available()
{
  return _gzip != 0;
}

IMPLEMENT inline
bool Mux_console::gzip_enabled()
{
  return _gzip_active != 0;
}

IMPLEMENT
void Mux_console::gzip_enable()
{
  if (gzip_available())
    {
      _gzip_active = 1;
      _gzip->write("START_GZIP", 10);
    }
}

IMPLEMENT
void Mux_console::gzip_disable()
{
  if (gzip_available())
    {
      _gzip->write("STOP_GZIP", 9);
      _gzip_active = 0;
    }
}

IMPLEMENT
void Mux_console::gzip_write( char const *str, size_t len )
{
  if(_cons[0].console)
    _cons[0].console->write(str,len);
}

