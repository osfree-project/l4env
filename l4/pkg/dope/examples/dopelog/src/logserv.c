#include "logserv.h"
#include <stdio.h>
#include <string.h>
#include <l4/names/libnames.h>
#include <l4/env/errno.h>
#include <l4/sys/ipc.h>
#include <l4/semaphore/semaphore.h>
#include "stringlist.h"

#define LOG_BUFFERSIZE 81
#define LOG_NAMESERVER_NAME "stdlogV05"
#define LOG_COMMAND_LOG 0

stringlist_t log_stringlist;
l4semaphore_t sem_log_sl = L4SEMAPHORE_INIT(0);

static char message_buffer[LOG_BUFFERSIZE + 5];

struct {
	l4_fpage_t   fpage;
	l4_msgdope_t size;
	l4_msgdope_t snd;
	l4_umword_t  d0, d1, d2, d3, d4, d5, d6;
	l4_strdope_t string;
	l4_msgdope_t result;
} message;

// ! the sender of the last request
l4_threadid_t msg_sender = L4_INVALID_ID;

static int get_message(void) {
	int err;


	message.size.md.strings = 1;
	message.size.md.dwords = 7;
	message.string.rcv_size = LOG_BUFFERSIZE;
	message.string.rcv_str = (l4_umword_t) & message_buffer;
	message.fpage.fpage = 0;

	memset(message_buffer, 0, LOG_BUFFERSIZE);
	while (1) {
		if (l4_thread_equal(msg_sender, L4_INVALID_ID)) {
			err = l4_ipc_wait(&msg_sender, &message, &message.d0, &message.d1,
					L4_IPC_TIMEOUT(0, 0, 0, 0, 0, 0),
					&message.result
			);
			if (err != 0) return err;
			break;
		} else {
			err = l4_ipc_reply_and_wait(msg_sender, NULL, message.d0, 0,
					   &msg_sender, &message,
					   &message.d0, &message.d1,
					   L4_IPC_TIMEOUT(0, 1, 0, 0, 0, 0),
					   &message.result
				);
			if (err & L4_IPC_SETIMEOUT)
				msg_sender = L4_INVALID_ID;
			else
				break;
		}
	}
	return 0;
}

void logserver_loop(void* data) {
	int err;
	char str[256];

	names_register(LOG_NAMESERVER_NAME);
	strcpy(message_buffer + LOG_BUFFERSIZE, "...\n");

	memset(&log_stringlist, 0, sizeof(log_stringlist));
	l4thread_started(NULL);

	while (1) {
		err = get_message();
		if (err == 0 || err == L4_IPC_REMSGCUT) {
			if (message.result.md.fpage_received == 0) {
				switch (message.d0) {
					case LOG_COMMAND_LOG:
						if (message.result.md.strings != 1) {
							message.d0 = -L4_EINVAL;
							continue;
						}

						outstring(message_buffer);
						fifo_in(&log_stringlist, message_buffer);
						l4semaphore_up(&sem_log_sl);
						message.d0 = 0;
						continue;
				}
			}
			message.d0 = -L4_EINVAL;
			continue;
		}

		if (err & 0x40)
			continue;

		sprintf(str, "dope_log | Error %#x getting message from %x.%x.\n", err, msg_sender.id.task, msg_sender.id.lthread);
		fifo_in(&log_stringlist, str);
		l4semaphore_up(&sem_log_sl);

		msg_sender = L4_INVALID_ID;
	}
}
