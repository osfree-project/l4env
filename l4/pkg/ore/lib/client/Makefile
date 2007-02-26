PKGDIR?= ../..
L4DIR ?= $(PKGDIR)/../..

TARGET    = libore.a
SYSTEMS   = x86-l4v2
SRC_C     = lib.c open.c rx_string.c tx_string.c close.c debug.c
CLIENTIDL = ore_manager.idl ore_rxtx.idl

ifeq ($(USE_DSI),y)
SRC_C	  += rx_dsi.c tx_dsi.c dsi.c
CFLAGS	  += -DORE_DSI
else
SRC_C     += dummy_dsi.c
endif

include $(L4DIR)/mk/lib.mk
