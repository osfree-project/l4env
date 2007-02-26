/* $Id$ */
/*****************************************************************************/
/**
 * \file   lib/backends/qws/l4qws_qlock_server.c
 * \brief  L4-specific QLock implementation.
 *
 * \date   11/02/2004
 * \author Carsten Weinhold <weinhold@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2004-2006 Technische Universitaet Dresden
 * This file is part of the Qt3 port for L4/DROPS, which is distributed under
 * the terms of the GNU General Public License 2. Please see the COPYING file
 * for details.
 */

/*** GENERAL INCLUDES ***/
#include <stdio.h>
#include <stdlib.h>

/*** L4-SPECIFIC INCLUDES ***/
#include <l4/log/l4log.h>
#include <l4/names/libnames.h>
#include <l4/thread/thread.h>

#include <l4/qt3/l4qws_qlock_server.h>


#ifdef DEBUG
# define _DEBUG 1
# define _DEBUG_LOG 1
#else
# define _DEBUG 0
# define _DEBUG_LOG 0
#endif

#if _DEBUG_LOG
# define LOG_STATE(l) print_state(__LINE__, l);
# undef _DEBUG
# define _DEBUG 1
#else
# define LOG_STATE(l)
#endif

#if _DEBUG
# define CHECK_CONSISTENCY(l) check_consistency(__LINE__, l);
#else
# define CHECK_CONSISTENCY(l)
#endif

#define MAX_LOCKS 1

/*
 * ************************************************************************
 */

typedef struct queue_node {
  l4_threadid_t     client;
  struct queue_node *next;
  int               shared:1;  // client requested Read lock if this is 1
} queue_node_t;

typedef struct l4qws_qlock {
  queue_node_t *first;         // - queue head
  queue_node_t *last;          // - queue end
  unsigned int locked     :1;  // - held by a client if this is 1
  unsigned int shareable  :1;  // - lock(Read) won't block if this is 1
  unsigned int held_shared:1;  // - current lock mode
  unsigned int num_shared :15; // - number of clients that currently hold
                               //   the lock in shared mode (Read) (excl. blocked)
  unsigned int num_total_clients;
} l4qws_qlock_t;

typedef struct l4qws_key_qlock {
  l4qws_key_t   key;
  l4qws_qlock_t lock;
} l4qws_key_qlock_t;

/*
 * ************************************************************************
 */

l4qws_key_qlock_t __key_lock[MAX_LOCKS];

/*
 * ************************************************************************
 */

static CORBA_int lock_internal(CORBA_Object client, l4qws_qlockid_t id,
                               CORBA_int shared_lock, CORBA_short *_dice_reply);
static CORBA_int unlock_internal(CORBA_Object client, l4qws_qlockid_t id,
                                 CORBA_int shared_lock);

static void setup_server_thread(void *d);
static void init_key_queue_map(void);
#if _DEBUG
static void print_state(int line, l4qws_qlock_t *l);
static void check_consistency(int line, l4qws_qlock_t *l);
#endif

/*
 * ************************************************************************
 */

CORBA_int
l4qws_qlock_create_component(CORBA_Object _dice_corba_obj,
                             l4qws_key_t lock_key,
                             l4qws_qlockid_t *id,
                             CORBA_Server_Environment *_dice_corba_env)
{
  // FIXME: implement support for multiple locks
  *id = 0;
  return 0;
}



CORBA_int
l4qws_qlock_get_component(CORBA_Object _dice_corba_obj,
                          l4qws_key_t lock_key,
                          l4qws_qlockid_t *id,
                          CORBA_Server_Environment *_dice_corba_env)
{
  // FIXME: implement support for multiple locks
  *id = 0;
  return 0;
}



CORBA_int
l4qws_qlock_destroy_component(CORBA_Object _dice_corba_obj,
                              l4qws_qlockid_t id,
                              CORBA_Server_Environment *_dice_corba_env)
{
  // FIXME: implement support for multiple locks
  return 0;
}


CORBA_int
l4qws_qlock_lock_read_component(CORBA_Object _dice_corba_obj,
                                l4qws_qlockid_t id,
                                CORBA_short *_dice_reply,
                                CORBA_Server_Environment *_dice_corba_env)
{
  return lock_internal(_dice_corba_obj, id, 1, _dice_reply);
}



CORBA_int
l4qws_qlock_lock_write_component(CORBA_Object _dice_corba_obj,
                                 l4qws_qlockid_t id,
                                 CORBA_short *_dice_reply,
                                 CORBA_Server_Environment *_dice_corba_env)
{
  return lock_internal(_dice_corba_obj, id, 0, _dice_reply);
}



CORBA_int
l4qws_qlock_unlock_read_component(CORBA_Object _dice_corba_obj,
                                  l4qws_qlockid_t id,
                                  CORBA_Server_Environment *_dice_corba_env)
{
  return unlock_internal(_dice_corba_obj, id, 1);
}



CORBA_int
l4qws_qlock_unlock_write_component(CORBA_Object _dice_corba_obj,
                                   l4qws_qlockid_t id,
                                   CORBA_Server_Environment *_dice_corba_env)
{
  return unlock_internal(_dice_corba_obj, id, 0);
}

/*
 * ************************************************************************
 */

static CORBA_int
lock_internal(CORBA_Object client,
              l4qws_qlockid_t id,
              CORBA_int shared_lock,
              CORBA_short *_dice_reply)
{
  // FIXME: implement support for multiple locks
  l4qws_qlock_t *l;

  if (id != 0)
    return -1;

  l = &__key_lock[id].lock;

  l->num_total_clients++;
  LOGd(_DEBUG_LOG, "Locking %s for %d.%d", shared_lock ? "READ" : "WRITE",
       client->id.task, client->id.lthread);

  if ( !l->locked) {

    // no one holds the lock (neither exclusive nor shared)

    l->locked = 1;
    if (shared_lock) {
      l->shareable   = 1;
      l->held_shared = 1;
      l->num_shared  = 1;
    } else {
      l->shareable   = 0;
      l->held_shared = 0;
      l->num_shared  = 0;
    }

    *_dice_reply = DICE_REPLY;
    return 0;

  }

  CHECK_CONSISTENCY(l);

  if (l->shareable && shared_lock) {

    // just another "shared-lock()"

    l->num_shared++;
    *_dice_reply = DICE_REPLY;
    return 0;
  }

  // ok, we need to enqueue ...

  // Hmmm ... malloc() here? No better idea?
  queue_node_t *n = (queue_node_t *) malloc(sizeof(queue_node_t));
  if (n == NULL) {
    LOG_Error("malloc() failed.\n");
    return -1;
  }

  // enqueue client
  LOGd(_DEBUG_LOG, "Enqueing %d ...", client->id.task);
  LOG_STATE(l);
  if (l->last) {
    l->last->next = n;
    l->last       = n;
  } else
    l->first = l->last = n;
  n->next   = NULL;
  n->client = *client;
        
  if (shared_lock) {
    n->shared    = 1;
  } else {
    n->shared    = 0;
    l->shareable = 0;
  }

  *_dice_reply = DICE_NO_REPLY;

  LOG_STATE(l);
  CHECK_CONSISTENCY(l);
  return 0;
}



static CORBA_int
unlock_internal(CORBA_Object client, l4qws_qlockid_t id, CORBA_int shared_lock)
{
  // FIXME: implement support for multiple locks

  CORBA_Server_Environment env   = dice_default_server_environment;
  CORBA_int      _dice_return    = 0;
  int            client_dequeued = 0;
  queue_node_t  *c;
  l4qws_qlock_t *l;

  if (id != 0)
    return -1;

  LOGd(_DEBUG_LOG, "Unlocking %s for %d.%d", shared_lock ? "READ" : "WRITE",
       client->id.task, client->id.lthread);

  l = &__key_lock[id].lock;
  l->num_total_clients--;
  if ( !l->locked) {
    LOG_Error("unlock() without corresponding lock()!\n");
    return -1;
  }

  CHECK_CONSISTENCY(l);
  LOG_STATE(l);

  if ((l->held_shared == 0) != (shared_lock == 0)) {
    LOG("Error during unlock(): l->held_shared=%d != shared_lock=%d", l->held_shared, shared_lock);
    return -1;
  }

  if (l->held_shared && shared_lock) {
    
    // shared unlock

    if (l->num_shared > 0)
      l->num_shared--;
    else {
      LOG("no more shared clients!? Something went wrong!");
      return -1;
    }

    LOG_STATE(l);
    
    if (l->num_shared > 0) {
      // there are still other clients holding the lock in shared
      // mode, so we can't wake any clients, that might be queued
      return 0;
    }
  }

  if (l->first) {
    /* Ok, someone is waiting in the queue. This can either be a writer
     * (and possibly other readers and writers) if the current unlock()
     * was done by a reader. If the current unlock was done by a writer
     * there may also be one or more readers at the head of the queue.
     */

    LOGd(_DEBUG_LOG, "Dequeing %d ...", l->first->client.id.task);
    c        = l->first;
    l->first = c->next;
    if (l->first == NULL)
      l->last = NULL;
  
    LOG_STATE(l);
    if (c->shared) {
      l->held_shared = 1;
      l->num_shared++;
    } else {
      l->held_shared = 0;
      l->shareable   = 0;
    }
    
    // wake up c->client (the one just taken from the queue's head)
    LOGd(_DEBUG_LOG, "Waking %d ...", c->client.id.task);
    if (c->shared)
      l4qws_qlock_lock_read_reply(&c->client, _dice_return, &env);
    else
      l4qws_qlock_lock_write_reply(&c->client, _dice_return, &env);
    free(c);
    
    if (l->held_shared) {
      // let's see, if we can wake up other readers
      LOGd(_DEBUG_LOG, "Waking Readers in queue");
      while (l->first && l->first->shared) {
        c        = l->first;
        l->first = c->next;
        if (l->first == NULL)
          l->last = NULL;
      
        l->num_shared++;
        LOGd(_DEBUG_LOG, "Waking %d ...", c->client.id.task);
        LOG_STATE(l);
        l4qws_qlock_lock_read_reply(&c->client, _dice_return, &env);
        free(c);
      }
    }

    // indicate, that we dequeued at least one client, which now holds the lock
    client_dequeued = 1;
  }

  if (l->first == NULL) {

    if (l->num_shared == 0 && !client_dequeued) {
      LOGd(_DEBUG_LOG, "l->locked = 0; l->held_shared = 0");
      l->locked      = 0;

    } else if (l->held_shared) {
      l->shareable = 1;
    }
  }

  CHECK_CONSISTENCY(l);
  return 0;
}


/*
 * ************************************************************************
 */

void l4qws_qlock_server_init(void) {

  l4thread_t th;

  th = l4thread_create(setup_server_thread, NULL, L4THREAD_CREATE_SYNC);

  if (th <= 0)
    LOG_Error("Failed to create server thread\n");
}



static void setup_server_thread(void *d) {

  names_register("qws-qlock");
  init_key_queue_map();

  l4thread_started(NULL);

  l4qws_qlock_server_loop(NULL);
}

/*
 * ************************************************************************
 */

static void init_key_queue_map(void) {

  int i;
  l4qws_key_qlock_t *slot = &__key_lock[0];

  for (i = 0; i < MAX_LOCKS; i++, slot++) {
    slot->key             = L4QWS_INVALID_KEY;
    slot->lock.first      = NULL;
    slot->lock.last       = NULL;
    slot->lock.locked     = 0;
    slot->lock.shareable  = 1;
    slot->lock.num_shared = 0;
    slot->lock.num_total_clients = 0;
  }
}


/*
 * ************************************************************************
 */

#if _DEBUG
static void print_state(int line, l4qws_qlock_t *l) {

  unsigned int locked      = l->locked;
  unsigned int held_shared = l->held_shared;
  unsigned int shareable   = l->shareable;
  unsigned int num_shared  = l->num_shared;
  queue_node_t *n =  l->first; 

  LOG("%d: locked=%d, held_shared=%d, shareable=%d, num_shared=%d, num_total_clients=%d",
      line, locked, held_shared, shareable, num_shared, l->num_total_clients);
  while (n) {
    LOG("%d(shrd=%d) ", n->client.id.task, n->shared);
    n = n->next;
  }
}
#endif


#if _DEBUG
static void check_consistency(int line, l4qws_qlock_t *l) {

  int e = 0;

  if ( !l->locked && l->num_total_clients != 0) {
    LOG("!l->locked && l->num_total_clients != 0");
    print_state(line, l);
    e++;
  }

  if ( !l->locked)
    return;

  if (l->locked == 0 && l->num_shared != 0) {
    LOG("l->locked == 0 && l->num_shared != 0");
    e++;
  }

  if (l->held_shared && l->num_shared == 0) {
    LOG("l->held_shared && l->num_shared == 0");
    e++;
  }

  if (!l->held_shared && l->shareable) {
    LOG("!l->held_shared && l->shareable");
    e++;
  }

  if (e > 0)
    *((int *) 0) = 0; /* crash here, please */
}
#endif


