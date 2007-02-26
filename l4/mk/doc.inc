# -*- Makefile -*-
#
# DROPS (Dresden Realtime OPerating System) Component
#
# Makefile-include to extract the project name from a Doxygen file
#
# We use make to browse the doxygen file. This "makefile" prints variables.

print:
	@echo -n $($(VAR))

.PHONY: print
