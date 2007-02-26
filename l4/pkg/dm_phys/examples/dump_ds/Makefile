PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= dump-ds
#MODE		= host
MODE		= l4linux
SYSTEMS		= x86-l4v2

LIBCINCDIR	= $(addprefix -I,$(L4INCDIR))
#LIBS		= -lnames -llogserver_capsule -ll4util -lrmgr -ldm_mem -ldm_generic
 
SRC_C		= main.c

include $(L4DIR)/mk/prog.mk
