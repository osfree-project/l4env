/* $Id$ */
/*****************************************************************************/
/**
 * \file	dde_linux/lib/sound/soundcore.c
 *
 * \brief	Linux DDE Soundcore
 *
 * \author	Christian Helmuth <ch12@os.inf.tu-dresden.de>
 */
/*****************************************************************************/
/** \ingroup mod_envs
 * \defgroup mod_sound Linux DDE Soundcore
 *
 * Linux soundcore emulation.
 *
 */
/*****************************************************************************/

/* L4 */
#include <l4/env/errno.h>
#include <l4/lock/lock.h>

#include <l4/dde_linux/dde.h>

/* Linux */
#include <linux/vmalloc.h>
#include <linux/fs.h>

/* local */
#include "__config.h"
#include "__macros.h"
#include "internal.h"

#include "soundcore.h"

/** internal sound device/unit structure */
struct sound_unit
{
  int type;			/**< type of device */
  int num;			/**< device number */
  struct file_operations *fops;	/**< file operations */
  struct sound_unit *next;	/**< next in list */
};

/** internal lists of sound units */
static struct sound_unit *dsp = NULL, *mixer = NULL;
/** initilization lock */
static l4lock_t sound_loader_lock = L4LOCK_UNLOCKED;

/** Low level list operator.
 *
 * Scan the ordered list, find a hole and join into it. Called with the lock
 * asserted.
 *
 * \krishna It differs from Linux as we use \e n as \e device number (not as \e
 * minor number).
 */
static int __sound_insert_unit(struct sound_unit * s, struct sound_unit **list,
			       struct file_operations *fops)
{
  int n=0;

  /* first free */
  while (*list && (*list)->num < n)
    list=&((*list)->next);

  while(n < NUM_MAX)
    {
      /* Found a hole ? */
      if(*list == NULL || (*list)->num > n)
	break;
      list = &((*list)->next);
      n++;
    }

  if(n >= NUM_MAX)
    return -ENOENT;
		
  /* fill it in */
  s->num = n;
  s->fops = fops;
	
  /* link it */
  s->next = *list;
  *list = s;
	
  return n;
}

/** Remove a node from the chain.
 * Called with the lock asserted
 */
static void __sound_remove_unit(struct sound_unit **list, int unit)
{
  while(*list)
    {
      struct sound_unit *p=*list;
      if (p->num == unit)
	{
	  *list = p->next;
	  vfree(p);
	  return;
	}
      list = &(p->next);
    }
  Error("Sound device %d went missing!\n", unit);
}

/** look */
static struct sound_unit *__look_for_unit(struct sound_unit *s, int unit)
{
  while(s && s->num <= unit)
    {
      if (s->num == unit)
	return s;
      s = s->next;
    }
  return NULL;
}

/** Insert a unit.
 * Allocate the controlling structure and add it to the sound driver
 * list. Acquires locks as needed
 */
static int sound_insert_unit(struct sound_unit **list,
			     struct file_operations *fops, const char *name,
			     int type)
{
  int r;
  struct sound_unit *s=(struct sound_unit *)vmalloc(sizeof(struct sound_unit));

  if(s==NULL)
    return -ENOMEM;
		
  spin_lock(&sound_loader_lock);
  r = __sound_insert_unit(s, list, fops);
  s->type = type;
  spin_unlock(&sound_loader_lock);

  if (r < 0)
    vfree(s);

  return r;
}

/** Remove a unit.
 * Acquires locks as needed. The drivers MUST have completed the removal before
 * their file operations become invalid.
 */
static void sound_remove_unit(struct sound_unit **list, int unit)
{
  struct sound_unit *s;

  if (!(s = __look_for_unit(*list, unit)))
    return;

  spin_lock(&sound_loader_lock);
  __sound_remove_unit(list, unit);
  spin_unlock(&sound_loader_lock);
}

/** Register a DSP Device
 * \ingroup mod_sound
 *
 * \param  fops		interface
 * \param  dev		unit number
 *
 * \return allocated number or negative error value
 *
 * This function allocates both the audio and dsp device entries together and
 * will always allocate them as a matching pair - eg dsp3/audio3 (Linux)
 */
int register_sound_dsp(struct file_operations *fops, int dev)
{
  if (dev != -1)
    {
#if DEBUG_SOUND
      DMSG("dev has to be -1 (dev=%d)\n", dev);
#endif
      return -EINVAL;
    }

  dev = sound_insert_unit(&dsp, fops, "dsp", TYPE_DSP);

#if DEBUG_SOUND
  DMSG("register dsp node %d\n", dev);
#endif

  return dev;
}

/** Register a Mixer Device
 * \ingroup mod_sound
 *
 * \param  fops		interface
 * \param  dev		unit number
 *
 * \return allocated number or negative error value
 *
 * Unit number has to be -1 for <em>find first free</em> or we panic.
 */
int register_sound_mixer(struct file_operations *fops, int dev)
{
  if (dev != -1)
    {
#if DEBUG_SOUND
      DMSG("dev has to be -1 (dev=%d)\n", dev);
#endif
      return -EINVAL;
    }

  dev = sound_insert_unit(&mixer, fops, "mixer", TYPE_MIXER);

#if DEBUG_SOUND
  DMSG("register mixer node %d\n", dev);
#endif

  return dev;
}

/** Register a MIDI Device
 * \ingroup mod_sound
 *
 * \param  fops		interface
 * \param  dev		unit number
 *
 * \return allocated number or negative error value
 */
int register_sound_midi(struct file_operations *fops, int dev)
{
  if (dev != -1)
    {
#if DEBUG_SOUND
      DMSG("dev has to be -1 (dev=%d)\n", dev);
#endif
      return -EINVAL;
    }
#if 0 /* if this is needed we also have to write the API */
  dev = sound_insert_unit(&midi, fops, "midi", TYPE_MIDI);

#if DEBUG_SOUND
  DMSG("register midi node %d\n", dev);
#endif

  return dev;
#else
  return 0;
#endif /* 0 */
}

/** Register a Special Sound Node
 * \ingroup mod_sound
 *
 * \param  fops		interface
 * \param  unit		unit number
 *
 * \return allocated number or negative error value
 *
 * For now no special nodes are supported. Only OSS/Free (old ISA) and dmasound
 * use this feature.
 */
int register_sound_special(struct file_operations *fops, int unit)
{
  char *name;

  switch (unit)
    {
    case 0:
      name = "mixer";
      break;
    case 1:
      name = "sequencer";
      break;
    case 2:
      name = "midi00";
      break;
    case 3:
      name = "dsp";
      break;
    case 4:
      name = "audio";
      break;
    case 5:
      name = "unknown5";
      break;
    case 6:		/* Was once sndstat */
      name = "unknown6";
      break;
    case 7:
      name = "unknown7";
      break;
    case 8:
      name = "sequencer2";
      break;
    case 9:
      name = "dmmidi";
      break;
    case 10:
      name = "dmfm";
      break;
    case 11:
      name = "unknown11";
      break;
    case 12:
      name = "adsp";
      break;
    case 13:
      name = "amidi";
      break;
    case 14:
      name = "admmidi";
      break;
    default:
      name = "unknown";
      break;
    }
#if DEBUG_SOUND
  DMSG("register special node \"%s\" not supported\n", name);
#endif
  return -ENOENT;
}

/** Register a synth Device
 * \ingroup mod_sound
 *
 * \param  fops		interface
 * \param  dev		unit number
 *
 * \return allocated number or negative error value
 *
 * For now no raw synthesizer nodes are supported. Only wavefront.c uses this
 * feature.
 */
int register_sound_synth(struct file_operations *fops, int dev)
{
#if DEBUG_SOUND
  DMSG("register synth node not supported\n");
#endif
  return -ENOENT;
}

/** Unregister a DSP Device
 * \ingroup mod_sound
 *
 * \param  unit		unit number
 */
void unregister_sound_dsp(int unit)
{
  sound_remove_unit(&dsp, unit);

#if DEBUG_SOUND
  DMSG("unregister dsp node %d\n", unit);
#endif
}

/** Unregister a Mixer Device
 * \ingroup mod_sound
 *
 * \param  unit		unit number
 */
void unregister_sound_mixer(int unit)
{
  sound_remove_unit(&mixer, unit);

#if DEBUG_SOUND
  DMSG("unregister mixer node %d\n", unit);
#endif
}

/** Unregister a MIDI Device
 * \ingroup mod_sound
 *
 * \param  unit		unit number
 */
void unregister_sound_midi(int unit)
{
#if 0
  sound_remove_unit(&midi, unit);
#endif /* 0 */

#if DEBUG_SOUND
  DMSG("unregister midi node %d\n", unit);
#endif
}

/** Unregister a Special Node
 * \ingroup mod_sound
 *
 * \param  unit		unit number
 *
 * For now no special nodes are supported. Only OSS/Free (old ISA) and dmasound
 * use this feature.
 */
void unregister_sound_special(int unit)
{
#if DEBUG_SOUND
  DMSG("unregister special node %d not supported\n", unit);
#endif
}
 
/** Unregister a synth Device
 * \ingroup mod_sound
 *
 * \param  unit		unit number
 *
 * For now no raw synthesizer nodes are supported. Only wavefront.c uses this
 * feature.
 */
void unregister_sound_synth(int unit)
{
#if DEBUG_SOUND
  DMSG("unregister synth node %d not supported\n", unit);
#endif
}


/** Query for device operations structure
 * \ingroup mod_sound
 *
 * \param type		desired device type
 * \param num		desired device number
 *
 * \return fops pointer; NULL on error
 */
struct file_operations* soundcore_req_fops(int type, int num)
{
  struct sound_unit *s;

  switch (type)
    {
    case TYPE_DSP: /* dsp */
      s = __look_for_unit(dsp, num);
      break;
    case TYPE_MIXER: /* mixer */
      s = __look_for_unit(mixer, num);
      break;
    default:
      s = NULL;
    }
  return s ? s->fops : NULL;
}

/** Release DSP/PCM device
 * XXX nothing to do ? */
void soundcore_rel_fops(int type, int num)
{
}
