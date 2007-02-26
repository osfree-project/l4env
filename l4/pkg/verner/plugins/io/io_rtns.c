/*!
 * \file   io_rtns.h
 * \brief  OshKosh/rt-fs specific plugin for file-I/O
 * \date   08/13/2004
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of the VERNER, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <l4/crtx/ctor.h>
#include <l4/names/libnames.h>
#include <l4/sys/l4int.h>
#include <l4/oshkosh/rtns-abi.h>
#include <l4/oshkosh/beapi.h>
#include <l4/oshkosh/inet.h>
#include <l4/env/errno.h>
#include <l4/log/l4log.h>
#include <l4/util/reboot.h>	/* for experiments only! */
#include <l4/util/parse_cmd.h>
#include <l4/util/mbi_argv.h>
#include <l4/util/getopt.h>
#include <alloca.h>
#include "io_rtns.h"

#define LOCAL_PORT	5000
#define REMOTE_PORT	9500
#define WINDOW		20000	/* number of outstanding bytes */

static int bandwidth;		/* in bytes/ms */

#define OPEN_TIMEOUT	(10*1000*1000)
#define READ_TIMEOUT	(10*1000*1000)

static void parse_args(void){
    int argc=l4util_argc;
    const char**argv = alloca(sizeof(char*)*l4util_argc);

    opterr=0;
    memcpy(argv, l4util_argv, sizeof(char*)*l4util_argc);
    parse_cmdline(&argc, &argv,
		  ' ', "iortns-bw", "bandwidth (bytes/ms)",
		  PARSE_CMD_INT, 200, &bandwidth,
		  0, 0);
}
L4C_CTOR(parse_args, L4CTOR_AFTER_BACKEND);

/*
 * open
 */
int io_rtns_open (const char *name, int mode, ...){
    rtns_conn_t *c;
    l4_uint16_t r_port = htons(REMOTE_PORT), local_port=0;
    l4_uint32_t remote_ip, local_ip=0;
    int err;
    char ipname[50], *slash;

    LOG("name=\"%s\"", name);

    /* pase server address out of name */
    if((slash = strchr(name, '/'))==0 || slash-name>=sizeof(ipname)-1){
	LOG_Error("Invalid server address.");
	return -1;
    }
    memcpy(ipname, name, slash-name);
    ipname[slash-name]=0;
    if(oshk_inet_aton(ipname, &remote_ip)==0){
	LOG_Error("Invalid server address: %s", ipname);
	return -1;
    }

    /* Open the connection */
    if((err=rtns_open(&local_ip, &local_port,
		      remote_ip, r_port,
		      slash+1, bandwidth, WINDOW, &c,
		      OPEN_TIMEOUT))<0){
	l4env_perror("rtns_open", -err);
	return -1;
    }
    /* Start the transmission of the first packets */
    if((err = rtns_start(c, 0))!=0) return -1;
    return (int)c;
}

/*
 * close
 */
int io_rtns_close (int fd){
    rtns_conn_t *c = (rtns_conn_t*)fd;

    LOG("conn=%p", c);
    return rtns_close(c)?-1:0;
}

/*
 * read
 */
unsigned long io_rtns_read (int fd, void *buf, unsigned long n){
    rtns_conn_t *c = (rtns_conn_t*)fd;
    int res;

    res = rtns_read(c, buf, n, READ_TIMEOUT);
    if(res!=n){
        LOG("conn=%p read %d/%ld, returning %d", c, res, n, res<0?-1:res);
    }
    return res<0?-1:res;
}


unsigned long io_rtns_write (int fd, void *buf, unsigned long n){
    LOG("Not supported");
    return -1;
}


/*
 * seek
 */
long io_rtns_lseek (int fd, long offset, int whence){
    rtns_conn_t *c = (rtns_conn_t*)fd;

    if (whence == SEEK_SET) {
	int err;
	if(offset!=rtns_offset(c)){
	    LOG("conn=%p offset=%ld whence=%d (%+ld)",
		c, offset, whence, offset-rtns_offset(c));
	    if((err = rtns_start(c, offset))!=0) return -1;
	}
	return offset;
    }
    /* caller wants the current file offset */
    if (whence == SEEK_CUR){
	if(offset){
	    LOG("conn=%p %+ld whence=%d (%ld)",
		c, offset, whence, offset+rtns_offset(c));
	    offset += rtns_offset(c);
	    if(rtns_start(c, offset)) return -1;
	}
	return rtns_offset(c);
    }
    /* others not supported */
    LOG("Seek: %s not supported",
	whence == SEEK_CUR?"SEEK_CUR":whence == SEEK_END?"SEEK_END":"unknown");
    return -1;
}

/*
 * fstat - size only
 */
int io_rtns_fstat (int __fd, struct stat *buf){
    LOG("Not supported");
    return 0;
}


/*
 * not implemented yet
 */
int io_rtns_ftruncate (int __fd, unsigned long __offset){
    LOG("Not supported");
    return -1;
}

/*
 * Local variables:
 *  compile-command: "make -C ../../vdemuxer"
 * End:
 */
