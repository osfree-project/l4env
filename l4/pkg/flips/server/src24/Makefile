PKGDIR        ?= ../..
L4DIR         ?= $(PKGDIR)/../..

# L4VFS libs
LIBS_l4vfs  = -ll4vfs_network_server-server -ll4vfs_select -ll4vfs_select_notify-server \
              -lflips_l4vfs_server

# FLIPS with loopback only
TARGET = flips
SRC_C_flips = server.c main.c notify.c
LIBS_flips  = -lsocket_linux.o -llinux.o -ldde_linux -lio -lomega0 \
              -llist_alloc  $(LIBS_l4vfs) -lparsecmdline

# FLIPS debug version with loopback only
TARGET += flips-standalone
SRC_C_flips-standalone = standalone.c single_thread.c multi_thread.c local.c
LIBS_flips-standalone  = -lsocket_linux.o -llinux.o -ldde_linux -llist_alloc -lio -lomega0

# FLIPS with experimental Linux driver
TARGET += flips-lxdrv
SRC_C_flips-lxdrv = server.c main_lxdrv.c notify.c
LIBS_flips-lxdrv  = -llxdrv.o $(LIBS_flips)

# FLIPS with ORe backend
FLIPS_DEPEND_PKG_ORE = $(strip $(if $(wildcard $(L4DIR)/pkg/ore),,ore))

ifeq ($(FLIPS_DEPEND_PKG_ORE), ore)

$(warning "Package ore is missing, skipping build of flips-ore.")

else

TARGET += flips-ore
SRC_C_flips-ore = $(SRC_C_flips)
ifeq ($(USE_DSI),y)
LIBS_flips-ore  = -lore.o -lore -ldsi $(LIBS_flips)
else
LIBS_flips-ore  = -lore.o -lore $(LIBS_flips)
endif

endif

ifeq ($(L4_TUN),y)
  LIBS_flips-lxdrv += -ll4tundrv.o -lnethub -lcxx_io -lcxx_log_io -lcxx_base
endif

DEFAULT_RELOC  = 0x01900000
SYSTEMS        = x86-l4v2

SERVERIDL_flips       = flips.idl flips-internal.idl
SERVERIDL_flips-lxdrv = flips.idl flips-internal.idl
SERVERIDL_flips-ore   = flips.idl flips-internal.idl
CLIENTIDL_flips       = flips-internal.idl
CLIENTIDL_flips-lxdrv = flips-internal.idl
CLIENTIDL_flips-ore   = flips-internal.idl

PRIVATE_INCDIR = $(PKGDIR)/server/lib-linux/include \
                 $(PKGDIR)/server/lib-lxdrv/include \
                 $(PKGDIR)/server/include           \
                 $(DDE_INCDIR) $(LINUX24_INCDIR) \
                 $(PKGDIR)/server/src24

PRIVATE_LIBDIR = $(PKGDIR_OBJ)/server/lib-linux/src/OBJ-$(SYSTEM) \
                 $(PKGDIR_OBJ)/server/lib-ore/src/OBJ-$(SYSTEM)   \
                 $(PKGDIR_OBJ)/server/lib-lxdrv/src/OBJ-$(SYSTEM)

DEFINES        = -D__KERNEL__ -DDDE_LINUX
WARNINGS       = -Wall -Wstrict-prototypes

vpath %.c $(PKGDIR)/server/src

include $(L4DIR)/mk/prog.mk
