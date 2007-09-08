
ifneq ($(KERNELRELEASE),)

inftpm-objs:= inftpm_base.o inftpm_init.o inftpm_io.o inftpm_tl.o
obj-m	:= inftpm.o

else

KDIR	:= ~/linux/src/
CC	:= /usr/local/gcc/3.3/bin/gcc
PWD	:= $(shell pwd)

default:
	$(REMOTE) $(MAKE) CC=$(CC) -C $(KDIR) SUBDIRS=$(PWD) modules
clean:
	$(REMOTE) $(MAKE) -C $(KDIR) SUBDIRS=$(PWD) clean
endif
