INTERFACE:

#include "console.h"
#include "types.h"

class Gdb_serv : public Console
{
public:

  typedef unsigned long Tid;

  class Tlb
  {
  public:

    Tlb( unsigned long mask = -1UL, unsigned long tag = -1UL, 
	 unsigned long addr = 0 )
      : mask(mask), addr(addr), tag (tag)
    {}
    
    bool valid() const
    { return mask != -1UL && tag != -1UL; }
  
    template< typename T >
    bool match( T *v_addr ) const
    { return ((unsigned long)v_addr & mask) == tag; }

    template< typename T >
    T* translate( T *v_addr ) const
    { return (T*)((unsigned long)v_addr & ~mask) + addr; }

  private:
    unsigned long mask;
    unsigned long addr;
    unsigned long tag;
      
  };

  Gdb_serv( Console *o );
  ~Gdb_serv();

  int write( char const *str, size_t len );
  int getchar( bool blocking = true );
  int char_avail() const;

  void enter(int sig);
  void attach();

  virtual unsigned num_registers() const = 0;
  virtual bool push_register( unsigned index ) const = 0;
  virtual unsigned char *get_register( unsigned index, unsigned &size ) = 0;
  virtual unsigned pc_index() const = 0;

  virtual Tlb lookup( unsigned long addr ) const = 0;
  
  template< typename T >
  Tlb lookup( T* addr ) const
  { return lookup((unsigned long)(addr)); }

  virtual Tid get_current_thread() const;
  virtual Tid first_thread() const;
  virtual Tid next_thread() const;
  virtual char const *thread_extra_info( Tid tid ) const;
  virtual bool thread_alive( Tid tid ) const;

protected:
  virtual unsigned char *in_buffer() = 0;
  virtual unsigned in_buffer_len() const = 0;
  virtual unsigned char *out_buffer() = 0;
  virtual unsigned out_buffer_len() const = 0;

  Tid current_tid;

private:
  bool attached;
  Console *_o;

  Tlb tlb;
};


IMPLEMENTATION:

#include <cstring>
#include <cstdlib>

IMPLEMENT
Gdb_serv::Gdb_serv( Console *o )
  : attached(false), _o(o)
{}

IMPLEMENT
Gdb_serv::~Gdb_serv()
{}

static char hexc[] = "0123456789abcdef";

static unsigned char hex(char c)
{
  if(c>='0' && c<='9')
    return c-'0';
  if(c>='a' && c<='f')
    return c-'a'+10;
  if(c>='A' && c<='F')
    return c-'A'+10;
  return 0;
}

IMPLEMENT
Gdb_serv::Tid Gdb_serv::get_current_thread() const
{
  return 1;
}

IMPLEMENT
bool Gdb_serv::thread_alive( Tid ) const
{
  return 0;
}

IMPLEMENT
Gdb_serv::Tid Gdb_serv::first_thread() const
{
  return 1;
}

IMPLEMENT
Gdb_serv::Tid Gdb_serv::next_thread() const
{
  return -1UL;
}

IMPLEMENT
char const *Gdb_serv::thread_extra_info( Tid ) const
{
  return "unknown";
}

#if 0
static void hex_encode( char *buffer, char const *str, size_t len )
{
  for( size_t pos = 0; pos < len; pos++ )
    {
      buffer[pos*2]   = hexc[str[pos] >> 4];
      buffer[pos*2+1] = hexc[str[pos] & 0x0f];
    }
}
#endif

PUBLIC  
char *
Gdb_serv::mem2hex(unsigned char const *mem, 
                  char *buf, int count, int may_fault = 0)
{
  unsigned char ch;
  unsigned char const *mem1;
  
  while (count-- > 0)
    {
      if (!may_fault)
	mem1 = mem;
      else
	{
	  if (!tlb.match(mem))
	    {
	      tlb = lookup(mem);
	      if (!tlb.valid())
		return 0;
	    }
	  mem1 = tlb.translate(mem);
	}
      ch = *mem1; mem++;
      *buf++ = hexc[ch >> 4];
      *buf++ = hexc[ch & 0xf];
    }

  *buf = 0;
  
  return buf;
}

/* convert the hex array pointed to by buf into binary to be placed in mem
 * return a pointer to the character AFTER the last byte written */

PUBLIC  
unsigned char *
Gdb_serv::hex2mem(char *buf, 
                  unsigned char *mem, int count, int may_fault = 0)
{
  int i;
  unsigned char ch;
  unsigned char *mem1;
  
  for (i=0; i<count; i++)
    {
      ch = hex(*buf++) << 4;
      ch |= hex(*buf++);
     
      if (!may_fault)
	mem1 = mem;
      else
	{
	  if (!tlb.match(mem))
	    {
	      tlb = lookup(mem);
	      if (!tlb.valid())
		return 0;
	    }
	  mem1 = tlb.translate(mem);
	}
      *mem1 = ch; mem++;
    }
  return mem;
}

IMPLEMENT
void Gdb_serv::attach()
{
  attached = true;
}

IMPLEMENT
int Gdb_serv::write( char const *str, size_t len )
{
 // if(!attached)
 //   return _o->write(str,len);

  char *ptr;
  unsigned l,lx;

  lx = len;
  while (lx)
    {
      ptr = (char*)out_buffer();
      *ptr++ = 'O';
      l = lx <? (out_buffer_len()/2 -1);
      ptr = mem2hex((unsigned char const *)str, ptr, l, 0);
      str += l;
      *ptr = 0;
      
      put_packet(out_buffer());
      
      lx -= l;
      lx = 0;
    }

  return len;
}

PRIVATE
unsigned char *Gdb_serv::get_packet()
{
  attached = true;
  unsigned char *buffer = in_buffer();
  unsigned char checksum;
  unsigned char xmitcsum;
  unsigned count;
  char ch;
  while (1)
    {
      /* wait around for the start character, ignore all other characters */
      while ((ch = _o->getchar(true)) != '$')
	;

retry:
      checksum = 0;
      xmitcsum = -1U;
      count = 0;

      /* now, read until a # or end of buffer is found */
      while (count < in_buffer_len())
	{
	  ch = _o->getchar(true);
          if (ch == '$')
            goto retry;
	  if (ch == '#')
	    break;
	  checksum = checksum + ch;
	  buffer[count] = ch;
	  count = count + 1;
	}
      buffer[count] = 0;

      if (ch == '#')
	{
	  ch = _o->getchar(true);
	  xmitcsum = hex (ch) << 4;
	  ch = _o->getchar(true);
	  xmitcsum += hex (ch);

	  if (checksum != xmitcsum)
	    {
	      _o->write("-",1);	/* failed checksum */
	    }
	  else
	    {
	      _o->write("+",1);	/* successful transfer */

	      /* if a sequence char is present, reply the sequence ID */
	      if (buffer[2] == ':')
		{
		  _o->write((char const*)buffer,2);
		  return &buffer[3];
		}

	      return &buffer[0];
	    }
	}
    }
}

PRIVATE 
void Gdb_serv::put_packet (unsigned char *buffer)
{
  unsigned char checksum;
  int count;
  unsigned char ch;

  /*  $<packet info>#<checksum>. */
  do
    {
      _o->write("$",1);
      checksum = 0;
      count = 0;

      while ((ch = buffer[count]))
	{
	  checksum += ch;
	  count += 1;
	  //_o->write((char const *)&ch,1);
	}
      _o->write( (char const *)buffer, count );

      _o->write("#",1);
      _o->write(hexc + (checksum >> 4),1);
      _o->write(hexc + (checksum & 0x0f),1);

    }
  while (_o->getchar(true) != '+');
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
Mword Gdb_serv::get_attributes() const
{
  return IN | OUT | DEBUG;
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

PRIVATE 
void Gdb_serv::send( char const *str )
{
  while(*str)
    send(*(str++));
}

PRIVATE
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


void gdb_server_put_byte( unsigned char v, char *&ptr )
{
  *ptr++ = hexc[v >> 4];
  *ptr++ = hexc[v & 0x0f];
}

template< typename O >
void put_object( O const &o, char *&ptr )
{
  unsigned char *b = (unsigned char*)&o;
  for (unsigned i = sizeof(O); i>0; --i)
    gdb_server_put_byte( b[i-1], ptr );
}

template< typename O >
void put_object_hbo( O const &o, char *&ptr )
{
  unsigned char *b = (unsigned char*)&o;
  for (unsigned i = 0; i< sizeof(O); ++i)
    gdb_server_put_byte( b[i], ptr );
}

IMPLEMENT
void Gdb_serv::enter( int sig )
{
  int sigval = sig;
  char *ptr;

  tlb = Tlb(); // flush my tlb

  current_tid = get_current_thread();

  ptr = (char*)out_buffer();
  *ptr++ = 'T';
  gdb_server_put_byte(sigval, ptr );

  unsigned regs = num_registers();
  for (unsigned i=0; i<regs; ++i)
    if (push_register(i))
      {
	gdb_server_put_byte(i, ptr);
	*ptr++ = ':';
	unsigned len;
	unsigned char *reg = get_register( i, len );
	ptr = mem2hex( reg, ptr, len, 0 );
	*ptr++ = ';';
      }

  strcpy(ptr,"thread:");
  ptr += 7;

  put_object( current_tid, ptr );
  *ptr++ = ';';

  *ptr++ = 0;

  put_packet(out_buffer());

  while (1)
    {
      unsigned long addr;
      unsigned long length;
      unsigned char *obuf = out_buffer();
      char *optr = (char*)obuf;
      char *tmp_ptr;
      obuf[0] = 0;
      ptr = (char*)get_packet();
      switch (*ptr++)
	{
	case '?':
	  *optr++ = 'S';
	  gdb_server_put_byte( sigval, optr );
	  *optr++ = 0;
	  break;

	case 'd':		/* toggle debug flag */
	  break;

	case 'p':
	  {
	    addr = strtoul( ptr, &tmp_ptr, 16 );
	    unsigned len;
	    unsigned char *reg = get_register( addr, len );
	    optr = mem2hex(reg,optr,len,0);
	  }
	  break;

	case 'P':
	  {
	    addr = strtoul( ptr, &tmp_ptr, 16 );
	    if (tmp_ptr<=ptr || *tmp_ptr!='=')
	      {
	        strcpy(optr,"E01");
		break;
	      }
	    unsigned len;
	    unsigned char *reg = get_register( addr, len );
	    ptr =tmp_ptr + 1;
	    hex2mem(ptr, reg, len, 0);
	    strcpy(optr,"OK");
	  }
	  break;
	  
	case 'g':		/* return the value of the CPU registers */
	  {
	    for (unsigned i = 0; i<num_registers(); ++i)
	      {
		unsigned char *reg;
		unsigned len;
		reg = get_register( i, len );
		optr = mem2hex(reg, optr, len, 0);
	      }
	  }
	  break;

	case 'G':	   /* set the value of the CPU registers - return OK */
	  {
	    for (unsigned i = 0; i< num_registers(); ++i)
	      {
		unsigned char *reg;
		unsigned len;
		reg = get_register( i, len );
		hex2mem(ptr, reg, len , 0);
		ptr += len;
	      }
	    strcpy(optr,"OK");
	  }
	  break;

	case 'm':	  /* mAA..AA,LLLL  Read LLLL bytes at address AA..AA */
	  /* Try to read %x,%x.  */

	  addr = strtoul( ptr, &tmp_ptr, 16 );
	  if (tmp_ptr<=ptr || *tmp_ptr!=',')
	    {
	      strcpy(optr,"E01");
	      break;
	    }
	  
	  ptr = tmp_ptr+1;
	  length = strtoul( ptr, &tmp_ptr, 16 );
	  if (tmp_ptr<=ptr)
	    {
	      strcpy(optr,"E01");
	      break;
	    }
	 
	  if (mem2hex((unsigned char *)addr, optr, length, 1))
	    break;

	  strcpy (optr, "E03");
	  break;

	case 'M': /* MAA..AA,LLLL: Write LLLL bytes at address AA.AA return OK */
	  /* Try to read '%x,%x:'.  */

	  addr = strtoul( ptr, &tmp_ptr, 16 );
	  if (tmp_ptr<=ptr || *tmp_ptr!=',')
	    {
	      strcpy(optr,"E01");
	      break;
	    }
	  
	  ptr = tmp_ptr+1;
	  length = strtoul( ptr, &tmp_ptr, 16 );
	  if (tmp_ptr<=ptr || *tmp_ptr!=':')
	    {
	      strcpy(optr,"E01");
	      break;
	    }
	  
	  ptr = tmp_ptr +1;
	  
	  if (hex2mem(ptr, (unsigned char *)addr, length, 1))
	    break;
	  
	  strcpy (optr, "E03");
	  break;
	  

	case 'c':    /* cAA..AA    Continue at address AA..AA(optional) */
	  /* try to read optional parameter, pc unchanged if no parm */

	  
	  addr = strtoul( ptr, &tmp_ptr, 16 );
	  if (tmp_ptr>ptr)
	    {
	      unsigned char *reg;
	      unsigned len;
	      reg = get_register(pc_index(), len);
	      *(unsigned long*)reg = addr;
	    }
	  return;

	case 'H':
	  
	  if (*ptr == 'g')
	    {
	      if (*(ptr+1) == '-')
		{
		  strcpy(optr,"OK");
		  break;
		}
	      
	      Tid tid = strtoul( ptr+1, 0, 16 );
	      if (tid != 0)
		{
		  tlb = Tlb();
		  current_tid = tid;
		}
	    }
	  else if (*ptr == 'c')
	    {
	      break;
	    }

	  strcpy(optr,"OK");
	  break;

	case 'T':
	  {
	    Tid tid = strtoul( ptr+1,0,16);
	    if (thread_alive(tid))
	      strcpy(optr,"OK");
	    else
	      strcpy(optr,"E01");
	    break;
	  }
	  
	case 'q':
	  if (ptr[0] == 'C')
	    {
	      Tid tid = get_current_thread();
	      *optr++ = 'Q';
	      *optr++ = 'C';
	      put_object(tid,optr);
	      *optr = 0;

	      break;
	    }
	  else if (strncmp(ptr+1,"ThreadInfo",11)==0)
	    {
	      Tid tid;
	      if (*ptr == 'f')
		tid = first_thread();
	      else
		tid = next_thread();

	      if (tid == -1UL)
		*optr++ = 'l';
	      else
		{
	          *optr++ = 'm';
	          put_object( tid, optr );
		}
	      *optr = 0;
	      break;
	    }
	  else if (strncmp(ptr,"ThreadExtraInfo,",16)==0)
	    {
	      ptr += 16;
	      Tid tid = strtoul(ptr,&tmp_ptr, 16);
	      *optr = 0;
	      if (tmp_ptr>ptr)
		{
		  char const *info = thread_extra_info(tid);
		  optr = mem2hex((unsigned char const*)info, optr, 
		                 strlen(info), 0);
		}
	      break;
	    }
#if 0
	  else if (*ptr == 'P')
	    {
	      ptr++;
	      char t = ptr[8];
	      ptr[8] = 0;
	      unsigned long mode = strtoul(ptr, 0, 16);
	      ptr[8] = t;
	      ptr += 8;
	      unsigned long tid = strtoul(ptr, 0, 16);
	      if (mode == 4) 
		{
		  char const *b = thread_
		}
	      
	    }
#endif
	  else 
	    break;

	  /* kill the program */
	case 'k' :		/* do nothing */
	  break;
#if 0
	case 't':		/* Test feature */
	  asm (" std %f30,[%sp]");
	  break;
	case 'r':		/* Reset */
	  asm ("call 0 \n"
		"nop ");
	  break;
#endif
	default:
	  obuf[0] = 0;
	  break;
	}			/* switch */

      /* reply to the request */
      put_packet(obuf);
    }
}

