# -*- Makefile -*-
# ============================================================================
#
# Do not edit this file manually, it is generated automatically by
# the configure script
#
# See ./configure --help
#
# ============================================================================

PKGDIR ?= ../../../../..
L4DIR  ?= $(PKGDIR)/../..
include $(L4DIR)/mk/Makeconf
-include $(PKGDIR_OBJ)/Makeconf.bid.local
include $(L4DIR)/mk/modes.inc

# ============================================================================
# Activated features
# ============================================================================

FEATURES=

# ============================================================================
# Architecture dependant things
# ============================================================================

ARCHITECTURE=-DARCH_IS_IA32
BUS=-DARCH_IS_32BIT
ENDIANNESS=-DARCH_IS_LITTLE_ENDIAN
SHARED_EXTENSION=so
STATIC_EXTENSION=a
OBJECT_EXTENSION=o

# ============================================================================
# Compiler
# ============================================================================

SPECIFIC_CFLAGS=-fPIC 
# CFLAGS=-Wall -O2 -fstrength-reduce -finline-functions -freduce-all-givs -ffast-math -fomit-frame-pointer $(LIBCINCDIR_x86_tiny)
CFLAGS=-Wall -O2 -fstrength-reduce -finline-functions -ffast-math -fomit-frame-pointer $(LIBCINCDIR_x86_tiny)

# ============================================================================
# Assembler
# ============================================================================

AS=nasm
AFLAGS=-I$(<D)/ -f elf 
ASSEMBLY_EXTENSION=asm

# ============================================================================
# Linker
# ============================================================================

SPECIFIC_LDFLAGS=-Wl,-soname,libxvidcore.$(SHARED_EXTENSION).$(API_MAJOR) -shared -lc -lm 
API_MAJOR=4
API_MINOR=0
RANLIB=ranlib

# ============================================================================
# Installation
# ============================================================================

INSTALL=/usr/bin/install -c
prefix=/usr/local
exec_prefix=${prefix}
libdir=${exec_prefix}/lib
includedir=${prefix}/include

# ============================================================================
# Sources
# ============================================================================

GENERIC_SOURCES=$(SRC_GENERIC)
ASSEMBLY_SOURCES=$(SRC_IA32)
DCT_IA64_SOURCES=$()
GENERIC_OBJECTS=$(SRC_GENERIC:.c=.o)
ASSEMBLY_OBJECTS=$(SRC_IA32:.asm=.o)
DCT_IA64_OBJECTS=$(:.asm=.o)

STATIC_LIB=libxvidcore.$(STATIC_EXTENSION)
SHARED_LIB=libxvidcore.$(SHARED_EXTENSION).$(API_MAJOR).$(API_MINOR)
PRE_SHARED_LIB=libxvidcore.$(SHARED_EXTENSION).$(API_MAJOR).$(API_MINOR)
