L4DIR = ../../../..

obj-m := taskcache.o

LX_DIR = /home/doebel/src/l4-svn/l4linux-2.6
KDIR := $(LX_DIR)/build_van
PWD := $(shell pwd)

default:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules

clean cleanall :
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) clean
