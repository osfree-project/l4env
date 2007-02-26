INTERFACE:

#include "console.h"
#include "types.h"

class Gdb_serv : public Console
{
public:

  Gdb_serv( Console *o );
  ~Gdb_serv();

  int write( char const *str, size_t len );
  int getchar( bool blocking = true );
  int char_avail() const;

  void enter();

  void attache();

  void putpacket( char const *p );

  virtual void get_register( unsigned index ) = 0;
  virtual unsigned num_registers() const = 0;
  virtual void set_register( unsigned index, char const *buffer ) = 0;

protected:
  void send_byte( char c );
  void send( char const *str );
  void send( char c ); 

  void send_registers();

private:
  bool attached;
  Console *_o;
};


IMPLEMENTATION:

IMPLEMENT
Gdb_serv::Gdb_serv( Console *o )
  : attached(false), _o(o)
{}

IMPLEMENT
Gdb_serv::~Gdb_serv()
{}

static char hexc[] = "0123456789abcdef";

static void hex_encode( char *buffer, char const *str, size_t len )
{
  for( size_t pos = 0; pos < len; pos++ )
    {
      buffer[pos*2]   = hexc[str[pos] >> 4];
      buffer[pos*2+1] = hexc[str[pos] & 0x0f];
    }
}

IMPLEMENT
void Gdb_serv::attache()
{
  attached = true;
}

IMPLEMENT
int Gdb_serv::write( char const *str, size_t len )
{
  if(!attached)
    return _o->write(str,len);

  send( '$' );
  send( 'O' );

  while(len--)
    {
      send( hexc[*str >> 4] );
      send( hexc[*(str++) % 16] );
    }

  send( '#' );

  _o->getchar(true);

  return len;
}

static unsigned char from_hex(char c)
{
  if(c>='0' && c<='9')
    return c-'0';
  if(c>='a' && c<='f')
    return c-'a'+10;
  if(c>='A' && c<='F')
    return c-'A'+10;
  return 0;
}

PRIVATE
int Gdb_serv::get_packet(char *buffer, size_t len)
{
  attached = true;
  unsigned char cs=0, cs2=0;
  char *b = buffer;
  enum {
    NONE,
    IN_PCK,
    IN_CS1,
    IN_CS2,
  } state = NONE;

  while((unsigned)(b-buffer) < len)
    {
      char c = _o->getchar(true);
      switch(state) 
        {
        case NONE:
          if(c=='$')
            {
              state = IN_PCK;
              cs = 0;
            }
          break;

        case IN_PCK:
          if(c=='#')
            state = IN_CS1;
          else 
            {
              cs += c;
              if((unsigned)(b-buffer) < sizeof(buffer))
                *(b++) = c;
            }
          break;
        case IN_CS1:
          cs2 = from_hex(c) << 4;
          state = IN_CS2;
          break;
        case IN_CS2:
          cs2 |= from_hex(c);
          if(cs2==cs)
            _o->write("+",1);
          else
            _o->write("-",1);
          state = NONE;
          return b-buffer;
        }
    }
  return -1;
}

IMPLEMENT
int Gdb_serv::getchar( bool blocking )
{
  return _o->getchar(blocking);
}

IMPLEMENT
int Gdb_serv::char_avail() const
{
  return _o->char_avail();
}

PUBLIC
char const *Gdb_serv::next_attribute( bool restart = false ) const
{
  return _o->next_attribute(restart);
}

PRIVATE
void Gdb_serv::snd_packet( char const *c )
{
  do 
    {
      send('$');
      while(*c)
        send(*(c++));
      
      send('#');
    }
  while(_o->getchar(true) == '-');
      
}

IMPLEMENT
void Gdb_serv::send_byte( char c )
{
  send( hexc[c >> 4] );
  send( hexc[c % 16] );
}

IMPLEMENT 
void Gdb_serv::send( char const *str )
{
  while(*str)
    send(*(str++));
}

IMPLEMENT
void Gdb_serv::send( char c ) 
{
  static char pbuf[256];
  static char *b = 0;
  static unsigned char cs=0;
  if(c=='$') 
    {
      b = pbuf;
      cs = 0;
    }
  
  if((b>=pbuf) && ((unsigned)(b-pbuf)<sizeof(pbuf)))
    *(b++) = c; 
  
  if(c!='$' && c!='#') 
    cs += c;
  
  if((b>pbuf) && (c=='#' || ((unsigned)(b-pbuf)==sizeof(pbuf))))
    {
      _o->write(pbuf, b-pbuf);
      b = pbuf;
    }

  if(c=='#') 
    {
      pbuf[0] = hexc[cs>>4];
      pbuf[1] = hexc[cs % 16];
      _o->write(pbuf,2);
      
    }
}

IMPLEMENT
void Gdb_serv::send_registers()
{
  send('$'); 
  unsigned regs = num_registers();
  for(unsigned i = 0; i<regs; i++)
    {
      get_register(i);
    }
  send('#');
}

IMPLEMENT
void Gdb_serv::enter()
{
  static char buffer[256];
  snd_packet("S05");

  while(1) 
    {
      int len = get_packet(buffer,sizeof(buffer));
      if(len<0)
        continue;

      switch(buffer[0])
        {
        case '?':
          snd_packet("S05");
          break;
        case 'q':
          snd_packet("E01");
          break;
        case 'Q':
          snd_packet("E02");
          break;
        case 'H':
          snd_packet("E03");
          break;
        case 'g':
          send_registers();
          break;
        case 'G':
          snd_packet("E05");
          break;
        case 'd':
          snd_packet("");
          break;
        case 'r':
          snd_packet("E06");
          break;
        case 'm':
        case 'M':
        case 'X':
          snd_packet("E07");
          break;
        case 'c':
          return;
        default:
          snd_packet("E08");
#if 0
          write("Hallo GDB\n",10);
          write(buffer,len);
          write("\n",1);
#endif
          break;
        }
    }
}
