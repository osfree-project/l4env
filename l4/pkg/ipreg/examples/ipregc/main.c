/* generic includes */
#include <stdio.h>
#include <stdlib.h>

/* L4-specific includes */
#include <l4/names/libnames.h>

/* local includes */
#include "ipreg-client.h"

/*
 * *****************************************************************
 */

char LOG_tag[9] = "ipregc";

/*
 * *****************************************************************
 */

int main(int argc, char **argv) {

    DICE_DECLARE_ENV(env);
    l4_threadid_t server_id;
    int           ret;

    if (argc != 2 && argc != 3) {
        printf("Invalid commandline arguments!\n");
        printf("  usage: ipregc NAME [IP]\n");
        return 1;
    }
    
    if ( !names_query_name(IPREG_NAMES_NAME, &server_id)) {
        printf("'%s' not registered at names!\n", IPREG_NAMES_NAME);
        return 1;
    }

    if (argc == 2) {
        char *received_addr = malloc(IPREG_MAX_NAME_LEN);

        if (received_addr == NULL) {
            printf("out of memory\n");
            return 1;
        }

        ret = ipreg_query_call(&server_id, argv[1], &received_addr, &env);
        if (ret == 0)
            printf("%s\n", received_addr);

    } else
        ret = ipreg_register_call(&server_id, argv[1], argv[2], &env);

    return (ret == 0) ? 0 : 1;
}

