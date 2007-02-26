TARGET_A 	:= $(TARGET).a
TARGET_E_A	:= $(TARGET)_e.a

CXXFLAGS_F_A	      := $(patsubst %.cc,%.cxxflags,$(SRC_CC))
CXXFLAGS_F_E_A	      := $(patsubst %.cc,%.e.cxxflags,$(SRC_CC))

SRC_CC_$(TARGET_A)    := $(SRC_CC)
SRC_CC_$(TARGET_E_A)  := $(patsubst %.cc,%.e.cc,$(SRC_CC))
SRC_CC 		      :=

PRIVATE_INCDIR	+= ../

TARGET := $(TARGET_A) $(TARGET_E_A)

ifneq ($(MAKECMDGOALS),)
  MAKEGOALS	:= $(MAKECMDGOALS)
else
  MAKEGOALS	:= all
endif

ifeq ($(findstring OBJ,$(shell pwd)),OBJ)
ifneq ($(filter-out clean cleanall scrub,$(MAKEGOALS)),)
  -include $(CXXFLAGS_F_A) $(CXXFLAGS_F_E_A)
endif
endif

all::

$(CXXFLAGS_F_A): %.cxxflags: %.cc
	$(VERBOSE)echo "CXXFLAGS_$(notdir $<) := -Wall -fno-exceptions -fno-rtti" > $@

$(CXXFLAGS_F_E_A): %.cxxflags: %.cc
	$(VERBOSE)echo "CXXFLAGS_$(notdir $<) := -Wall" > $@

%.e.cc: %.cc
	$(VERBOSE)ln -s $< $@
