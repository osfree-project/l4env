SYSTEMS	:= x86-l4v2
PKGDIR	?= ../..
L4DIR	?= $(PKGDIR)/../..

TARGET	 = libpredict.a

SRC_C	 = llsp_solver.c predict_mpeg.c predict_xvid.c

# we need to use the dietlibc headers to be able to use l4vfs
# for loading and storing the coefficients
LIBCINCDIR = $(LIBCINCDIR_x86_l4env_base)

include $(L4DIR)/mk/lib.mk
