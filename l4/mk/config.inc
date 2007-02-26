# -*- Makefile -*-
# DROPS (Dresden Realtime OPerating System) Component
#
# Makefile-Include for compiling templates (prog.mk, lib.mk)
#

# Makefile-Include for binary, lib, subdir and other directories.
# Definitions and rules for the DROPS configuration tool.

# Supported targets:
#
# config::      - run the menu-driven configuration tool
# txtconfig::	- run the configuration tool
# oldconfig::	- (re)create the configuration header based on a prior
#		  configuration or default values
#
#
# Required Parameters:
#
# PKGDIR
#
#
# Optional Parameters:
#
# DROPSCONF		- if nunempty, the configuration tool is run for
#			  target config::. If empty, the configuration tool
#			  is not run.
# DROPSCONF_TITLE	- the main title in the configuration tool.
# DROPSCONF_DEFCONFIG	- default config file
# DROPSCONF_CONFIG_IN	- configuration defintion file
# DROPSCONF_CONFIG	- config file
# DROPSCONF_CONFIG_H	- generated config header file
# DROPSCONF_MACRO	- macro to indicate inclusion of config header file
# DROPSCONF_HELPFILE	- options help file
# DROPSCONF_TOOL	- the menudriven configuration tool
# DROPSCONF_TOOL_TXT	- the configuration tool
# DROPSCONF_TOOL_OLD	- helper for recreating the config header file

DROPSCONF		?=
DROPSCONF_TITLE		?= DROPS Configuration Tool
DROPSCONF_DEFCONFIG	?= defconfig
DROPSCONF_CONFIG_IN	?= config.in
DROPSCONF_CONFIG	?= .config
DROPSCONF_CONFIG_H	?= config.h
DROPSCONF_MACRO		?= CONFIG_H_INCLUDED
DROPSCONF_HELPFILE	?= config.help
DROPSCONF_TOOL		?= $(firstword $(wildcard \
					$(L4DIR)/tool/config/Menuconfig \
					$(DROPS_STDDIR)/tool/bin/Menuconfig) \
					false)
DROPSCONF_TOOL_TXT	?= $(firstword $(wildcard \
					$(L4DIR)/tool/config/Configure \
					$(DROPS_STDDIR)/tool/bin/Configure) \
					false)
DROPSCONF_TOOL_OLD	?= $(firstword $(wildcard \
					$(L4DIR)/tool/config/Configure \
					$(DROPS_STDDIR)/tool/bin/Configure) \
					false) -d
export DROPSCONF_TITLE DROPSCONF_DEFCONFIG DROPSCONF_CONFIG_IN \
	DROPSCONF_CONFIG DROPSCONF_CONFIG_H DROPSCONF_MACRO \
	DROPSCONF_HELPFILE DROPSCONF_UNDEF

ifneq ($(DROPSCONF),)
.o:	$(DROPSCONF_CONFIG_H)

$(DROPSCONF_CONFIG_H) $(DROPSCONF_CONFIG):
	@echo "Create the configuration files"
	true | $(DROPSCONF_TOOL_OLD) || \
	  ( echo -e "\nError: Some defaults for config options are missing." ; \
	    false )

config::
	$(DROPSCONF_TOOL)

txtconfig::
	$(DROPSCONF_TOOL_TXT)

oldconfig::
	$(DROPSCONF_TOOL_OLD)

clean::
	$(VERBOSE)$(RM) $(DROPSCONF_CONFIG_H) .menuconfig.log $(DROPSCONF_CONFIG).old

cleanall:: clean
	$(VERBOSE)$(RM) $(DROPSCONF_CONFIG)

help::
	@echo "  config         - run the menu-driven configuration tool"
	@echo "  txtconfig      - run the configuration tool"
	@echo "  oldconfig      - (re)create the configuration header based on a prior"
	@echo "                   configuration or default values"

ifeq ($(filter config oldconfig txtconfig help scrub clean cleanall,$(MAKECMDGOALS)),)
-include $(DROPSCONF_CONFIG)
endif

# end of DROPSCONF defined
else
config txtconfig oldconfig::
endif

.PHONY: config oldconfig txtconfig


