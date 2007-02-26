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
