PKGDIR    ?= ../..
L4DIR     ?= $(PKGDIR)/../..

DEPENDS_PKGS = gmp
# rtc l4vfs

TARGET    = tpmemu-l4
SYSTEMS   = x86-l4v2
MODE      = l4env

# the default relocation address. This may be superseded by a STATIC file.
DEFAULT_RELOC	= 0x017C0000

SERVERIDL	= stpm.idl

TPM_DIRS  = $(PKGDIR)/contrib/tpmemulator/tpm \
            $(PKGDIR)/contrib/tpmemulator/crypto 

FILES     = server.c glue.c events.c link.c \
             $(foreach file, \
               $(foreach dir, $(TPM_DIRS), $(wildcard $(dir)/*.c)), \
                 $(lastword $(subst /, ,$(file))) \
              )
SRC_C     = $(PKGDIR_OBJ)/server/tpmemu/integrity.c
SRC_C    += $(filter-out tpm_integrity.c, $(FILES))

LIBS		  = -lgmp
LIBS     += -ltcg -ltcg_crypt
LIBS     += -lcrypto_sha1_linux -lcrypto_aes_linux_586.o -lcrypto_modes
LIBS     += -lstpm-client
LIBS		 += -lparsecmdline
LIBS		 += -lfprov_ext
LIBS		 += -levents

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

$(PKGDIR_OBJ)/server/tpmemu/integrity.c:
	cp $(PKGDIR)/contrib/tpmemulator/tpm/tpm_integrity.c $(PKGDIR_OBJ)/server/tpmemu/integrity.c
	patch $(PKGDIR_OBJ)/server/tpmemu/integrity.c <$(PKGDIR)/server/tpmemu/integrity.diff
