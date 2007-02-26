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

  enum { 
    SIZE = 8  ///< The maximum number of consoles to be multiplexed.
  };

  Mux_console();

  int write( char const *str, size_t len );
  int getchar( bool blocking = true );
  int char_avail() const;

  /**
   * @brief Register a console to be multiplexed.
   * @param cons the Console to add.
   * @param pos the position of the console, normally not needed.
   */
  bool register_console( Console *cons, int pos = 0 );

  /**
   * @brief Unregister a console from the multiplexer.
   * @param cons the console to remove.
   */
  bool unregister_console( Console *cons );

private:

  int _items;
  Console *_cons[SIZE];

};


IMPLEMENTATION:


#include "processor.h"

IMPLEMENT 
Mux_console::Mux_console()
  : _items(0)
{}

IMPLEMENT
int Mux_console::write( char const *str, size_t len )
{
  for(int i=0; i<_items; ++i) 
    if(_cons[i])
      _cons[i]->write(str,len);
  return len;
}



IMPLEMENT
int Mux_console::getchar( bool blocking )
{
  int ret = -1;
  do {
    for(int i=0; i<_items; ++i) {
      if(_cons[i]) {
	ret = _cons[i]->getchar( false );
	if(ret!=-1) return ret;
      }
    }
    if(blocking) Proc::pause();
  } while( blocking && ret==-1 );

  return ret;
}

IMPLEMENT
int Mux_console::char_avail() const
{
  int ret = -1;
  int tmp;
  for(int i=0; i<_items; ++i) 
    {
      if(_cons[i]) 
	{
	  tmp = _cons[i]->char_avail();
	  if(tmp==1) 
	    return 1;
	  else if(tmp==0)
	    ret = tmp;
	}
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
  _cons[pos] = c;  

  return true;
}

IMPLEMENT
bool Mux_console::unregister_console( Console *c )
{
  int pos;
  for(pos = 0;pos < _items && _cons[pos]!=c;++pos);
  if(pos==_items) 
    return false;
  

  _items--;
  for(int i = pos; i<_items; ++i) 
    _cons[i] = _cons[i+1];


  return true;

}
