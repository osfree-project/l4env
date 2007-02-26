/* $Id$ */
/*****************************************************************************/
/**
 * \file   lib/drivers/sound.cc
 * \brief  L4 dsound backend for Qt/Embedded.
 *
 * \date   12/04/2004
 * \author Josef Spillner <js177634@inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2005 Technische Universitaet Dresden
 * This file is part of the Qt3 port for L4/DROPS, which is distributed under
 * the terms of the GNU General Public License 2. Please see the COPYING file
 * for details.
 */

#include <l4/log/l4log.h>
#define DROPS_DSOUND 0
#if DROPS_DSOUND
#include <l4/dsound/dslib.h>
#endif

void drops_qws_sound_play(const char *filename)
{
	// lookup DROPS sound server in names
	// then do IPC call to DROPS sound server
	LOG("DROPS Soundserver request <%s>", filename);
}

#if DROPS_DSOUND
static void *dsound_buf;

int drops_qws_sound_init(void)
{
	int ret = dslib_init(1000);
	if(ret)
	{
		LOG("dsound error: init (%i)", ret);
		return -1;
	}
	ret = dslib_open_dsp(/*512 * 1024*/8192, &dsound_buf);
	if(ret)
	{
		LOG("dsound error: open_dsp (%i)", ret);
		return -1;
	}
	dslib_set_vol(0xf0f0);
	//dslib_set_frag(2, 256 * 1024);
	dslib_set_fmt(DSD_AFMT_S16_LE); // whatever...
	dslib_set_chans(2);
	dslib_set_freq(44100);
	return 0;
}

int drops_qws_sound_feed(void *buffer, int length)
{
	memcpy(dsound_buf, buffer, length);
	long len = length;
	dslib_play(&len);
	return (int)len;
}
#endif // DROPS_DSOUND

