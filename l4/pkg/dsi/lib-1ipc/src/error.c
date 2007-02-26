/*****************************************************************************/
/*!
 * \file   lib/src/error.c
 * \brief  Error strings for DSI
 *
 * \date   08/09/2001
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *
 */
/*****************************************************************************/

#include <l4/sys/types.h>
#include <l4/env/errno.h>
#include <l4/dsi/errno.h>
#include "__error.h"

L4ENV_ERR_DESC_STATIC(err_msg,
    { DSI_ENODSM,	"no dataspace manager found" },
    { DSI_ENOSOCKET,	"no socket descriptor available" },
    { DSI_ENOSTREAM,	"no stream descriptor available" },
    { DSI_ECONNECT,	"socket not connected" },
    { DSI_ENOSGELEM,	"no scatter gather list element available" },
    { DSI_ENOPACKET,	"no packet available" },
    { DSI_ENODATA,	"no data in packet" },
    { DSI_ESGLIST,	"scatter gather list too long" },
    { DSI_EGAP,		"chunk contains a gap" },
    { DSI_EEOS,		"end of stream" },
    { DSI_ECOMPONENT,	"component operation failed" },
    { DSI_EPHYS,	"chunk contains physical addresses" }
);

/** Register our own error-codes.
 */
int dsi_init_error(void){
    return l4env_err_register_desc(&err_msg);
}
