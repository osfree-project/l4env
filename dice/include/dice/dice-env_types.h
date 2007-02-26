#ifndef __DICE_ENV_TYPES_H__
#define __DICE_ENV_TYPES_H__

#ifndef CORBA_exception_type_typedef
#define CORBA_exception_type_typedef
typedef int CORBA_exception_type;
#endif

#define COMMON_ENVIRONMENT \
  CORBA_exception_type major:4; \
  CORBA_exception_type repos_id:28; \
  void *param

#endif // __DICE_ENV_TYPES_H__      
