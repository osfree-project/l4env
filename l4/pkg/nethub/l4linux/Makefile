CP:=cp
PWD:=$(shell pwd)

all:
	@echo "Usage:"
	@echo "  patching l4linux:   make L4LX=<patho/to/l4linux> patch"
	@echo "  build flips driver: make flips-drv"

patch: l4lx-2.4.28.diff
	$(CP) l4tundrv/linux/if_l4tun.h $(L4LX)/include/linux
	$(CP) l4tundrv/l4tun.c $(L4LX)/drivers/net
	patch -d $(L4LX) -p0 <$<

flips-drv: l4tundrv flips-control

l4tundrv:
	$(MAKE) -C l4tundrv

flips-control:
	$(MAKE) -C flips-control

clean::
	$(MAKE) -C l4tundrv clean

cleanall::
	$(MAKE) -C l4tundrv cleanall
	
.PHONY: l4tundrv flips-control
