PKGDIR	?= ../..
L4DIR	?= $(PKGDIR)/../..

# the default is to build the listed directories, provided that they
# contain a Makefile. If you need to change this, uncomment the following
# line and adapt it.
TARGET = util controller view model exec

include $(L4DIR)/mk/subdir.mk

model: util controller view
