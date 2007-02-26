PKGDIR = ..
L4DIR  ?= $(PKGDIR)/../..

SRC_DOX_REF = log.cfg

ifneq ($(wildcard $(L4DIR)/pkg/log_net/),)
DOXY_FLAGS  += INPUT+= ../../log_net/server/src/muxed.c \
		    ../../log_net/server/src/tcpip.c
endif

include $(L4DIR)/mk/doc.mk
