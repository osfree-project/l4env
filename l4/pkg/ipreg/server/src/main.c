#include <stdint.h>
#include <string.h>

#include <l4/log/l4log.h>
#include <l4/names/libnames.h>

#include "ipreg-server.h"

/*
 * ******************************************************************
 */

#define MAX_ENTRIES 16

#define DBG 0

/*
 * ******************************************************************
 */

typedef struct ipreg_entry_s ipreg_entry_t;
struct ipreg_entry_s {

    char name[IPREG_MAX_NAME_LEN];
    char addr[IPREG_MAX_ADDR_LEN];
};

/*
 * ******************************************************************
 */

char LOG_tag[9] = "ipreg";

static ipreg_entry_t ipr[MAX_ENTRIES];

/*
 * ******************************************************************
 */

int
ipreg_register_component (CORBA_Object _dice_corba_obj,
                          const char* name,
                          const char* addr,
                          CORBA_Server_Environment *_dice_corba_env)
{
    int i;

    if (name == NULL || addr == NULL || name[0] == 0)
        return -1;

    LOGd(DBG, "registering '%s' for name '%s'", addr, name);

    for (i = 0; i < MAX_ENTRIES; i++) {

        if (strcmp(name, ipr[i].name) == 0) {
            /* name already registered */
            return -1;
        }

        if (ipr[i].name[0] == 0) {
            strncpy(ipr[i].name, name, IPREG_MAX_NAME_LEN - 1);
            strncpy(ipr[i].addr, addr, IPREG_MAX_ADDR_LEN - 1);
            ipr[i].name[IPREG_MAX_NAME_LEN - 1] = 0;
            ipr[i].addr[IPREG_MAX_ADDR_LEN - 1] = 0;
#if DBG
            {
                int j;
                for (j = 0; j < MAX_ENTRIES; j++) {
                    LOG("(%s,%s)", ipr[j].name, ipr[j].addr);
                }
            }
#endif
            return 0;
        }
    }

    return -1;
}


int
ipreg_query_component (CORBA_Object _dice_corba_obj,
                       const char* name,
                       char* *addr,
                       CORBA_Server_Environment *_dice_corba_env)
{
    int i;

    *addr = "";

    LOGd(DBG, "looking for '%s'", name);

    if (name == NULL || name[0] == 0)
        return -1;

    for (i = 0; i < MAX_ENTRIES; i++) {
        if (strcmp(name, ipr[i].name) == 0) {
            *addr = ipr[i].addr;
            LOGd(DBG, "found '%s'", ipr[i].addr);
            return 0;
        }
    }

    return -1;
}

/*
 * ******************************************************************
 */

int main(int argc, char **argv) {

    int i;

    for (i = 0; i < MAX_ENTRIES; i++) {
        ipr[i].name[0] = 0;
        ipr[i].addr[0] = 0;
    }

    if ( !names_register(IPREG_NAMES_NAME)) {
        LOG_Error("Failed to register '%s' at names", IPREG_NAMES_NAME);
        return 1;
    }

    ipreg_server_loop(NULL);

    // we shouldn't get here ...
    return 1;
}
