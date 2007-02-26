#include "test-server.h"

inline
int error_handling(CORBA_Object *sender, base_msg_buffer_t *buffer, CORBA_Environment *env)
{
  // we could test where that message comes from using 'sender'
  //
  // we can unamrshal data from the message buffer
  // using the macros from dice.h
  // e.g. 
  // - UNMARSHAL_DWORD, UNMARSHAL_STRING
  // - MARSHAL_DWORD, MARSHAL_STRING
  // - GET_DWORD_COUNT, GET_STRING_COUNT
  // - SET_DWORD_COUNT, SET_STRING_COUNT
  GET_DWORD(buffer, 0)=0;
  GET_DWORD(buffer, 1)=0;
  SET_DWORD_COUNT(buffer, 2);
  SET_STRING_COUNT(buffer, 0);
  // and finally we can return a status specifying if we want to reply or not
  return REPLY; // or NO_REPLY
}
