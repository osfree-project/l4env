INTERFACE:

#include "static_init.h"
#include "jdb_module.h"

/**
 * @brief The core of the modularized Jdb.
 * @see Jdb_module
 * @see Jdb_category
 *
 * This class provides the core functions for handling
 * Jdb_modules and providing them with the right input.
 *
 */
class Jdb_core
{
public:

  /**
   * @brief The command structure for Jdb_core.
   * 
   * This structure consists of a pointer to the Jdb_module
   * and a Jdb_module::Cmd structure. It is used in exec_cmd()
   * and returned from has_cmd().
   */
  struct Cmd
  {
    /**
     * @brief Pointer to the module providing this command.
     */
    Jdb_module            *mod;

    /**
     * @brief The Jdb_module::Cmd structure, describing the 
     *        command.
     *
     * If this is a null pointer the command is invalid.
     * @see Jdb_module
     * @see Jdb_module::Cmd
     */
    Jdb_module::Cmd const *cmd;

    /**
     * @brief Create a Jdb_core::Cmd.
     * @param _mod the Jdb_module providing the command.
     * @param _cmd the command structure (see Jdb_module::Cmd).
     */
    Cmd( Jdb_module *_mod, Jdb_module::Cmd const *_cmd = 0 ) 
      : mod(_mod), cmd(_cmd) 
    {}
  };

  /**
   * @brief Get the command structure accoring to the given name.
   * @param cmd the command to look for.
   * @return A valid Cmd structure if cmd was found, or a
   *         Cmd structure where Cmd::cmd is a null pointer if
   *         no module provides such a command.
   */
  static Cmd has_cmd( char const *cmd );

  /**
   * @brief Execute the command according to cmd.
   * @param cmd the command structure (see Jdb_core::Cmd), which
   *        describes the command to execute.
   * @return 1 if the input was aborted or the Jdb_module::action() 
   *         method returned NOTHING. 0 if the Jdb_module::action() 
   *         method returned LEAVE. 2 if we got KEY_HOME.
   *
   * This method is actually responsible for reading the input
   * with respect to the commands format string and calling
   * the Jdb_module::action() method after that.
   *
   */
  static int exec_cmd( Cmd const cmd, int push_next_char = -1 );

  /**
   * @brief Overwritten getchar() to be able to handle next_char.
   */
  static int getchar( void );

private:

  static Jdb_module *_first;
  static bool short_mode;
  static int  next_char;

};


IMPLEMENTATION:

#include <cstring>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cctype>
#include "l4_types.h"
#include "kernel_console.h"
#include "keycodes.h"
#include "simpleio.h"

bool Jdb_core::short_mode = true;

Jdb_module *Jdb_core::_first = 0;
int  Jdb_core::next_char  = -1;

IMPLEMENT
Jdb_core::Cmd Jdb_core::has_cmd( char const *cmd )
{
  Cmd c(Jdb_module::first());
  while(c.mod)
    {
      c.cmd = c.mod->has_cmd( cmd, short_mode );
      if(c.cmd)
	return c;

      c.mod = c.mod->next();
    }

  return c;
}

IMPLEMENT
int Jdb_core::getchar( void )
{
  if (next_char != -1)
    {
      int c = next_char;
      next_char = -1;
      return c;
    }
  
  return Kconsole::console()->getchar();
}

IMPLEMENT
int Jdb_core::exec_cmd( Cmd const cmd, int push_next_char = -1 )
{
  char const* f = cmd.cmd->fmt;
  char const* f1;

  //char args[256];
  char *argbuf = (char*)cmd.cmd->argbuf;

  enum {
    NORMAL,
    UNSIGNED,
    MULTI,
  } num_mode;

  int num_pos = 0;
  int num_base = 10;
  int max_len = 0;
  int c,cv;
  char fm;

  next_char = push_next_char;

  do {

    char *next_arg = argbuf;
    char const *old_f = f;
    while(*f) 
      {
	f1 = f;
      
	while(*f && *f!='%')
	  ++f;
      
	putnstr( f1, (f-f1) );
      
	if(*(f++))
	  {
	    int long_fmt = 0;
	    bool negative = false;
	    long long int val = 0;
	    max_len = 0;
	  
	  next_fmt:
	    if(*(f))
	      switch((fm=*(f++)))
		// Attention: Each case statement must finish with "continue"
		//            else it falls through to int_done!
		{
		case '0': case '1': case '2':
		case '3': case '4': case '5':
		case '6': case '7': case '8':
		case '9':
		  max_len = max_len * 10 + fm - '0';
		  goto next_fmt;
		case '%':
		  putchar('%');
		  continue;

		case 'l':
		  long_fmt++;
		  if(long_fmt > 2) 
		    long_fmt = 2;
		  goto next_fmt;

		case 'h':
		  long_fmt--;
		  if(long_fmt < -1)
		    long_fmt = -1;
		  goto next_fmt;

		case 'i':
		  num_base = 10;
		  num_mode = MULTI;
		  goto input_num;

		case 'p':
		  num_base = 16;
		  num_mode = UNSIGNED;
		  if(sizeof(short int)==sizeof(void*))
		    long_fmt = -1;
		  else if(sizeof(int)==sizeof(void*))
		    long_fmt = 0;
		  else if(sizeof(long int)==sizeof(void*))
		    long_fmt = 1;
		  else //if(sizeof(long long int)==sizeof(void*))
		    long_fmt = 2;
		  goto input_num;

		case 'o':
		  putchar('0');
		  num_mode = UNSIGNED;
		  num_base = 8;
		  goto input_num;

		case 'X':
		case 'x':
		  num_mode = UNSIGNED;
		  num_base = 16;
		  goto input_num;

		case 'd':
		  num_mode = NORMAL;
		  num_base = 10;
		  goto input_num;

		case 'u':
		  num_mode = UNSIGNED;
		  num_base = 10;
		  goto input_num;

		input_num:
		  num_pos = 0;
		  while((c = getchar()) != ' ' && c!=KEY_RETURN)
		    {
		      if(c==KEY_ESC)
			return 1;
		      
		      if(c==KEY_BACKSPACE && num_pos>0)
			{
			  putstr("\b \b");
			  if(num_pos == 1 && negative)
			    negative = false;
			  else if(num_pos==1 && num_mode==MULTI && num_base==8)
			    num_base = 10;
			  else if(num_pos==2 && num_mode==MULTI && num_base==16)
			    num_base = 8;
			  else
			    val = val / num_base;

			  num_pos--;
			  continue;
			}
		      else if(num_mode!=UNSIGNED && num_pos==0 && c=='-')
			{
			  num_pos++;
			  putchar(c);
			  negative = true;
			  continue;
			}
		      else if(num_mode==MULTI && num_pos==0 && c=='0')
			{
			  num_pos++;
			  putchar(c);
			  num_base = 8;		      
			  continue;
			}
		      else if(num_mode==MULTI && num_pos==1 && num_base==8 
			      && (c=='x' || c=='X'))
			{
			  num_pos++;
			  putchar(c);
			  num_base = 16;
			  continue;
			}
		      else if(num_pos==1 && (c=='x' || c=='X'))
			{
			  // ignore 0x to allow direct pasting of addresses
			  num_pos--;
			  putstr("\b \b");
			  continue;
			}
		      else if(c>='0' && c<='9')
			{
			  cv = c-'0';
			}
		      else if((c|0x20)>='a' && (c|0x20)<='f')
			{
			  cv = (c|0x20) - 'a' + 10;
			}
		      else
			continue;

		      if(cv < num_base)
			{
			  num_pos++;
			  putchar(c);
			  val = val * num_base + cv;
			  if ((max_len != 0) && (num_pos >= max_len))
			    break;
			}
		    }
		  goto int_done;

		case 't':
		    {
		      int digits[2] = {0, 0};
		      int state     = 0;
		      Mword num     = 0;
		      L4_uid tid;
		      while((c = getchar()) != ' ' && c!=KEY_RETURN)
			{
			  cv = -1;

			  if(c==KEY_ESC)
			    return 1;

			  if(c==KEY_BACKSPACE)
			    {
			      if(digits[state])
				{
				  num /= 16;
				  digits[state]--;
				}
			      else if (state)
				{
				  state--;
				  num = tid.task();
				}
			      else
				continue;
			      putstr("\b \b");
			      continue;
			    }

			  if(c>='0' && c<='9')
			    {
			      cv = c-'0';
			    }
			  else if((c|0x20)>='a' && (c|0x20)<='f')
			    {
			      cv = (c|0x20) - 'a' + 10;
			    }

			  if (cv != -1)
			    {
			      num = num*16 + cv;
			      digits[state]++;
			      putchar(c);
			    }

			  if (state)
			    {
			      if (digits[1] == 2)
				break;
			    }
			  else
			    {
			      if (digits[0] < 4 && digits[0] > 0 && c == '.')
				{
				  tid.task(num);
				  num = digits[1] = 0;
				  state++;
				  putchar('.');
				}
			      else if (digits[0] == 8)
				break;
			    }
			}
		      if (state)
			tid.lthread(num);
		      else
			{
			  if (digits[0] > 3)
			    tid = num;
			  else if (digits[0] > 0)
			    {
			      tid.task(num);
			      tid.lthread(0);
			    }
			  else
			    tid = L4_uid::INVALID;
			}
		      *(((L4_uid*)next_arg)++) = tid;
		    }
		  continue;

		case 'C':
		case 'c':
		  {
		    int c = getchar();
		    if(c==KEY_ESC)
		      return 1;

		    if(fm == 'c' && isprint(c))
		      putchar(c);
		    if(next_arg)
		      *next_arg++ = c;
		  }
		  continue;

		case 's':
		case 'S':
		  if(!max_len)
		    continue;

		  num_pos = 0;
		  while((c = getchar()) != KEY_RETURN && c!=' ')
		    {
		      if(c==KEY_ESC)
			return 1;

		      if(c==KEY_BACKSPACE && num_pos)
			{
			  putstr("\b \b");
			  num_pos--;
			}
		      else if(isprint(c) && (num_pos+1 < max_len))
			{
			  putchar(c);
			  next_arg[num_pos++] = c;
			}
		      if (fm=='S')
			{
			  int oldlen = num_pos;
			  next_arg[num_pos] = '\0';
			  (**(Jdb_module::Gotkey**)(next_arg+max_len))
			    (next_arg, max_len-1, c);
			  int newlen = strlen(next_arg);
			  if (newlen > oldlen)
			    printf("%s", next_arg + oldlen);
			  else if (newlen < oldlen)
			    for (int i=newlen; i<oldlen; i++)
			      putstr("\b \b");
			  num_pos = newlen;
			}
		    }
		  next_arg[max_len-1] = 0;
		  next_arg += max_len;
		  continue;

		default:
		  puts(" unknown format! ");
		  return 1;
		}

	  int_done:
	    if(negative) val = -val;

	    if(next_arg)
	      switch(long_fmt)
		{
		default:
		  *(((int*)next_arg)++) = val;
		  break;
		case 1:
		  *(((long int*)next_arg)++) = val;
		  break;
		case 2:
		  *(((long long int*)next_arg)++) = val;
		  break;
		case -1:
		  *(((short int*)next_arg)++) = val;
		  break;
		}
	  }
	else
	  break;
      }

    f = old_f;

    switch(cmd.mod->action( cmd.cmd->id, (void*&)argbuf, f, next_char ))
      {
      case Jdb_module::EXTRA_INPUT:
	next_char = -1;
	// fall through
      case Jdb_module::EXTRA_INPUT_WITH_NEXTCHAR:
	break;
      case Jdb_module::LEAVE:
	return 0;
      case Jdb_module::GO_BACK:
	return 2;
      case Jdb_module::NOTHING:
      default:
	return 1;
      }
  } while(1);

}



//===================
// Std JDB modules
//===================


/**
 * @brief Private 'go' module.
 * 
 * This module handles the 'go' or 'g' command 
 * that continues normal program execution.
 */
class Go_m 
  : public Jdb_module
{
public:

};

static Go_m go_m INIT_PRIORITY(JDB_MODULE_INIT_PRIO);

PUBLIC
Go_m::Go_m()
  : Jdb_module("GENERAL")
{}

PUBLIC
Jdb_module::Action_code Go_m::action( int, void *&, char const *&, int & )
{
  return LEAVE;
}

PUBLIC
int const Go_m::num_cmds() const
{ 
  return 1;
}

PUBLIC
Jdb_module::Cmd const *const Go_m::cmds() const
{
  static Cmd cs[] =
    { Cmd( 0, "g", "go", "\n",
	   "g\tleave kernel debugger\n"
	   "Return\tshow debug message", 0 ),
    };

  return cs;
}


/**
 * @brief Private 'help' module.
 * 
 * This module handles the 'help' or 'h' command and 
 * prints out a help screen.
 */
class Help_m : public Jdb_module
{
public:
};


static Help_m help_m INIT_PRIORITY(JDB_MODULE_INIT_PRIO);


PUBLIC
Jdb_module::Action_code Help_m::action( int, void *&, char const *&, int & )
{
  size_t const tab_width = 27;

  Jdb_category const *c = Jdb_category::first();
  if(!c)
    {
      printf("There seems to be no debugger commands registerd\n");
      return NOTHING;
    }

  unsigned line = 0;

  while(c) 
    {
      bool first = true;

      Jdb_category::Iterator i = c->modules();
      Jdb_module const *m;
 
      while((m = i.next()))
	{
	  if(first) 
	    {
	      putchar('\n');
	      if(!new_line(line))
		return NOTHING;
	      printf("\033[1m[%s]\033[0m %s", c->name(), c->description());
	      putchar('\n');
	      if(!new_line(line))
		return NOTHING;
	      first = false;
	    }
	  unsigned ncmds = m->num_cmds();
	  Jdb_module::Cmd const *cmds = m->cmds();
	  for(unsigned x = 0; x < ncmds; x++)
	    {
	      char const *descr = cmds[x].descr;
	      if (!descr)
		continue;
	      size_t pos = strcspn(descr, "\n\t");
	      size_t xpos = 2;
	      putstr("  ");
	      while( pos  < strlen(descr)) 
		{
		  putnstr(descr, pos);
		  xpos += pos;
		  switch(descr[pos]) 
		    {
		    case '\n': 
		      putchar('\n');
		      if(!new_line(line))
			return NOTHING;
		      putstr("  ");
		      xpos = 2;
		      break;
		    case '\t':
		      if(xpos<tab_width)
			{
			  putnstr("                            ",
				  tab_width-xpos); 
			  xpos = tab_width;
			}
		      else
			{
			  putchar(' ');
			  xpos++;
			}
		      break;
		    default:
		      break;
		    }
		  descr = descr + pos + 1;
		  pos = strcspn(descr, "\n\t");
		}
							
	      putstr(descr); 
	      putchar('\n');
	      if(!new_line(line))
		return NOTHING;
	    }
	}

      c=c->next();
    }

  putchar('\n');

  return NOTHING;
}

PUBLIC
int const Help_m::num_cmds() const
{ 
  return 2;
}

PUBLIC
Jdb_module::Cmd const *const Help_m::cmds() const
{
  static Cmd cs[] =
    {
      Cmd( 0, "h", "help", "\n", "h\tShow this help screen.", 0 ),
      Cmd( 0, "?", "help", "\n", 0, 0 ),
    };

  return cs;
}

PUBLIC
Help_m::Help_m()
  : Jdb_module("GENERAL")
{}



static INIT_PRIORITY(JDB_CATEGORY_INIT_PRIO) Jdb_category std_cats[] = {
  Jdb_category("GENERAL","general debugger commands",0),
  Jdb_category("INFO","information about kernel state",1),
  Jdb_category("MONITORING","monitoring kernel events",2),
  Jdb_category("DEBUGGING","real debugging stuff",3)
};

