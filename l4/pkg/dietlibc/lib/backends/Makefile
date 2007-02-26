PKGDIR	?= ../..
L4DIR	?= $(PKGDIR)/../..

# the default is to build the listed directories, provided that they
# contain a Makefile. If you need to change this, uncomment the following
# line and adapt it.
TARGET	= preliminary minimal l4env_base                                 \
	  l4_start_stop l4env_start_stop l4xvfs minimal_io io simple_mem \
	  buddy_slab_mem basic_io                                        \
	  include

include $(L4DIR)/mk/subdir.mk

io: include basic_io
