PKGDIR        ?= ../..
L4DIR         ?= $(PKGDIR)/../..

SYSTEMS = x86-l4v2

include $(PKGDIR)/../dde/linux26/Makeconf

# L4VFS libs
LIBS_l4vfs  = -ll4vfs_network_server-server -ll4vfs_select -ll4vfs_select_notify-server \
              -lflips26_l4vfs_server

# FLIPS with loopback only
TARGET = flips26
SRC_C_flips26 = server.c main.c notify.c
LIBS_flips26  = -lsocket_linux26.o -llinux26.o \
                -ldde_linux26 -lddekit -ldde_linux26_net_ip -ldde_linux26 -ldde_linux26_net_ip \
                -lio -lomega0 \
                -llist_alloc  $(LIBS_l4vfs) -lparsecmdline

# FLIPS debug version with loopback only
#TARGET += flips26-standalone
SRC_C_flips26-standalone = standalone.c single_thread.c multi_thread.c local.c
LIBS_flips26-standalone  = -lsocket_linux26.o -llinux26.o -ldde_linux26 -lddekit -llist_alloc -lio -lomega0

# FLIPS with experimental Linux driver
#TARGET += flips26-lxdrv
SRC_C_flips26-lxdrv = server.c main_lxdrv.c notify.c
LIBS_flips26-lxdrv  = -llxdrv.o $(LIBS_flips26)

# FLIPS with ORe backend
FLIPS_DEPEND_PKG_ORE = $(strip $(if $(wildcard $(L4DIR)/pkg/ore),,ore))

ifeq ($(FLIPS_DEPEND_PKG_ORE), ore)

$(warning "Package ore is missing, skipping build of flips-ore.")

else

TARGET += flips26-ore
SRC_C_flips26-ore = $(SRC_C_flips26)
ifeq ($(USE_DSI),y)
LIBS_flips26-ore  = -lore26.o -lore -ldsi $(LIBS_flips26)
else
LIBS_flips26-ore  = -lore26.o -lore $(LIBS_flips26)
endif

endif

ifeq ($(L4_TUN),y)
  LIBS_flips26-lxdrv += -ll4tundrv.o -lnethub -lcxx_io -lcxx_log_io -lcxx_base
endif

DEFAULT_RELOC  = 0x01900000
SYSTEMS        = x86-l4v2

SERVERIDL_flips26       = flips.idl flips-internal.idl
SERVERIDL_flips26-lxdrv = flips.idl flips-internal.idl
SERVERIDL_flips26-ore   = flips.idl flips-internal.idl
CLIENTIDL_flips26       = flips-internal.idl
CLIENTIDL_flips26-lxdrv = flips-internal.idl
CLIENTIDL_flips26-ore   = flips-internal.idl

PRIVATE_INCDIR += $(PKGDIR)/server/lib-linux/include 	\
                 $(PKGDIR)/server/include           	\
                 $(PKGDIR)/server/src26		 	\
                 $(PKGDIR)/server/lib-lxdrv/include 

PRIVATE_LIBDIR = $(PKGDIR_OBJ)/server/lib-linux/src/OBJ-$(SYSTEM) \
                 $(PKGDIR_OBJ)/server/lib-ore/src/OBJ-$(SYSTEM)   \
                 $(PKGDIR_OBJ)/server/lib-lxdrv/src/OBJ-$(SYSTEM)

DEFINES        = -D__KERNEL__ -DDDE_LINUX
WARNINGS       = -Wall -Wstrict-prototypes

vpath %.c $(PKGDIR)/server/src

include $(L4DIR)/mk/prog.mk
