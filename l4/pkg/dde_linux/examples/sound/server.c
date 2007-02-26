/* $Id$ */
/*****************************************************************************/
/**
 * \file	dde_linux/examples/sound/server.c
 *
 * \brief	Sound Server
 *
 * \author	Christian Helmuth <ch12@os.inf.tu-dresden.de>
 *
 * Insert configuration parameters into my_cfg.h before compilation.
 */
/*****************************************************************************/

/* L4 */
#include <l4/sys/types.h>
#include <l4/env/init.h>
#include <l4/util/macros.h>
#include <l4/generic_io/libio.h>
#include <l4/dde_linux/dde.h>
#include <l4/dde_linux/sound.h>

/* Linux */
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/soundcard.h>

/* OSKit */
#include <l4/oskit10/grub_mb_info.h>

/* local */
#include "my_cfg.h"

extern struct grub_multiboot_info *_mbi;

l4_addr_t sample[4][2];
int sample_no = 0;

static int afmt = AFMT_S16_LE;
static int channels = 2;
static int rate = 44100;

static int snddev;

static int init_mods(void)
{
  int err;

  Msg("%ld module(s) (only 4 or less usable):\n", _mbi->mods_count);

  for (err = 0; err < _mbi->mods_count; err++)
    {
      struct grub_mod_list *mod =
	(struct grub_mod_list *) (_mbi->mods_addr +
				  (err * sizeof(struct grub_mod_list)));

      Msg("  [%d] @ %p\n", err, (void *) mod->mod_start);

      sample[err][0] = mod->mod_start;
      sample[err][1] = mod->mod_end;
      sample_no++;

      if (err == 3)
	break;
    }

  return 0;
}

static int cfg_mixer(int choice)
{
  int ret, mixer;
  int vol;
  
  /* 8 bits per channel:
     31:16 undefined  15:8 right  7:0 left */
  switch (choice)
    {
    case 1:
      vol = 0xf0f0;
      break;
    case 2: /* only left channel */
      vol = 0x00f0;
      break;
    case 3: /* only right channel */
      vol = 0xf000;
      break;
    default: /* mute */
      vol = 0;
    }

  if ((mixer = l4dde_snd_open_mixer(0)) < 0)
    {
      Error("opening MIXER (%d)", mixer);
      return 1;
    }
  Msg("MIXER opened\n");

  if ((ret = l4dde_snd_ioctl(mixer, SOUND_MIXER_WRITE_PCM, (l4_addr_t)&vol)))
    {
      Error("ioctl VOLUME (%d)", ret);
      return 1;
    }
  Msg("volume set to %x\n", vol);

  l4dde_snd_close(mixer);
  
  return 0;
}

static int cfg_card(void)
{
  int ret;
  unsigned long arg;

  if ((snddev = l4dde_snd_open_dsp(1)) < 0)
    {
      Error("opening DSP (%d)", snddev);
      return 1;
    }
  Msg("opened\n");

  arg = (unsigned long) &afmt;
  if ((ret = l4dde_snd_ioctl(snddev, SNDCTL_DSP_SETFMT, arg)))
    {
      Error("ioctl SETFMT (%d)", ret);
      return 1;
    }
  Msg("fmt set\n");

  arg = (unsigned long) &channels;
  if ((ret = l4dde_snd_ioctl(snddev, SNDCTL_DSP_CHANNELS, arg)))
    {
      Error("ioctl CHANNELS (%d)", ret);
      return 1;
    }
  Msg("chn set\n");
  arg = (unsigned long) &rate;
  if ((ret = l4dde_snd_ioctl(snddev, SNDCTL_DSP_SPEED, arg)))
    {
      Error("ioctl SPEED (%d)", ret);
      return 1;
    }
  Msg("speed set\n");

  Msg("AFMT = 0x%x CHANNELS = %d RATE = %d\n", afmt, channels, rate);

  return 0;
}

static int close_card(void)
{
  int ret;

  if ((ret = l4dde_snd_close(snddev)))
    Error("releasing DSP (%d)", ret);

  return ret;
}

static int play_sound(void)
{
  int ret;
  int fsize = sample[0][1] - sample[0][0];
  int fpos = 0;
#define FRAME 65536
  int count = FRAME;

  Msg("sample 0 is %d bytes\n", fsize);

  for (;;)
    {
      /* break before playing last short frame */
      if (fpos + FRAME > fsize)
	break;
      count = FRAME;

      ret = l4dde_snd_write(snddev, (void *) sample[0][0] + fpos, count);
      if (ret < 0)
	{
	  Error("on write (%d)", ret);
	  return 1;
	}
      if (ret < FRAME)
	Msg("frame truncated\n");

      fpos += ret;
    }
  return 0;
}

/** sound demo */
void demo(void)
{
  int count = 3;

  if (init_mods())
    return;
  if (!sample_no)
    return;

  Msg("Playing sample %d times maximum...\n", count);

  for (; count; count--)
    {
      Assert(!cfg_card());
      Assert(!cfg_mixer(count));
      Assert(!play_sound());
      Assert(!close_card());
      if (count-1)
	enter_kdebug("Play again? Hit g");
    }

  Msg("released, goodbye...\n");
}

/** main */
int main(void)
{
  int err;
  l4io_info_t *io_info_addr = NULL;

  /* request io info page for "jiffies" and "HZ" */
  if (l4io_init(&io_info_addr, L4IO_DRV_INVALID))
    exit(-1);

  ASSERT(io_info_addr);
  
  /* initialize all DDE modules required ... */
  if ((err=l4dde_mm_init(VMEM_SIZE, KMEM_SIZE)))
    {
      Error("initializing mm (%d)", err);
      exit(-1);
    }
  if ((err=l4dde_irq_init(io_info_addr->omega0)))
    {
      Error("initializing irqs (%d)", err);
      exit(-1);
    }
  if ((err=l4dde_time_init()))
    {
      Error("initializing time (%d)", err);
      exit(-1);
    }
  if ((err=l4dde_softirq_init()))
    {
      Error("initializing softirqs (%d)", err);
      exit(-1);
    }
  if ((err=l4dde_pci_init()))
    {
      Error("initializing pci (%d)", err);
      exit(-1);
    }
  if ((err=l4dde_process_init()))
    {
      Error("initializing process-level (%d)", err);
      exit(-1);
    }

  /* startup */
  DMSG("starting up sound...\n");

  if ((err=l4dde_snd_init()))
    {
      Error("initializing sound (%d)", err);
      exit(-1);
    }
  l4env_do_initcalls();
  DMSG("sound initialized\n");

  demo();

  /* shutdown */
  /* XXX Currently, l4env does not provide exitcalls. If we need them,
         we have to implement them. */
  if ((err=l4dde_snd_exit()))
    {
      Error("finalizing sound (%d)", err);
      exit(-1);
    }
  DMSG("sound shut down.\n");

  exit(0);
}
