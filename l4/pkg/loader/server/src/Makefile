PKGDIR            ?= ../..
L4DIR              = $(PKGDIR)/../..
SYSTEMS            = x86-l4v2 arm-l4v2 amd64-l4v2
TARGET             = loader

WITH_INTEGRITY ?= n
#WITH_INTEGRITY    ?= lyon
#WITH_INTEGRITY    ?= vtpm

DEFAULT_RELOC      = 0x01300000
DEFAULT_RELOC_arm  = 0x002c0000
PRIVATE_INCDIR     = $(PKGDIR)/server/src .
LIB_INTEGRITY-lyon = -llyon-client -lcrypto_sha1_linux
LIB_INTEGRITY-vtpm = -ltcg -ltcg_crypt -lcrypto_sha1_linux -lstpm-client
LIB_TASK-y         = -ltask_server.o
LIBS               = \
                    -lgeneric_ts \
                    -lgeneric_fprov \
                    -lloaderif \
                    -ldm_phys \
                    -levents \
                    -lparsecmdline \
                    -ll4util_root \
                    -lipcmon

SRC_C		= main.c cfg-parse.c cfg.c pager.c app.c fprov-if.c dm-if.c \
		  idl.c events.c elf-loader.c kquota.c

SRC_C_INTEGRITY-lyon = integrity.c
SRC_C_INTEGRITY-vtpm = integrity.c
SRC_C		+= $(SRC_C_INTEGRITY-$(WITH_INTEGRITY))

SERVERIDL	= loader.idl
OBJS		= trampoline.s.o

include $(L4DIR)/mk/prog.mk

DEFINES_INTEGRITY-lyon = -DUSE_INTEGRITY -DUSE_INTEGRITY_LYON
DEFINES_INTEGRITY-vtpm = -DUSE_INTEGRITY -DUSE_INTEGRITY_VTPM
DEFINES		+= $(DEFINES_INTEGRITY-$(WITH_INTEGRITY))

LIBS		+= $(LIB_TASK-$(USE_TASKLIB))
LIBS		+= $(LIB_INTEGRITY-$(WITH_INTEGRITY))

cfg-parse.c: cfg-scan.c

WARNINGS_cfg-parse.o = -Wno-unused -Wno-parentheses \
		       -Wno-missing-prototypes -Wno-missing-declarations
