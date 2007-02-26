/* libc includes */
#ifdef USE_OSKIT
#include <malloc.h>
#endif
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* L4 includes */
#include <l4/env/errno.h>
#include <l4/log/l4log.h>
#include "direct.h"
#include "direct_ihb.h"

#ifdef USE_DIETLIBC
#undef getchar
#endif

#define KEY_UP		0xffffffb8
#define KEY_DOWN	0xffffffb2
#define KEY_LEFT	0xffffffb4
#define KEY_RIGHT	0xffffffb6
#define KEY_HOME	0xffffffb7
#define KEY_END		0xffffffb1
#define KEY_BACKSPACE	0x08
#define KEY_INSERT	0xffffffb0
#define KEY_DELETE	0xffffffae
#define KEY_ENTER	0x0d
#define KEY_ESC		0x1b
#define KEY_CTRL_R	0x88

static int sb_x;

static int
_my_getchar(void)
{
  return getchar();
}

/* make sure that we edit the right entry of history buffer */
static void
_set_edit_current(direct_ihb_t *ihb, int *line)
{
  if (*line != ihb->last)
    {
      /* we are about to edit something, so copy the
       * current history buffer line into the edit buffer */
      memcpy(ihb->buffer + ihb->length*ihb->last,
	     ihb->buffer + ihb->length*(*line),
	     ihb->length);
      *line = ihb->last;
    }
}

static void
_backwards(int count)
{
  for (; count > 0; count--)
    putchar('\b');
}

static void
_forwards(int count, const char *str)
{
  for (; count > 0; count--)
    putchar(*str++);
}

/* CTRL+R: search in history (similar to bash) */
static int
_search_ihb(direct_ihb_t *ihb, int *line, int x0, int maxlen, int *idx)
{
  int ch;
  char search[20];
  char *s   = 0;
  int found_l = 0;
  int until_l = 0;
  char *found_s = 0;
  int s_idx = 0;
  int l = ihb->last-1;

  /* start with empty search string */
  *search = '\0';
  /* clear line */
  printf("%*s", -maxlen, "");
  
  for (;;)
    {
      switch (ch = _my_getchar())
	{
	case KEY_UP:
	case KEY_DOWN:
	case KEY_END:
	case KEY_HOME:
	case KEY_LEFT:
	case KEY_RIGHT:
	case KEY_DELETE:
	case KEY_BACKSPACE:
	case KEY_INSERT:
	case KEY_ESC:
	case KEY_ENTER:
	  if (found_s)
	    {
	      /* search string found */
	      *idx  = found_s - ihb->buffer - ihb->length*found_l;
	      *line = found_l;
	    }
	  else
	    {
	      /* string not found */
	      *idx  = strlen(ihb->buffer + ihb->length*ihb->last);
	      *line = ihb->last;
	    }
	  
	  printf("%*s", *idx, "");
	  return ch;
	default:
	  if (ch)
	    {
	      char *buff;

	      if (ch == KEY_CTRL_R)
		{
		  /* search next */
		  until_l = l;
		}
	      else if (s_idx < sizeof(search)-1)
		{
		  /* begin search + add character to search string */
		  search[s_idx  ] = ch;
		  search[s_idx+1] = '\0';
		  until_l = ihb->last;
		}
	      
	      l = until_l-1;
    	      for ( ; ; l--)
		{
		  if (l < 0)
		    /* wrap around */
		    l = ihb->lines-1;

		  buff = ihb->buffer + ihb->length*l;
		  if ((s = strstr(buff, search)))
		    {
		      /* found -- append last char to search str */
		      if (search[s_idx] != '\0')
			s_idx++;
		      sb_x = x0;
		      /* print line we found */
		      printf("%*s", -maxlen, buff);
		      sb_x = x0 + (s - buff) + s_idx;
		      found_l = l;
		      found_s = s + s_idx;
		      break;
		    }
		  if (l == until_l)
		    {
		      /* not found -- don't append last char */
		      search[s_idx] = '\0';
		      break;
		    }
		}
	      break;
	    }
	}
    }
}
  
/**
 * \brief  Read a number of character with input history buffer (ihb)
 *
 * \param  maxlen          ... maximum length of return string
 *
 * \retval retstr          ... return string
 * \retval ihb	           ... input history buffer
 *
 * Read a number of character and store it in the input history buffer.
 */
static void
_read_ihb(char* retstr, int maxlen, direct_ihb_t* ihb)
{
  int idx = 0;		/* current position of current line */
  int line;		/* current line */
  int pos_x;		/* current x position */
  int len;		/* length of current string */
  int abort = 0;	/* leave loop if != 0 */
  int insert = 1;	/* 0: replace character / 1: insert character */
  unsigned int ch;	/* key */

  /* consider terminating zero in retstr */
  maxlen--;
  
  /* sanity checks */
  if (maxlen < 1)
    maxlen = 1;
  /* consider terminating zero in history buffer */
  if (maxlen > ihb->length-1)
    maxlen = ihb->length-1;

  /* make room for additional entry in history buffer */
  if (ihb->buffer[ihb->length*ihb->last] != '\0')
    {
      ihb->last++;
      if (ihb->last >= ihb->lines)
	ihb->last = 0;
      if (ihb->last == ihb->first)
	ihb->first++;
      if (ihb->first >= ihb->lines)
	ihb->first = 0;
    }
  /* else
   *   is first entry in history or last entry was empty
   *   ==> we don't need a new entry */

  /* from now on, ihb->first is the oldest line in the history buffer and
   * ihb->last is the current (to be edited) line */

  /* clear current line */
  ihb->buffer[ihb->length*ihb->last] = '\0';
  
  /* clear return string */
  *retstr = '\0';

  /* line points to the current line. If different from ihb->last, we are
   * scrolling in the history buffer */
  line = ihb->last;
  
  ch = 0;
  while (!abort)
    {
      pos_x = sb_x;
      len = strlen(ihb->buffer + ihb->length*line);
      
      if (!ch)
	ch = _my_getchar();
      switch (ch)
	{
	case KEY_UP:
	  /* one history entry up */
	  if (ihb->first != ihb->last)
	    {
	      if (line == ihb->first)
		line = ihb->last;
	      else
		{
		  line--;
		  if (line < 0)
		    line = ihb->lines-1;
		}
	      _backwards(idx);
	      /* print next previous line in history buffer */
	      printf("%s\033[K", ihb->buffer + ihb->length*line);
	      /* goto end of line */
	      idx = strlen(ihb->buffer + ihb->length*line);
	    }
	  /* else no entry in history buffer */
	  break;
	case KEY_DOWN:
	  /* one history entry down */
	  if (ihb->first != ihb->last)
	    {
	      if (line == ihb->last)
		line = ihb->first;
	      else
		{
		  line++;
		  if (line >= ihb->lines)
		    line = 0;
		}
	      _backwards(idx);
	      /* print next line in history buffer */
	      printf("%s\033[K", ihb->buffer + ihb->length*line);
	      /* goto end of line */
	      idx = strlen(ihb->buffer + ihb->length*line);
	    }
	  /* else no entry in history buffer */
	  break;
	case KEY_LEFT:
	  /* one character left in current line */
	  if(idx > 0)
	    {
	      _backwards(1);
	      idx--;
	    }
	  break;
	case KEY_RIGHT:
	  /* one character right in current line */
	  if (idx < len)
	    {
	      _forwards(1, ihb->buffer + ihb->length*line + idx);
	      idx++;
	    }
	  break;
	case KEY_HOME:
	  /* first character in current line */
	  if (idx > 0)
	    {
	      _backwards(idx);
	      idx = 0;
	    }
	  break;
	case KEY_END:
	  /* last character in current line */
	  if (idx < len)
	    {
	      _forwards(len-idx, ihb->buffer + ihb->length*line + idx);
	      idx = len;
	    }
	  break;
	case KEY_DELETE:
	  if (len <= 0 || idx >= len)
	    break;
	  _forwards(1, ihb->buffer + ihb->length*line + idx);
	  idx++;
	  /* fall through */
	case KEY_BACKSPACE:
	  /* clear previous character in current line */
	  _set_edit_current(ihb, &line);
	  if(idx > 0)
	    {
	      char *buf = ihb->buffer + ihb->length*ihb->last;
	      int k;
	      
	      /* buf[length] is ever '\0' */
	      for (k=idx; k<=len; k++)
		buf[k-1] = buf[k];
	      
	      _backwards(1);
	      idx--;
	      /* re-print rest of line */
	      printf("%s ", buf + idx);
	      _backwards(strlen(buf+idx)+1);
	    }
	  break;
	case KEY_INSERT:
	  /* insert mode <==> replace mode */
       	  insert = !insert;
	  break;
	case KEY_ENTER:
	  /* commit buffer */
	  abort = 1;
	  break;
	case KEY_ESC:
	  /* abort */
	  return;
	default:
	  /* add character to buffer */
	  _set_edit_current(ihb, &line);
	  if (ch)
	    {
	      char *buf = ihb->buffer + ihb->length*ihb->last;

	      if (ch == KEY_CTRL_R && ihb->first != ihb->last)
		{
		  pos_x -= idx;
		  ch = _search_ihb(ihb, &line, pos_x, maxlen, &idx);
		  sb_x = pos_x + idx;
		  continue; /* continue using last typed character */
		}
	      else
		{
		  if (insert)
		    {
		      /* insert character */
		      if (len < maxlen)
			{
			  int k;
			  
			  for (k=len; k>idx; k--)
			    buf[k] = buf[k-1];
			  
			  buf[k] = ch;
			  /* re-print rest of line */
			  printf("%s", buf + idx);
			  _backwards(strlen(buf+idx)-1);
			  idx++;
			}
		    }
		  else
		    {
		      /* replace current character */
		      if (idx < maxlen)
			{
			  buf[idx++] = ch;
			  if (idx == len)
			    /* character was appended */
			    buf[idx] = '\0';
			  putchar(ch);
			}
		    }
		}
	      break;
	    }
	}
      
      ch = 0;
    } 
 
  _set_edit_current(ihb, &line);
  
  memcpy(retstr, ihb->buffer + ihb->length*ihb->last, maxlen);
  retstr[maxlen] = '\0';
}


/**
 * \brief  Read a number of character without input history buffer (ihb).
 *
 * \param  maxlen          ... maximum length of return string
 *
 * \retval retstr          ... return string
 */
static void
_read_noihb(char *retstr, int maxlen)
{
  int idx = 0;
  int abort = 0;
  unsigned int ch;
  
  while(!abort)
    {
      switch (ch = _my_getchar())
	{
	case KEY_UP:
	case KEY_DOWN: 
	case KEY_RIGHT:
	case KEY_LEFT:
	  break;
	case KEY_BACKSPACE:
	  if (idx>0)
	    {
	      retstr[--idx] = 0;
	      sb_x--;
	    }
	  break;
	case KEY_ENTER:
	  abort = 1;
	  break;
	default:
	  if(idx < maxlen-1) /* consider terminating 0 */
	    {
	      if(ch)
		{
		  retstr[idx++] = ch;
		  putchar(ch);
		}
	    }
	  break;
	}
    }
  
  retstr[idx] = 0;
}


/**
 * \brief   Reads a maxcount of character
 *
 * \param   maxlen         ... size of return string buffer
 *
 * \retval  retstr         ... return string buffer
 * \retval  ihb            ... used input history buffer
 *                             if 0, no input history buffer will be used
 *
 * This function reads a number (maximum maxlen) of character.
 */
void
direct_ihb_read(char* retstr, int maxlen, direct_ihb_t* ihb)
{ 
  if (maxlen < 2)
    return;
  
  if(ihb) 
    _read_ihb(retstr, maxlen, ihb);
  else
    _read_noihb(retstr, maxlen);
}


/**
 * \brief   Init of input history buffer
 *
 * \param   ihb            ... input history buffer
 * \param   lines          ... number of lines
 * \param   lenght         ... number of characters per line
 *          
 * This is the init-function of the input history buffer. It allocates the
 * history buffer.
 */
int
direct_ihb_init(direct_ihb_t *ihb, int lines, int length)
{  
  if (lines < 2)
    lines = 2;
  if (length < 2)
    length = 2;

  /* consider terminating '\0' */
  length++;
  
  ihb->lines  = lines;
  ihb->first  = 0;
  ihb->last   = 0;
  ihb->length = length;
 
  /* allocate history buffer */
  if (!(ihb->buffer = (char*)(malloc(lines*length))))
    {
      LOGl("no mem for ihb->head");
      return -L4_ENOMEM;
    }

  /* empty whole history buffer */
  memset(ihb->buffer, 0, lines*length);
  
  return 0;
}

/** Add string to history buffer
 *
 * \param   ihb            ... input history buffer structure
 * \param   s              ... string to add
 */
void
direct_ihb_add(direct_ihb_t *ihb, const char *s)
{
  if (ihb && ihb->buffer)
    {
      char *ptr = ihb->buffer + ihb->last*ihb->length;

      ihb->last++;
      if (ihb->last >= ihb->lines)
	ihb->last = 0;
      if (ihb->last == ihb->first)
	ihb->first++;
      if (ihb->first >= ihb->lines)
	ihb->first = 0;
      strncpy(ptr, s, ihb->length-1);
      ptr[ihb->length-1] = '\0';
    }
}

