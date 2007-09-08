/*
 * \brief   Gluefile for using the TPM emulator daemon tpmd.
 * \date    2006-12-20
 * \author  Carsten Weinhold <weinhold@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2006 Carsten Weinhold <weinhold@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the STPM package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#include <l4/lxfuxlibc/lxfuxlc.h>
#include <string.h>
#include <errno.h>

/*
 * **************************************************************************
 */

/* The following macro restarts a system call, if it has been interrupted
 * by a signal (currently: -EINTR, -ERESTARTSYS) */

#define RETRY_LOOP(__ret, __func, __args...) \
    {                                        \
        do                                   \
            __ret = __func(__args);          \
        while (__ret == -4 || ret == -512);  \
    }

/*
 * **************************************************************************
 */

static char *TPM_EMU_SOCK_NAME="/var/tpm/tpmd_socket:0";

/*
 * **************************************************************************
 */

/**
 * Transmit a blob to the TPM
 */
int stpm_transmit(const char *write_buf, int write_count, char **read_buf, int *read_count)
{
    struct lx_sockaddr_un addr;
    int fd, ret;

    fd = lx_socket(LX_PF_UNIX, LX_SOCK_STREAM, 0);
    if (fd < 0)
        return -ENODEV;

    addr.sun_family = LX_AF_UNIX;
    strncpy(addr.sun_path, TPM_EMU_SOCK_NAME, sizeof(addr.sun_path));

    ret = lx_connect(fd, (struct lx_sockaddr *) &addr, sizeof(addr));
    if (ret >= 0)
    {
        RETRY_LOOP(ret, lx_write, fd, write_buf, write_count);
        if (ret > 0)
            RETRY_LOOP(ret, lx_read, fd, *read_buf, *read_count);

        *read_count = (ret >= 0) ? ret : 0;
    }
    else
        ret = -ENODEV;

    lx_close(fd);
    return ret;
}


/**
 * Abort the tpm call.
 */
int stpm_abort(void)
{
    return -ENODEV;
}
