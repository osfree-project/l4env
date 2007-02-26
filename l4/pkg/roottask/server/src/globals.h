#ifndef GLOBALS_H
#define GLOBALS_H

extern int _start;		/* begin of RMGR memory -- defined in crt0.S */
extern int _stext;		/* begin of RGMR text -- defined by rmgr.ld */
extern int _stack;		/* end of RMGR stack -- defined in crt0.S */
extern int _end;		/* end of RGMR memory -- defined by rmgr.ld */
extern int _etext;		/* begin of data memory -- defined by crt0.S */

#endif
