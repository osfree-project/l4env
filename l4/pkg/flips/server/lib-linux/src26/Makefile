PKGDIR ?= ../../..
L4DIR  ?= $(PKGDIR)/../..

DEFINES = -D__KERNEL__ -DDDE_LINUX

all:: net

SYSTEMS = x86-l4v2

TARGET = liblinux26.o.a

SRC_C = dummy.c proc.c select.c init.c \
        timer.c

#loopback device
SRC_C += drivers/net/loopback.c
#random device
SRC_C += drivers/char/random.c
SRC_C += lib/halfmd4.c

SRC_C += $(addprefix net/core/, \
           dst.c request_sock.c stream.c netpoll.c)
SRC_C += $(addprefix net/ipv4/, \
           af_inet.c arp.c \
           datagram.c devinet.c \
           fib_frontend.c fib_hash.c fib_semantics.c \
           icmp.c igmp.c inetpeer.c \
           inet_connection_sock.c inet_hashtables.c inet_timewait_sock.c \
           ip_forward.c ip_fragment.c ip_input.c \
           ip_options.c ip_output.c ip_sockglue.c \
           protocol.c \
           raw.c route.c \
           sysctl_net_ipv4.c \
           tcp.c tcp_cong.c tcp_input.c tcp_ipv4.c tcp_minisocks.c \
           tcp_output.c tcp_timer.c udp.c udplite.c)

# DHCP support during boot phase
SRC_C += net/ipv4/ipconfig.c

vpath %.c $(PKGDIR)/server/lib-linux/src $(LINUX26_DIR)

# include autoconf.h of flips to enable INET etc.
# overrrides autoconf of dde26 
PRIVATE_INCDIR += $(PKGDIR)/server/lib-linux/src26
# include default dde26
include $(PKGDIR)/../dde/linux26/Makeconf
# local includes
PRIVATE_INCDIR += \
		  $(PKGDIR)/server/lib-linux/include \
                  $(PKGDIR)/server/include \
                  $(DDE26_INCDIR) $(LINUX26_INCDIR)


include $(L4DIR)/mk/lib.mk

#
# patch handling
#
TOPATCH_IPV4 = arp.c icmp.c inetpeer.c ip_fragment.c route.c tcp.c tcp_minisocks.c tcp_timer.c \
		af_inet.c devinet.c ipconfig.c
CPFILES_IPV4 = $(foreach patchfile, $(TOPATCH_IPV4),$(addprefix $(LINUX26_DIR)/net/ipv4/, $(patchfile)))
TOPATCH_CORE = dst.c
CPFILES_CORE = $(foreach patchfile, $(TOPATCH_CORE),$(addprefix $(LINUX26_DIR)/net/core/, $(patchfile)))

net:
	$(VERBOSE) if [ ! -d net/ipv4 ]; then     \
                mkdir -p net/ipv4 &&           \
                echo "  ... creating net/ipv4 subfolder" ; \
        fi
	$(VERBOSE) if [ ! -d net/core ]; then     \
                mkdir -p net/core &&           \
                echo "  ... creating net/core subfolder"; \
        fi
	$(VERBOSE) echo "  ... copying original sources to net/ipv4" && \
	cp $(CPFILES_IPV4) net/ipv4
	$(VERBOSE) echo "  ... copying original sources to net/core" && \
	cp $(CPFILES_CORE) net/core
	$(VERBOSE) patch -p0 <src.diff

# be paranoid and make a diff before deleting patched files
cleanall::
	! diff -ur $(LINUX26_DIR)/net net >sic.diff
	$(VERBOSE)rm -rf net
