L4DIR	?= ../../..

SINGLES  = simple_idl 
SINGLES += arrays
SINGLES += structs
SINGLES += corba_idl
SINGLES += function_id
SINGLES += strings
SINGLES += ping
SINGLES += andy
SINGLES += pagefault
SINGLES += sort1

COUPLES	 = l4str
COUPLES += l4union
COUPLES += fpage1
COUPLES += fpage2
COUPLES += init-rcvstring

OTHER	 = sockets

# name sockets here to avoid testing with fisco-ux
TARGET	= $(SINGLES) $(COUPLES) $(OTHER)

include $(L4DIR)/mk/subdir.mk

test::

-include Makeconf.local

