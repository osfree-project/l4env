/*****************************************************************************
 * Checksum algorithms - used for generation of hopefully non-overlapping    *
 * MAC addresses for ORe clients.                                            *
 *                                                                           *
 * Sources: http://en.wikipedia.org/wiki/CRC-32                              *
 *          http://en.wikipedia.org/wiki/Adler32                             *
 *                                                                           *
 * Bjoern Doebel <doebel@os.inf.tu-dresden.de>                               *
 * 2005-10-06                                                                *
 *																			 *
 * (c) 2005 - 2007 Technische Universitaet Dresden					         *		
 * This file is part of DROPS, which is distributed under the				 *
 * terms of the GNU General Public License 2. Please see the				 *
 * COPYING file for details.												 *
 *****************************************************************************/

#include "ore-local.h"

#define MOD_ADLER 65521

/* Adler32 checksum algorithm - said to be faster than CRC32, but this will
 * probably not matter because our checksummed data is very small.
 */
unsigned int adler32(unsigned char *buf, unsigned int len)
{
    unsigned int a=1, b=0;

    while (len) { 
        unsigned tlen = len > 5550 ? 5550 : len;
        len -= tlen;
        do {
            a += *buf++; 
            b += a;
        } while (--tlen);
        a = (a & 0xffff) + (a >> 16) * (65536-MOD_ADLER);
        b = (b & 0xffff) + (b >> 16) * (65536-MOD_ADLER);
   }
    
   /* It can be shown that a <= 0x1013a here, so a single subtract will do. */
   if (a >= MOD_ADLER)
           a -= MOD_ADLER;
   
   /* It can be shown that b can reach 0xffef1 here. */
   b = (b & 0xffff) + (b >> 16) * (65536-MOD_ADLER);
   if (b >= MOD_ADLER)
           b -= MOD_ADLER;

   return b << 16 | a; 
}

/* CRC16 checksum algorithm
 */
unsigned short crc16(unsigned char *buf, int len, short magic) 
{
    short ret       = 0;
    int cnt, cnt2;

    for (cnt=0; cnt < len; cnt++)
    {
        for (cnt2=0; cnt2 < 8; cnt2++) 
        {
            ret <<= 1;
            if ((ret & 0x80) ^ (buf[cnt] & (0x01 << cnt2))) 
                ret ^= magic;
        }
    }

    return ret;
}
 
/* CRC32
 */
unsigned int crc32(unsigned char *buf, int len, short magic) 
{
    unsigned int ret = 0;
    int cnt, cnt2;

    for (cnt=0; cnt < len; cnt++)
    {
        for (cnt2=0; cnt2 < 8; cnt2++) 
        {
            ret <<= 1;
            if ((ret & 0x80) ^ (buf[cnt] & (0x01 << cnt2))) 
                ret ^= magic;
        }
    }

    return ret;
}

