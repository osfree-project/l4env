/**
 * \file    ddekit/include/config.h
 * \brief   Configuration file for ddekit.
 *
 * FIXME this file is empty
 */

#ifndef __ddekit_config_h
#define __ddekit_config_h

#define DEBUG_MSG(msg, ...) ddekit_printf("%s: \033[35m"msg"\033[0m\n", __FUNCTION__, ##__VA_ARGS__)

#endif /* __ddekit_config_h */
