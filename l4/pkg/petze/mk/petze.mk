
#
# Introduce an olle Petze into the program
#

DEFINES += -include stdlib.h -include l4/petze/petze.h
LIBS    += -lpetze


#
# Generate a DEFINE_<srcfile> = -D'PETZE_POOLNAME="<srcfile>"' for every
# file specified in $(SRC)
#

$(foreach pfn,$(SRC_C),$(eval $(shell echo "DEFINES_$(pfn) = -D'PETZE_POOLNAME=\"$(pfn)\"'")))

