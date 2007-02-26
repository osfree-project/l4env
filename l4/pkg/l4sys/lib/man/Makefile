#
# $Id$
#
# Makefile for L4 systemcall manpages
#

# scan the L4 global configuration file
ifeq (${L4_DIR},)
  L4DIR = ../../../..
else
  L4DIR = ${L4_DIR}
endif
include $(L4DIR)/Makeconf

# scan the local configuration file if it exists
ifneq (${L4INSTALLDIR},)
  L4_INSTALL_DIR = ${L4INSTALLDIR}
endif

.SUFFIXES: .sgml .html .man .ps

SYSCALLS = l4_fpage_unmap.man l4_myself.man l4_nchief.man l4_task_new.man \
           l4_thread_ex_regs.man l4_thread_switch.man l4_thread_schedule.man 

IPC = l4_ipc_call.man l4_ipc_send_deceiting.man l4_ipc.man \
      l4_ipc_wait.man l4_ipc_reply_deceiting_and_wait.man \
      l4_ipc_reply_and_wait.man l4_ipc_receive.man  \
      l4_ipc_send.man 

man all: $(SYSCALLS) $(IPC)

html: $(SYSCALLS:.man=.html) $(IPC:.man=.html)

.sgml.man:    
	sdc -Oman -o$*.man $*.sgml

.sgml.html:    
	sdc -Ohtml -o$*.html $*.sgml

.sgml.ps:
	sdc -Ops -o$*.ps $*.sgml

clean:
	rm -f *.ps *.html

cleanall:	clean
	rm -f *~

clean-man: cleanall
	rm -f *.man

install:	man
	@echo "Installing Manuals..."
	$(INSTALL) -d $(L4_INSTALL_DIR)/man/man2
	$(INSTALL) $(SYSCALLS) $(L4_INSTALL_DIR)/man/man2
	$(INSTALL) $(IPC) $(L4_INSTALL_DIR)/man/man2
