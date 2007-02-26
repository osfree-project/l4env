L4DIR	?= ../../..

#SINGLES	= simple_idl
SINGLES = simple_idl arrays structs \
	  corba_idl function_id strings ping andy \
	  pagefault sort1
COUPLES	= init-rcvstring l4str l4union fpage1 fpage2

# name sockets here to avoid testing with fisco-ux
TARGET	= $(SINGLES) $(COUPLES) sockets

include $(L4DIR)/mk/subdir.mk

test::

-include Makeconf.local

