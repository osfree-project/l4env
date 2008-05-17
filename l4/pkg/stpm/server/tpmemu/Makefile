PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

DEPENDS_PKGS = gmp
# rtc l4vfs

TARGET  = tpmemu-l4
SYSTEMS = x86-l4v2
MODE    = l4env

# the default relocation address. This may be superseded by a STATIC file.
DEFAULT_RELOC	= 0x017C0000

SERVERIDL	= stpm.idl

TPM_DIRS  = $(PKGDIR)/contrib/tpmemulator/tpm \
            $(PKGDIR)/contrib/tpmemulator/crypto 

SRC_C     = server.c glue.c \
             $(foreach file, \
               $(foreach dir, $(TPM_DIRS), $(wildcard $(dir)/*.c)), \
                 $(lastword $(subst /, ,$(file))) \
              )

LIBS		=  -lgmp -ltcg -lstpm-client
LIBS		+= -lparsecmdline

#required to run tpmd with socket interface ...

#-lc_be_io.o -lc_be_socket_io \
#                   -ll4vfs_common_io -ll4vfs_net_io \
#                   -ll4vfs_basic_name_server -ll4vfs_basic_io \
#                   -ll4vfs_connection -ll4vfs_name_server \
#                   -lc_be_file-table \
#                   -lgmp 
#                   -lc_be_select -ll4vfs_select -ll4vfs_select_listener-server \
#                   -lc_be_time -lrtc
# -lc_be_file-table -lc_be_l4sig.o -lsignal \

# we need some special dirs
PRIVATE_INCDIR = \
                 $(PKGDIR)/contrib/tpmemulator \
                 $(PKGDIR)/contrib/tpmemulator/tpm \
                 $(PKGDIR)/contrib/tpmemulator/tpmd \
                 $(PKGDIR)/server/tpmemu \
                 $(PKGDIR)/include

vpath %.c $(strip $(TPM_DIRS))

include $(L4DIR)/mk/prog.mk
