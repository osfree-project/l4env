
ifneq ($(KERNELRELEASE),)

tis_oslo-objs:= tis_oslo_base.o tis.o
tis_oslo2-objs:= tis_oslo_base2.o tis.o
obj-m	:= tis_oslo.o tis_oslo2.o

else

KDIR	?= /usr/src/linux/
PWD	:= $(shell pwd)

default:
	$(REMOTE) $(MAKE) CC=$(CC) -C $(KDIR) SUBDIRS=$(PWD) modules
clean:
	$(REMOTE) $(MAKE) -C $(KDIR) SUBDIRS=$(PWD) clean
endif
