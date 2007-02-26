
/** MORE SUITABLE BINDINGS FOR IDL CALLS
 *
 * These defines are not 100% compatible with the linux interface.
 * Note the way of how the length parameter is transmitted. Currently,
 * DICE only allows the length parameter of an [out] array to be a
 * [in,out] int pointer.
 */

#define socket(domain,type,protocol) flips_socket_call(&srv,domain,type,protocol,&env)
#define connect(sd,addr,addrlen) flips_connect_call(&srv,sd,addr,addrlen,&env)
#define read(sd,buf,size) flips_read_call(&srv,sd,buf,size,&env)
#define write(sd,buf,size) flips_write_call(&srv,sd,buf,size,&env)
#define accept(sd,addr,addrlen) flips_accept_call(&srv,sd,addr,addrlen,&env)
#define listen(sd,backlog) flips_listen_call(&srv,sd,backlog,&env)
#define bind(sd,addr,addrlen) flips_bind_call(&srv,sd,addr,addrlen,&env)
