/*
 * \brief   Audio specific for VERNER's sync component
 * \date    2004-02-11
 * \author  Carsten Rietzschel <cr7@os.inf.tu-dresden.de>
 */

/*
 * Copyright (C) 2003  Carsten Rietzschel  <cr7@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the VERNER, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */


/* L4 includes */
#include <l4/env/errno.h>
#include <l4/util/macros.h>
#include <l4/sys/types.h>
#include <l4/generic_io/libio.h>
#include <l4/dde_linux/dde.h>
#include <l4/dde_linux/sound.h>
#include <l4/dde_linux/ctor.h>
#include <l4/semaphore/semaphore.h>

/* Linux */
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/soundcard.h>

/* verner */
#include "arch_globals.h"

/* configuration */
#include "verner_config.h"

/* local */
#include "ao_oss.h"


/* 
 * Reseting DSP while playing may crash or beeing ignored by
 * the soundcard, but it could provide better sync after seeking. 
 * Try to change to zero, then we'll store bytecount when reseting
 * and take it into calculation for audio_position.
 *
 * default is "0" - bytecount resync
 */
#define RESET_HW_TO_SYNC 0

/* devices */
static int mixerdev;
/* flag if dde is initialized - this should only be done ONCE !! */
static int ao_oss_initialized = 0;

/* bytes played already by soundcard */
static unsigned long last_audio_bytes_played = 0;
/* bytes per msec audio */
static double audio_bytes_per_msec;
/* last sync point set be demuxer/core */
static double audio_sync_pts = 00.00;
/* current audio pos */
static double audio_position = 0;
/* the semaphore to protect hardware access */
static l4semaphore_t sem_hwaccess = L4SEMAPHORE_UNLOCKED;


/* internal helper functions */
static int audio_cfg_card (int audio_snddev, int afmt, int channels,
			   int rate);
static void audio_init_dde (void);


/*
 * init soundcard
 */
int
ao_oss_init (plugin_ctrl_t * attr, stream_info_t * info)
{
  int num = 0;			/* device number */
  int audio_snddev = -1;	/* -1 means not initialized */
  int ret = 0;			/* just a simple return value */

  /* info string */
  strncpy (attr->info, "OSS export plugin", 32);

  /* check valid mode */
  if (attr->mode != PLUG_MODE_EXPORT)
  {
    LOG_Error ("MODE NOT SUPPORTED (%d)", attr->mode);
    return -L4_ENOTSUPP;
  }

  /* set default values */
  audio_position = audio_sync_pts = 0.00;
  last_audio_bytes_played = 0;

  /* open hardware */
  if (!ao_oss_initialized)
  {
    /* init dde and it's calls */
    audio_init_dde ();
    l4dde_do_initcalls ();
  }
  ao_oss_initialized = 1;

  if (l4dde_process_add_worker ())
  {
    LOG_Error ("l4dde_process_add_worker() failed");
    return -1;
  }
  if ((audio_snddev = l4dde_snd_open_dsp (num)) < 0)
  {
    LOG_Error ("opening DSP %d (%d) failed! correct binary/oss-driver ?", num,
	       audio_snddev);
    audio_snddev = -1;
    return -2;
  }

  /* set default mixer values */
  ao_oss_setVolume (attr, 0xf0, 0xf0);

  /* setup samplerate etc. */
  audio_cfg_card (audio_snddev, AFMT_S16_LE, info->ai.channels,
		  (int) info->ai.samplerate);

  /* wait until we played all bytes */
  l4dde_snd_ioctl (audio_snddev, SNDCTL_DSP_SYNC, (int) &ret);

  /* reset hw to set timer to zero. crashes sometimes ?!. */
  if ((ret =
       l4dde_snd_ioctl (audio_snddev, SNDCTL_DSP_RESET, (int) &ret)) != 0)
  {
    LOG_Error ("ioctl RESET_DSP (%d) failed", ret);
  }
  audio_position = 0;

  /* check for valid values - else set SOME defaults */
  if (info->ai.bits_per_sample == 0)
    info->ai.bits_per_sample = 16;
  if (info->ai.samplerate == 0)
    info->ai.samplerate = 44100;
  if (info->ai.channels == 0)
    info->ai.channels = 2;

  /* calculate bytes per msec */
  audio_bytes_per_msec =
    (info->ai.channels * info->ai.bits_per_sample / 8 *
     info->ai.samplerate) / 1000;

  LOGdL (DEBUG_EXPORT, " - channels=%i\n", info->ai.channels);
  LOGdL (DEBUG_EXPORT, " - samplerate=%i\n", (int) info->ai.samplerate);
  LOGdL (DEBUG_EXPORT, " - bits per sample=%i\n",
	 (int) info->ai.bits_per_sample);
  LOGdL (DEBUG_EXPORT, " -> bytes_per_msec=%i\n", (int) audio_bytes_per_msec);

  /* the handle is the device no + 1 */
  attr->handle = (void *) (audio_snddev + 1);

  /* done */
  return 0;
}

/*
 * commit chung - unused
 */
int
ao_oss_commit (plugin_ctrl_t * attr)
{
  /* nothing todo */
  return 0;
}


/*
 * play one submitted chunk
 */
int
ao_oss_step (plugin_ctrl_t * attr, void *addr)
{
  int ret;
  int audio_snddev = ((int) attr->handle) - 1;
  frame_ctrl_t *frameattr = (frame_ctrl_t *) addr;
#if !RESET_HW_TO_SYNC
  count_info info;
#endif

  /* check for valid handle and mode */
  if (!attr->handle)
  {
    LOG_Error ("HANDLE FAILURE");
    return -L4_ENOHANDLE;
  }
  if (attr->mode != PLUG_MODE_EXPORT)
  {
    LOG_Error ("MODE NOT SUPPORTED");
    return -L4_ENOTSUPP;
  }
  if (!addr)
  {
    LOG_Error ("no data to export");
    return -L4_ENODATA;
  }


  /* we have a sync frame (start or seeked */
  if (is_reset_sync_point (frameattr->keyframe))
  {
    /* wait for hw accees */
    l4semaphore_down (&sem_hwaccess);

    LOGdL (DEBUG_SYNC, "last_sync_pts is about %i ms",
	   (int) frameattr->last_sync_pts);

    /* it is no wrap around - reset sound infos */
    last_audio_bytes_played = 0;
    /* remember sync pos */
    audio_sync_pts = frameattr->last_sync_pts;
    /* wait until we played all bytes */
    l4dde_snd_ioctl (audio_snddev, SNDCTL_DSP_SYNC, (int) &ret);
/* 
 * Reseting DSP while playing - it may crash. If so change to #if 0
 */
#if RESET_HW_TO_SYNC
    /* reset hw to set timer to zero. */
    if ((ret =
	 l4dde_snd_ioctl (audio_snddev, SNDCTL_DSP_RESET, (int) &ret)) != 0)
    {
      LOG_Error ("ioctl RESET_DSP (%d) failed - SYNC DOES NOT WORK!!!", ret);
    }
    audio_position = 0;
#else
/*
 * work around: we store current bytecount in audio_position  
 */
    /* now get byte count */
    if ((ret =
	 l4dde_snd_ioctl (audio_snddev, SNDCTL_DSP_GETOPTR, (int) &info)))
    {
      LOG_Error ("ERROR - ioctl GETOPTR (%d) - SYNC DOES NOT WORK!!!", ret);
    }
    audio_position = info.bytes / audio_bytes_per_msec;
    audio_position = -audio_position;
#endif

    /* end wait for hw accees */
    l4semaphore_up (&sem_hwaccess);

  }				/* end reset_sync_point */

  ret =
    l4dde_snd_write (audio_snddev,
		     (void *) addr + sizeof (frame_ctrl_t),
		     frameattr->framesize);

  if (ret < 0)
  {
    LOG_Error ("failed write (%d)", ret);
    return -1;
  }

  /* done */
  return 0;
}

/*
 * close device
 */
int
ao_oss_close (plugin_ctrl_t * attr)
{
  int err;
  int audio_snddev = ((int) attr->handle) - 1;

  /* check for valid handle */
  if (!attr->handle)
  {
    LOG_Error ("HANDLE FAILURE");
    return -L4_ENOHANDLE;
  }

  /* wait for hw accees */
  l4semaphore_down (&sem_hwaccess);

  /* closing devices */
  if ((err = l4dde_snd_close (audio_snddev)))
    LOG_Error ("releasing DSP (%d)", err);

  /* end wait for hw accees */
  l4semaphore_up (&sem_hwaccess);

  /* shutdown */
  /* XXX Currently, l4env does not provide exitcalls. If we need them,
     we have to implement them. */
#if 0
  if ((err = l4dde_snd_exit ()))
  {
    LOG_Error ("finalizing sound (%d)", err);
    return -1;
  }
#endif
  /* done */
  return 0;
}


/*
 * get playback position in millisec
 */
int
ao_oss_getPosition (plugin_ctrl_t * attr, double *position)
{
  /* bytes played already by soundcard */
  unsigned long audio_bytes_played = 0;
  /* noof snd device */
  int audio_snddev = ((int) attr->handle) - 1;
  int ret;
  count_info info;

  /* check for valid handle */
  if (!attr->handle)
  {
    LOG_Error ("HANDLE FAILURE");
    return -L4_ENOHANDLE;
  }

  /* wait for hw accees */
  l4semaphore_down (&sem_hwaccess);

  /*  calculate current audio pos */
  if ((ret = l4dde_snd_ioctl (audio_snddev, SNDCTL_DSP_GETOPTR, (int) &info)))
  {
    LOG_Error ("ioctl GETOPTR (%d) - SYNC DOES NOT WORK!!!", ret);
    return -L4_EUNKNOWN;
  }

  /* ETOPTR wrap around ? */
  audio_bytes_played = info.bytes;
  if (last_audio_bytes_played > audio_bytes_played)
  {
    LOG_Error ("GETOPTR wrap around. Good luck :) - it's UNTESTED!!!");
  }
  audio_position += audio_bytes_played / audio_bytes_per_msec;
  audio_position -= last_audio_bytes_played / audio_bytes_per_msec;
  last_audio_bytes_played = audio_bytes_played;

  /* calculate audio_position in ms */
  *position = (double) (audio_position + audio_sync_pts);

  /* end critical */
  l4semaphore_up (&sem_hwaccess);

  /* done */
  return 0;
}



/* 
 * set volume (pcm + master)
 */
int
ao_oss_setVolume (plugin_ctrl_t * attr, int left, int right)
{
  int ret;
  int master_volume = 0xf0f0;
  int left_vol;
  int right_vol;
  int pcm_volume;

  /* we ignore valid handle - cause it's not used here! */

  /* strip too many bits */
  left_vol = left & 0x7F;
  right_vol = right & 0x7F;
  pcm_volume = left_vol | (right_vol << 8);

  /* 8 bits per channel:
     31:16 undefined  15:8 right  7:0 left */
  if ((mixerdev = l4dde_snd_open_mixer (0)) < 0)
  {
    LOGdL (DEBUG_EXPORT, "opening MIXER (%d)", mixerdev);
    return 1;
  }
  /* set master volume loud */
  if ((ret =
       l4dde_snd_ioctl (mixerdev, SOUND_MIXER_WRITE_VOLUME,
			(l4_addr_t) & master_volume)))
  {
    LOGdL (DEBUG_EXPORT, "ioctl VOLUME (%d) failed. ignored.", ret);
  }

  /* set volume control via PCM out */
  if ((ret =
       l4dde_snd_ioctl (mixerdev, SOUND_MIXER_WRITE_PCM,
			(l4_addr_t) & pcm_volume)))
  {
    LOGdL (DEBUG_EXPORT, "ioctl VOLUME (%d) failed. ignored.", ret);
  }

  l4dde_snd_close (mixerdev);

  return 0;
}


/*
  configure audio card (l4dde_snd_*)
*/
static int
audio_cfg_card (int audio_snddev, int afmt, int channels, int rate)
{
  int ret;
  unsigned long arg;
  if (audio_snddev == -1)
    return -1;

  arg = (unsigned long) &afmt;
  if ((ret = l4dde_snd_ioctl (audio_snddev, SNDCTL_DSP_SETFMT, arg)))
  {
    LOG_Error ("ioctl SETFMT (%d)", ret);
    return 1;
  }

  arg = (unsigned long) &channels;
  if ((ret = l4dde_snd_ioctl (audio_snddev, SNDCTL_DSP_CHANNELS, arg)))
  {
    LOG_Error ("audio.c: ioctl CHANNELS (%d)", ret);
    return 1;
  }
  arg = (unsigned long) &rate;
  if ((ret = l4dde_snd_ioctl (audio_snddev, SNDCTL_DSP_SPEED, arg)))
  {
    LOG_Error ("audio.c: ioctl SPEED (%d)", ret);
    return 1;
  }
  return 0;
}


/*
  init dde-stuff
*/
static void
audio_init_dde (void)
{
  int err;
  l4io_info_t *io_info_addr = NULL;

  /* request io info page for "jiffies" and "HZ" */
  if (l4io_init (&io_info_addr, L4IO_DRV_INVALID))
    return;

  ASSERT (io_info_addr);

  /* initialize all DDE modules required ... */
  if ((err = l4dde_mm_init (VSYNC_VMEM_SIZE, VSYNC_KMEM_SIZE)))
  {
    LOG_Error ("initializing mm (%d)", err);
    return;
  }
  if ((err = l4dde_process_init ()))
  {
    LOG_Error ("initializing process-level (%d)", err);
    return;
  }
  if ((err = l4dde_irq_init (io_info_addr->omega0)))
  {
    LOG_Error ("initializing irqs (%d)", err);
    return;
  }
  if ((err = l4dde_time_init ()))
  {
    LOG_Error ("initializing time (%d)", err);
    return;
  }
  if ((err = l4dde_softirq_init ()))
  {
    LOG_Error ("initializing softirqs (%d)", err);
    return;
  }
  if ((err = l4dde_pci_init ()))
  {
    LOG_Error ("initializing pci (%d)", err);
    return;
  }
  /* startup */
  LOGdL (DEBUG_EXPORT, "starting up sound...");

  if ((err = l4dde_snd_init ()))
  {
    LOG_Error ("initializing sound (%d)", err);
    return;
  }

  return;
}
