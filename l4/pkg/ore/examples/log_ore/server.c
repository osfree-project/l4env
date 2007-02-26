#include <l4/thread/thread.h>
#include <l4/log/l4log.h>
#include <l4/sys/ipc.h>
#include <l4/sys/syscalls.h>
#include <l4/sys/kdebug.h>
#include <l4/util/macros.h>
#include <l4/util/parse_cmd.h>
#include <l4/env/errno.h>
#include <l4/ore/uip-ore.h>

#include <string.h>
#include <stdlib.h>

#include "log_comm.h"
#include "muxed.h"
#include "log-server.h"

char *ip_addr   = "127.0.0.1";
int port_nr     = 23;
int debug       = 0;
int log_kdebug  = 1;
int binary      = 0;

#if 0
rcv_message_t message;

static int get_msg(char *msgbuf, unsigned msgbuf_size);

/* get_msg - wait for a log message from a client.
 *
 * Taken from pkg/dmon/server/src/logger.c
 */
static int get_msg(char *msgbuf, unsigned msgbuf_size)
{
    int err;

    static l4_threadid_t client = L4_INVALID_ID;

    message.size.md.strings = 1;
    message.size.md.dwords = 7;
    message.string.rcv_size = msgbuf_size;
    message.string.rcv_str = (l4_umword_t) msgbuf; 
    message.fpage = /* channel_get_next_fpage() */ l4_fpage(0,0,0,0);

    memset(msgbuf, 0, LOG_BUFFERSIZE);
    while (1) {
        if (l4_thread_equal(client, L4_INVALID_ID)) {
            err = l4_ipc_wait(&client, &message, &message.dw0, &message.dw1,
                              L4_IPC_NEVER,
                              &message.result);
        } else {
            err = l4_ipc_reply_and_wait(client, NULL, message.dw0, 0,
                                        &client, &message, &message.dw0, &message.dw1,
                                        L4_IPC_SEND_TIMEOUT_0,
                                        &message.result);
            /* don't care for non-listening clients */
            if (err == L4_IPC_SETIMEOUT) {
                client = L4_INVALID_ID;
                continue;
            }
        }

        if (err == 0 || err == L4_IPC_REMSGCUT) {
            if (message.result.md.fpage_received == 0) { 
                switch (message.dw0) {
                       case LOG_COMMAND_LOG:
                            if (message.result.md.strings != 1) { 
                                /* aw9's hack from log/server/logserver.c for
                                 * the kernel tracebuffer
                                 */
                                if ((message.dw1 & L4_PAGEMASK) > 0xc0000000)
                                {
                                    //message.dw0 = channel_open();
                                    continue;
                                }
                        
                                // else it is a simple log message
                                message.dw0 = -L4_EINVAL;
                                continue;
                            }
                            /* GOTCHA */
                            return LOG_COMMAND_LOG;
                       case LOG_COMMAND_CHANNEL_WRITE:
                            if (message.result.md.dwords != 4)
                            {
                                message.dw0 = -L4_EINVAL;
                                continue;
                            }
                            //message.dw0 = channel_write();
                            continue;
                       case LOG_COMMAND_CHANNEL_FLUSH:
                            //message.dw0 = channel_flush();
                            continue;
                       case LOG_COMMAND_CHANNEL_CLOSE:
                            //message.dw0 = channel_close();
                            continue;
                            break;
                }
            }
            else // fpage received --> open channel
            {
                //message.dw0 = channel_open();
                continue;
            }

            // everything else is invalid
            message.dw0 = -L4_EINVAL;
            continue;
        }

        /* IPC was canceled - try again */
        if (err == L4_IPC_RECANCELED)
            continue;

        /* hm, serious error ? */
        snprintf(msgbuf, msgbuf_size,
                 "log_ore     | Error %#x getting log message from " l4util_idfmt"\n",
                 err, l4util_idstr(client));
        client = L4_INVALID_ID;
        return err;
    }
}
#endif

void
log_outstring_component (CORBA_Object _dice_corba_obj,
    int flush_flag,
    const char* string,
    CORBA_Server_Environment *_dice_corba_env)
{
    if (debug)
	outstring("LOG MESSAGE\n");
    if (log_kdebug)
	outstring(string);
    
    if (binary)
	//log_ore_send_to_channel(msg, strlen(msg), 1)
	;
    else
    {
	uip_ore_send(string, strlen(string), port_nr);
    }
    
//     if(flush_flag ||
// 	strstr(string, "***"))
// 	flush_buffer();
}

int
log_channel_open_component (CORBA_Object _dice_corba_obj,
    l4_snd_fpage_t page,
    int channel,
    CORBA_Server_Environment *_dice_corba_env)
{
// #if CONFIG_USE_TCPIP
//     int ret = channel_open(page.fpage, channel);
//     _dice_corba_env->rcv_fpage = channel_get_next_fpage();
//     return ret;
// #else
    return -L4_EINVAL;
// #endif
}

int
log_channel_write_component (CORBA_Object _dice_corba_obj,
    int channel,
    unsigned int offset,
    unsigned int size,
    CORBA_Server_Environment *_dice_corba_env)
{
// #if CONFIG_USE_TCPIP
//     return channel_write(channel, offset, size);
// #else
    return -L4_EINVAL;
// #endif
}

int
log_channel_flush_component (CORBA_Object _dice_corba_obj,
    int channel,
    CORBA_Server_Environment *_dice_corba_env)
{
// #if CONFIG_USE_TCPIP
//     return channel_flush(channel);
// #else
    return -L4_EINVAL;
// #endif
}

int
log_channel_close_component (CORBA_Object _dice_corba_obj,
    int channel,
    CORBA_Server_Environment *_dice_corba_env)
{
// #if CONFIG_USE_TCPIP
//     return channel_close(channel);
// #else
    return -L4_EINVAL;
// #endif
}


int main(int argc, const char **argv)
{
    int ret;
    uip_ore_config conf;
    CORBA_Server_Environment env = dice_default_server_environment;
    env.rcv_fpage = l4_fpage(0,0,0,0);
    env.malloc = (dice_malloc_func)malloc;

    ret = parse_cmdline(&argc, &argv,
            'i', "ip", "specify IP address",
            PARSE_CMD_STRING, "127.0.0.1", &ip_addr,
            'p', "port", "specify port to listen",
            PARSE_CMD_INT, 23, &port_nr,
            'q', "quiet", "no LOG output in kdebug",
            PARSE_CMD_SWITCH, 0, &log_kdebug,
            'd', "debug", "debug mode",
            PARSE_CMD_SWITCH, 1, &debug,
            'b', "binary", "enable binary channels",
            PARSE_CMD_SWITCH, 1, &binary,
            0);
    
    if (log_kdebug)
        outstring("logging to kdebug\n");
    else
        outstring("NOT logging to kdebug\n");
    if (debug)
        outstring("debugging log_ore\n");
    if (binary)
        outstring("binary channel mode\n");

    strcpy(conf.ip, ip_addr);
    conf.port_nr = port_nr;
    uip_ore_initialize(&conf);
    ret = l4thread_create(uip_ore_thread, NULL, L4THREAD_CREATE_SYNC);
    
    outstring("log_ore initialized\n");

    if (!names_register(LOG_NAMESERVER_NAME))
        Panic("Could not register %s at names!", LOG_NAMESERVER_NAME);
    
    log_server_loop (&env);
#if 0
    while(1)
    {
        char *msg;
        int ret;
        struct rxtx_entry *ent;

        msg = malloc(LOG_BUFFERSIZE);
        if (!msg)
            Panic("out of memory");

        ret = get_msg(msg, LOG_BUFFERSIZE);
        
        switch(ret)
        {
            case LOG_COMMAND_LOG:
                if (debug)
                    outstring("LOG MESSAGE\n");
                if (log_kdebug)
                    outstring(msg);

                if (binary)
                    //log_ore_send_to_channel(msg, strlen(msg), 1)
                    ;
                else
                {
                    uip_ore_send(msg, strlen(msg), port_nr);
                }
                break;
            default:
                break;
        }
    }
#endif
}

