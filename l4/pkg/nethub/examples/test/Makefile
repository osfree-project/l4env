PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

include $(L4DIR)/mk/Makeconf

MODE		= sigma0
TARGET		= nethub_test
DEFAULT_RELOC	= 0x01500000
LIBCLIBS_oskit	= -nostdlib -lmc -loskit_support $(GCCLIB)
LIBCLIBS_diet	= -nostdlib -ldiet_c $(GCCLIB)
LIBCLIBS_uc	= -nostdlib -luc_c $(GCCLIB)
LIBCLIBS	= $(LIBCLIBS_$(LIBC_TYPE))
L4LIBS		= -lnethub \
                  -lcxx_start -lcxx_thread -lcxx_io -lcxx_util -lcxx_base \
		  -lcxx_log_io -llogserver -ll4sys -lnames $(ROOTLIB) \
		  -ll4util

SYSTEMS		= x86-l4v2 x86-l4x0

SRC_CC		= main.cc

CPPFLAGS 	+= -DLOG_TAG=\"nh_test\"
CXXFLAGS	+= -fno-exceptions -fno-rtti

OBJS            += snd.packets.o rcv.packets.o

vpath %.src ..

%.packets.o: %.packets.src
	$(OBJCOPY) -B i386 -O elf32-i386 -I binary $< $@

include $(L4DIR)/mk/prog.mk

