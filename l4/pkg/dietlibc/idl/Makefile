PKGDIR		?= ..
L4DIR		?= $(PKGDIR)/../..

IDL		= l4xvfs.idl l4xexec.idl io.idl name_server.idl basic_name_server.idl \
		  attachable.idl name_space_provider.idl                  \
		  basic_io.idl net_io.idl connection.idl                  \
		  object_server.idl rw.idl

IDL_FLAGS	= -t -fforce-corba-alloc -fl4types #-ftrace-server=LOG
SYSTEMS		= x86-l4v2 x86-l4x0
#LIBCINCDIR	= -I/usr/include
#PRIVATE_INCDIR	= /usr/include
PRIVATE_INCDIR	= $(PKGDIR)/lib/dietlibc/include \
		  $(PKGDIR)/lib/contrib/dietlibc/include

include $(L4DIR)/mk/idl.mk
