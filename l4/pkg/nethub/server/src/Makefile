PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

WITHOUT_CRYPTO 	?= y
USE_L4CRYPTO    ?= n

include $(L4DIR)/mk/Makeconf

MODE		:= sigma0
TARGET		:= nethub
DEFAULT_RELOC	:= 0x01000000
LIBCLIBS_oskit	= -nostdlib -lmc -loskit_support $(GCCLIB)
LIBCLIBS_diet	= -nostdlib -ldiet_c $(GCCLIB)
LIBCLIBS_uc	= -nostdlib -luc_c $(GCCLIB)
LIBCLIBS	= $(LIBCLIBS_$(LIBC_TYPE))
L4LIBS		= -lcxx_start -lcxx_thread -lcxx_io -lcxx_base -lcxx_util \
		  -lcxx_log_io -ll4sys -llogserver -lnames $(ROOTLIB) \
		  -ll4util

SRC_CC		:= main.cc ip_forward.cc ip.cc ip_sec.cc pager.cc \
                   region.cc sadb.cc sadb_cfg.cc routing.cc       \
		   routing_fab.cc interface.cc ip_sec_fwd.cc      \
		   ike_connector.cc time.cc

PRIVATE_INCDIR	+= ..
SRC_C		:= 
ifeq ("$(WITHOUT_CRYPTO)","y")
  DEFINES	+= -DWITHOUT_CRYPTO
else
ifeq ("$(USE_L4CRYPTO)","y")
  DEFINES	+= -DUSE_L4CRYPTO
  L4LIBS 	+= -ll4crypto
  SRC_CC	+= ip_sec_l4crypto.cc
else
PRIVATE_INCDIR	+= ../crypto
  SRC_C 	+= crypto/ipsec_md5c.c
endif
endif

SYSTEMS		= x86-l4v2 x86-l4x0

SERVERIDL	:= nh.idl
IDL_PATH	:= $(PKGDIR)/base_lib/idl/OBJ-$(ARCH)-$(L4API)


CPPFLAGS 	+= -DLOG_TAG=\"nethub\"
CXXFLAGS	+= -fno-rtti -fno-exceptions -Wall

include $(L4DIR)/mk/prog.mk

ifeq ($(filter clean cleanall scrub,$(MAKECMDGOALS)),)
ifeq ("$(WITHOUT_CRYPTO)$(USE_L4CRYPTO)","nn")
OBJS		+= ../crypto/libdes.a
endif
endif

