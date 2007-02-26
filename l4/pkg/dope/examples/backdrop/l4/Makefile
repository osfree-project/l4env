SYSTEMS         := x86-l4v2
PKGDIR          ?= ../../..
L4DIR           ?= $(PKGDIR)/../..

TARGET           = backdrop
LIBS             = -ll4dope -lvscreen -ll4png_wrap -ll4png -ll4z
DEFAULT_RELOC    = 0x00c40000

vpath % $(PKGDIR)/examples/backdrop/l4

-include Makeconf.local

SRC_C            = backdrop.c
OBJS            += imagedata.o
PICTURE         ?= background.png

include $(L4DIR)/mk/prog.mk

imagedata.png: $(PICTURE)
	ln -sf $^ $@

%.o: %.png
	ld -r --oformat default -o $@ -b binary $^

