# 
# $Id$
#
# Makefile for IO flex pages tests: 
#

# scan the L4 global configuration file
ifeq (${L4_DIR},)
  L4DIR = ../../../../..
else
  L4DIR = ${L4_DIR}
endif
include $(L4DIR)/Makeconf

# scan the local configuration file if it exists
ifneq (${L4INSTALLDIR},)
  L4_INSTALL_DIR = ${L4INSTALLDIR}
endif

-include Makeconf.local			# read possible PRIVATE_DIRS

# enviroment
CFLAGS +=	-O -Wall

CXXFLAGS += 	-Wall

SRC_CC =	iotest.cc port_access.cc io_page_acc.cc \
		clisti.cc kernel_mem.cc \
		maphole.cc

SRC_S =		crt0.S int_entry.S

LIBS =		-lmc -loskit_support -lmc -llmm -lkern -lrmgr

TARGET =	port_access

OBJS_PORT_ACCESS= crt0.o int_entry.o iotest.o port_access.o 
OBJS_IO_PAGE_ACC= crt0.o int_entry.o iotest.o io_page_acc.o 
OBJS_CLISTI= crt0.o int_entry.o iotest.o clisti.o
OBJS_KERNEL_MEM= crt0.o int_entry.o iotest.o kernel_mem.o
OBJS_MAPHOLE= crt0.o int_entry.o iotest.o maphole.o

#####################
# default make action

all: port_access port_access.disasm io_page_acc io_page_acc.disasm \
	clisti clisti.disasm kernel_mem kernel_mem.disasm \
	maphole maphole.disasm

port_access.disasm: port_access
	objdump -lD port_access > port_access.disasm

io_page_acc.disasm: io_page_acc
	objdump -lD io_page_acc > io_page_acc.disasm

clisti.disasm: clisti
	objdump -lD clisti > clisti.disasm

kernel_mem.disasm: kernel_mem
	objdump -lD kernel_mem > kernel_mem.disasm

maphole.disasm: maphole
	objdump -lD maphole > maphole.disasm

#int_entry.o: int_entry.S
#	$(CC) -E -D__ASSEMBLY__ -traditional -c $*.S -o $*.pre
#	$(CROSS_COMPILE)gasp -D__ASSEMBLY__ $*.pre -o $*.s
#	$(CC) -c -D__ASSEMBLY__ -traditional -c $*.s -o $*.o

port_access:		$(OBJS_PORT_ACCESS)
	$(VERBOSE)$(LD) -N -Ttext 0x200000 -o $@ $^ $(L4ALL_LIBDIR) $(LIBS)
	$(INSTALL) $@ -s -m 755 $(L4DIR)/bin/

io_page_acc:		$(OBJS_IO_PAGE_ACC)
	$(VERBOSE)$(LD) -N -Ttext 0x200000 -o $@ $^ $(L4ALL_LIBDIR) $(LIBS)
	$(INSTALL) $@ -s -m 755 $(L4DIR)/bin/

clisti:		$(OBJS_CLISTI)
	$(VERBOSE)$(LD) -N -Ttext 0x200000 -o $@ $^ $(L4ALL_LIBDIR) $(LIBS)
	$(INSTALL) $@ -s -m 755 $(L4DIR)/bin/

kernel_mem:		$(OBJS_KERNEL_MEM)
	$(VERBOSE)$(LD) -N -Ttext 0x200000 -o $@ $^ $(L4ALL_LIBDIR) $(LIBS)
	$(INSTALL) $@ -s -m 755 $(L4DIR)/bin/

maphole:		$(OBJS_MAPHOLE)
	$(VERBOSE)$(LD) -N -Ttext 0x200000 -o $@ $^ $(L4ALL_LIBDIR) $(LIBS)
	$(INSTALL) $@ -s -m 755 $(L4DIR)/bin/

include $(L4DIR)/Makeconf.stdtargets

# read module dependencies
-include $(DEPS)
