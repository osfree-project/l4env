/*** GENERAL INCLUDES ***/
#include <stdio.h>

/*** L4-SPECIFIC INCLUDES ***/
#include <l4/log/l4log.h>
#include <l4/names/libnames.h>
#include <l4/thread/thread.h>
#include <l4/dm_mem/dm_mem.h>

#include <l4/qt3/l4qws_shm_server.h>

#ifdef DEBUG
# define _DEBUG 1
#else
# define _DEBUG 0
#endif

#define MAX_KEY_DS_ENTRIES 256

/*
 * ************************************************************************
 */

typedef struct l4qws_key_ds {
  l4qws_key_t      key;
  l4dm_dataspace_t ds;  
  int              ref_count;
} l4qws_key_ds_t;

/*
 * ************************************************************************
 */

l4qws_key_ds_t __key_ds[MAX_KEY_DS_ENTRIES];

/*
 * ************************************************************************
 */

static void setup_server_thread(void *d);
static void init_key_ds_map(void);
static int register_ds(l4qws_key_t key, l4dm_dataspace_t *ds);
static void unregister_ds(l4qws_key_ds_t *slot);
l4qws_key_ds_t *ref_count_add(l4qws_key_ds_t *slot, int num);
static l4qws_key_ds_t *find_ds(l4qws_key_t key);

/*
 * ************************************************************************
 */

CORBA_int l4qws_shm_create_component(CORBA_Object _dice_corba_obj,
                                     l4qws_key_t shm_key,
                                     CORBA_int size,
                                     l4dm_dataspace_t *ds,
                                     CORBA_Server_Environment *_dice_corba_env)
{
  if (find_ds(shm_key) != NULL) {
    LOGd(_DEBUG, "Key 0x%x is already in use\n", shm_key);
    return -1;
  }
    
  if (l4dm_mem_open(L4DM_DEFAULT_DSM, size, 0, 0, NULL, ds) == 0 &&
      l4dm_share(ds, *_dice_corba_obj, L4DM_RW) == 0) {
    if (register_ds(shm_key, ds) == 0)
      return 0;

    l4dm_close(ds); // could not register
  }

  LOG_Error("Could not create dataspace\n");
  return -1;
}



CORBA_int l4qws_shm_get_component(CORBA_Object _dice_corba_obj,
                                  l4qws_key_t shm_key,
                                  l4dm_dataspace_t *ds,
                                  CORBA_Server_Environment *_dice_corba_env)
{
  l4qws_key_ds_t *slot = find_ds(shm_key);

  if (slot && l4dm_share(&slot->ds, *_dice_corba_obj, L4DM_RW) == 0) {
    *ds = slot->ds;
    ref_count_add(slot, 1);
    return 0;
  }

  LOG_Error("Could not get rights to attach to dataspace\n");
  return -1;
}



CORBA_int l4qws_shm_destroy_component(CORBA_Object _dice_corba_obj,
                                      l4qws_key_t shm_key,
                                      CORBA_Server_Environment *_dice_corba_env)
{
  l4qws_key_ds_t *slot = find_ds(shm_key);

  if (slot) {
    ref_count_add(slot, -1);
    if (slot->ref_count > 0)
      return 0;
    else if (l4dm_close(&slot->ds) == 0) {
      unregister_ds(slot);
      return 0;
    }
  }

  return -1;
}

/*
 * ************************************************************************
 */

void l4qws_shm_server_init(void) {

  l4thread_t th;

  th = l4thread_create(setup_server_thread, NULL, L4THREAD_CREATE_SYNC);

  if (th <= 0)
    LOG_Error("Failed to create server thread\n");
}



void setup_server_thread(void *d) {

  names_register("qws-shm");
  init_key_ds_map();

  l4thread_started(NULL);

  l4qws_shm_server_loop(NULL);
}

/*
 * ************************************************************************
 */

void init_key_ds_map(void) {

  int i;
  l4qws_key_ds_t *slot = &__key_ds[0];

  for (i = 0; i < MAX_KEY_DS_ENTRIES; i++, slot++) {
    slot->key = L4QWS_INVALID_KEY;
    slot->ds  = L4DM_INVALID_DATASPACE;
  }
}



int register_ds(l4qws_key_t key, l4dm_dataspace_t *ds) {

  int i;
  l4qws_key_ds_t *slot = &__key_ds[0];
  l4qws_key_ds_t *free_slot = NULL;

  i = 0;
  while (free_slot == NULL && i < MAX_KEY_DS_ENTRIES) {

    if (slot->key == L4QWS_INVALID_KEY)
      free_slot = slot;

    i++;
    slot++;
  }

  if (free_slot) {
    free_slot->key = key;
    free_slot->ds  = *ds;
    free_slot->ref_count = 1;
    return 0;
  }

  return -1;
}



void unregister_ds(l4qws_key_ds_t *slot) {

  slot->key = L4QWS_INVALID_KEY;
  slot->ds  = L4DM_INVALID_DATASPACE;
}



l4qws_key_ds_t *ref_count_add(l4qws_key_ds_t *slot, int num) {

  if (slot) {
    slot->ref_count += num;
    if (slot->ref_count < 0)
      LOG_Error("Reference counter for dataspace dropped below 0\n");
  }

  return slot;
}

 

l4qws_key_ds_t *find_ds(l4qws_key_t key) {

  int i = 0;
  l4qws_key_ds_t *slot = &__key_ds[0];

  while (i < MAX_KEY_DS_ENTRIES && slot->key != key) {
    i++;
    slot++;
  }

  return (i < MAX_KEY_DS_ENTRIES) ? slot : NULL;
}

