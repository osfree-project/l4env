#
# Makefile for ORe linuxemul lib
#
# mostly taken from Oshkosh's Linux emulation
#

PKGDIR         ?= ../../..
L4DIR          ?= $(PKGDIR)/../..

TARGET		= liblinuxemul.a

DEFINES		= -D__KERNEL__ -D__ORE__ -DDDE_LINUX

LINUX_HEADER_DIR = $(PKGDIR)/server/include/linux_headers

PRIVATE_INCDIR	= 	$(LINUX_HEADER_DIR)				\
					$(DDE_INCDIR) $(LINUX24_INCDIR) \
				  	$(PKGDIR)/server/include

SRC_C		= arch/i386/lib/old-checksum.c				\
			  lib/crc32.c								\
			  drivers/net/net_init.c					\
			  drivers/net/Space.c						\
			  drivers/net/setup.c						\
			  net/core/dev.c							\
			  net/core/dev_mcast.c						\
			  net/core/utils.c							\
			  net/core/rtnl.c							\
			  net/ethernet/eth.c						\
			  emul.c									\
			  net/core/skbuff.c							\
#			  net/sched/sch_generic.c					\

SRC_S		= arch/i386/lib/checksum.S

all::
lib/crc32.o lib/crc32.pr.o : crc32table.h
lib/crc32.o lib/crc32.pr.o : PRIVATE_INCDIR += .

crc32table.h : gen_crc32table
	@$(GEN_MESSAGE)
	$(VERBOSE)./$< > $@

gen_crc32table : lib/gen_crc32table.c
	@$(GEN_MESSAGE)
	$(VERBOSE)$(HOST_CC) -O2 -o $@ $<

include $(L4DIR)/mk/lib.mk
