PKGDIR	?= ../..
L4DIR		?= $(PKGDIR)/../..
SRC_C 	= main.c
MODE		= l4linux
LIBS		= -lgeneric_ts
LDFLAGS	= -static
SYSTEMS	= x86-l4v2 arm-l4v2

include $(L4DIR)/mk/Makeconf

#killing of tasks isn't supported in tasklib currently
ifneq ($(USE_TASKLIB),y)
  TARGET += kill-l4
endif
	
include $(L4DIR)/mk/prog.mk
