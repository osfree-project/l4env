/*
 * \brief  Utility function to find a free port to use for RPC communication
 * \author Norman Feske
 * \date   2003-08-20
 */

/*
 * Copyright (C) 2002-2004  Norman Feske  <nf2@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the Overlay WM package, which is distributed
 * under the  terms  of the GNU General Public Licence 2. Please see
 * the COPYING file for details.
 */

#include <sys/socket.h>
#include <sys/types.h>
#include <resolv.h>
#include <arpa/inet.h>
#include <unistd.h>

int get_free_port(void);
int get_free_port(void) {

	int sh;
	struct sockaddr_in myaddr;
	int namelen = sizeof(struct sockaddr);

	myaddr.sin_family = AF_INET;
	myaddr.sin_port = 0;
	myaddr.sin_addr.s_addr = INADDR_ANY;

	sh = socket (AF_INET, SOCK_STREAM, 0);
	bind(sh,(struct sockaddr *)&myaddr,sizeof(struct sockaddr_in));
	getsockname(sh,(struct sockaddr *)&myaddr,&namelen);
	close(sh);

	return (int)myaddr.sin_port;
}
