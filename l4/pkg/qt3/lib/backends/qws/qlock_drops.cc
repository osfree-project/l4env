/*** L4-SPECIFIC INCLUDES ***/
#include <l4/log/l4log.h>

/*** GENERAL INCLUDES ***/
#include <private/qlock_p.h>

/*** LOCAL INCLUDES ***/
#include <l4/qt3/l4qws_qlock_client.h>

/* **************************************************************** */

#ifdef DEBUG
# define _DEBUG 1
#else
# define _DEBUG 0
#endif

/* **************************************************************** */

class QLockData
{
public:
  l4qws_key_t     key;
  l4qws_qlockid_t id;
  int  count;
  bool owned;
};

/* **************************************************************** */

QLock::QLock(const QString &filename, char c, bool create )
{
  // FIXME: Currently only one QLock is needed, which does not need to be created as it
  //        is hardcoded in QWS-Server (l4qws_qlock_server.c). *_create()/*_get() return
  //        id = 0.

  data        = new QLockData;
  data->key   = l4qws_key(filename.latin1(), c);
  data->count = 0;
  data->owned = create;
  
  // FIXME: error handling
  if (create) {    
    l4qws_qlock_create_call(&l4qws_qlock_server, data->key, &data->id, &l4qws_dice_qlock_env);
  } else {
    l4qws_qlock_get_call(&l4qws_qlock_server, data->key, &data->id, &l4qws_dice_qlock_env);
  }
}



QLock::~QLock()
{
  if ( locked() )
    unlock();

  if(data->owned) {
    l4qws_qlock_destroy_call(&l4qws_qlock_server, data->id, &l4qws_dice_qlock_env);
  }
  delete data;
}



bool QLock::isValid() const
{
  return (data->key != L4QWS_INVALID_KEY);
}


void QLock::lock( Type t )
{
  if (data->count == 0) {
    type = t;
    if (type == Read)
      l4qws_qlock_lock_read_call(&l4qws_qlock_server, data->id,
                                 &l4qws_dice_qlock_env);
    else
      l4qws_qlock_lock_write_call(&l4qws_qlock_server, data->id,
                                  &l4qws_dice_qlock_env);
  }
  data->count++;
}


void QLock::unlock()
{
  if (data->count > 0) {
    data->count--;

    if (data->count == 0) {

      if (type == Read)
        l4qws_qlock_unlock_read_call(&l4qws_qlock_server, data->id,
                                     &l4qws_dice_qlock_env);
      else
        l4qws_qlock_unlock_write_call(&l4qws_qlock_server, data->id,
                                      &l4qws_dice_qlock_env);        
    }

  } else {
    LOG("Unlock without corresponding lock\n");
  }
}



bool QLock::locked() const
{
  return (data->count > 0);
}



