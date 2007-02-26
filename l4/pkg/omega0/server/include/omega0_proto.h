#ifndef __OMEGA0_PROTO_H
#define __OMEGA0_PROTO_H

/* for internal use of client and server */

#define MANAGEMENT_THREAD	0	// thread number of management thread

typedef enum{
	OMEGA0_ATTACH,
	OMEGA0_DETACH,
	OMEGA0_PASS,
	OMEGA0_REQUEST,
	OMEGA0_FIRST,
	OMEGA0_NEXT,
	OMEGA0_DETACH_ALL,
} omege0_request_descriptor;

#define OMEAG0_SERVER_NAME	"omega0" // name to register with at nameserver
#define NAMESERVER_WAIT_MS	5000	// wait up to 5 s for nameserver

#endif
