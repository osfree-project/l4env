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
   *         method returned LEAVE.
   *
   * This method is actually responsible for reading the input
   * with respect to the commands format string and calling
   * the Jdb_module::action() method after that.
   *
   */
  static int exec_cmd( Cmd const cmd );

private:
  
  static bool short_mode;


};


IMPLEMENTATION:

#include <cstring>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cctype>

bool Jdb_core::short_mode = true;

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
int Jdb_core::exec_cmd( Cmd const cmd )
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

  do {

    char *next_arg = argbuf;
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
		{
		case '0'...'9':
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
		  putstr("0x");
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
		  putstr("0x");
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
		  while((c = getchar()) != ' ' && c!=13)
		    {
		      if(c==27)
			return 1;
		      
		      if(c=='\b' && num_pos>0)
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
			} 
		    }
		  goto int_done;

		case 'c':
		  {
		    int c = getchar();
		    if(c==27)
		      return 1;
		    if(isprint(c))
		      putchar(c);
		    if(next_arg)
		      *next_arg++ = c;

		  }
		  continue;

		case 's':
		  if(!max_len)
		    continue;

		  num_pos = 0;
		  while((c=getchar()) != 13 && c!=' ')
		    {
		      if(c=='\b' && num_pos)
			{
			  putstr("\b \b");
			  num_pos--;
			}
		      else if(num_pos+1 < max_len)
			{
			  putchar(c);
			  next_arg[num_pos++] = c;
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

    switch(cmd.mod->action( cmd.cmd->id, (void*&)argbuf, f ))
      {
      case Jdb_module::LEAVE:
	return 0;
      case Jdb_module::NOTHING:
	return 1;
      case Jdb_module::EXTRA_INPUT:
	break;
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
Jdb_module::Action_code Go_m::action( int, void *&, char const *& )
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
	   "g\tleave kernel debugger", 0 ),
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


static unsigned new_line( unsigned &line )
{
  if (line++ > 22)
    {
      putstr("--- CR: line, SPACE: page, ESC: abort ---");
      int a = getchar();
      printf("\r\033[K");
      
      switch (a)
	{
	case 0x1b:
	case 'q':
	  putchar('\n');
	  return 0;
	case 0x0d:
	  line--;
	  return 1;
	default:
	  line=0;
	  return 1;
	}
    }
  return 1;
}

PUBLIC
Jdb_module::Action_code Help_m::action( int, void *&, char const *& )
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
	      if(!new_line(line))
		return NOTHING;
	      printf("\n\033[1m[%s]\033[0m %s",c->name(),c->description());
	      if(!new_line(line))
		return NOTHING;
	      putchar('\n');
	      first = false;
	    }
	  unsigned ncmds = m->num_cmds();
	  Jdb_module::Cmd const *cmds = m->cmds();
	  for(unsigned x = 0; x < ncmds; x++)
	    {
	      char const *descr = cmds[x].descr;
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
		      if(!new_line(line))
			return NOTHING;
		      putstr("\n  ");
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
	      if(!new_line(line))
		return NOTHING;
	      putchar('\n');
	    }
	}

      c=c->next();
    }
			
  if(!new_line(line))
    return NOTHING;
  putchar('\n');

  return NOTHING;
}

PUBLIC
int const Help_m::num_cmds() const
{ 
  return 1;
}

PUBLIC
Jdb_module::Cmd const *const Help_m::cmds() const
{
  static Cmd cs[] =
    { Cmd( 0, "h", "help", "\n",
					 "h\tShow this help screen.", 0 ),
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

