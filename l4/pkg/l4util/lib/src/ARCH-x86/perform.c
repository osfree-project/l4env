/* $Id$ */
/**
 * \brief   lib for performance mesaurement counters. 
 */

#include <l4/sys/types.h>

#define CONFIG_PERFORM_ONLY_PROTOTYPES
#include <l4/util/perform.h>

typedef struct
{
  l4_uint32_t index;
  const char *string;
} event_entry;

static const event_entry event_array[]=
{
#include "pmc_events.h"
};

const char*
strp6pmc_event(l4_uint32_t event)
{
  int i;

  for (i=0; i<sizeof(event_array)/sizeof(event_entry); i++)
    if (event_array[i].index == event)
      return event_array[i].string;

  return "unkown";
}
