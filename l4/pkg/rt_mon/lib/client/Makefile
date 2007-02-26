PKGDIR   ?= ../..
L4DIR    ?= $(PKGDIR)/../..

CLIENTIDL = rt_mon_reg.idl
SRC_C     = histogram.c histogram2d.c comm.c event_list.c scalar.c
SRC_S     = floor.S
DEFINES   = -D_GNU_SOURCE

MODE      = l4env_minimal

SYSTEMS   = x86-l4v2

TARGET    = librt_mon_client.a

include $(L4DIR)/mk/lib.mk
