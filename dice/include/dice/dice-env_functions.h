#ifndef __DICE_CORBA_EXCEPTION_H__
#define __DICE_CORBA_EXCEPTION_H__

#define CORBA_NO_EXCEPTION      0
#define CORBA_USER_EXCEPTION    1
#define CORBA_SYSTEM_EXCEPTION  2

#define CORBA_DICE_EXCEPTION_NONE 0
#define CORBA_DICE_EXCEPTION_WRONG_OPCODE 1
#define CORBA_DICE_EXCEPTION_IPC_ERROR 2
#define CORBA_DICE_INTERNAL_IPC_ERROR 3
#define CORBA_DICE_EXCEPTION_COUNT 4

static CORBA_char* __CORBA_Exception_Repository[CORBA_DICE_EXCEPTION_COUNT+1] = { "none", "wrong opcode", "ipc error", "", 0 };

static inline
void CORBA_exception_free(CORBA_Environment *ev)
{
  if (ev->major != CORBA_NO_EXCEPTION)
    {
      //CORBA_free(ev->param);    
      ev->param = 0;
    }
  ev->major = CORBA_NO_EXCEPTION;
  ev->repos_id = CORBA_DICE_EXCEPTION_NONE;
}


static inline
void CORBA_exception_set(
    CORBA_Environment *ev,
    CORBA_exception_type major,
//    CORBA_char *except_repos_id,
    CORBA_exception_type repos_id,
    void *param)
{
  CORBA_exception_free(ev);
  ev->major = major;
  if (major != CORBA_NO_EXCEPTION)
    {
      ev->repos_id = repos_id;
      ev->param = param;
    }
}

static inline
CORBA_char* CORBA_exception_id(CORBA_Environment *ev)
{
  // string can be found using repository id (repos_id)
  if ((ev->major != CORBA_NO_EXCEPTION) && (ev->repos_id >= 0) &&
      (ev->repos_id < CORBA_DICE_EXCEPTION_COUNT))
    return __CORBA_Exception_Repository[ev->repos_id];
  else
    return 0;
}

static inline
void* CORBA_exception_value(CORBA_Environment *ev)
{
  return ev->param;
}

static inline
CORBA_any* CORBA_exception_as_any(CORBA_Environment *ev)
{
  // not supported
  return 0;
}
 
#endif

