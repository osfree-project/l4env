#include "internal.h"

/******************************************************************************
 * contxt_write                                                               *
 *                                                                            *
 * in:    s           ... output string                                       *
 *        len         ... string length                                       *
 * ret:                                                                       *
 *                                                                            *
 * print a string (low-level)                                                 *
 *****************************************************************************/
void 
contxt_write(const l4_uint8_t *s, int len)
{
  l4_uint8_t c, strbuf[vtc_cols];
  int sidx, bidx = 0, x = sb_x;
    
  for(sidx = 0; sidx < len; sidx++) 
    {
      c = s[sidx];
    
      if(c == '\n') 
	{
	  _flush(strbuf, bidx, 1);
	  bidx = 0;
	  x    = 0;
	  continue;
	}
      if(  (bidx >= vtc_cols)   /* buffer overrun */
	 ||(   x == vtc_cols))  /* wrap at right screen limit */
      {
	_flush(strbuf, bidx, 1);
	bidx = 0;
	x    = 0;
      }
      strbuf[bidx++] = c;
      x++;
    }
  
  if(bidx != 0)
    _flush(strbuf, bidx, 0);
}

