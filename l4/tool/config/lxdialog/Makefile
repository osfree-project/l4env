L4DIR ?= ../../..
include $(L4DIR)/mk/Makeconf

HOSTCFLAGS += -DLOCALE -MD
LIBS = -lncurses

ifeq (/usr/include/ncurses/ncurses.h, $(wildcard /usr/include/ncurses/ncurses.h))
        HOSTCFLAGS += -I/usr/include/ncurses -DCURSES_LOC="<ncurses.h>"
else
ifeq (/usr/include/ncurses/curses.h, $(wildcard /usr/include/ncurses/curses.h))
        HOSTCFLAGS += -I/usr/include/ncurses -DCURSES_LOC="<ncurses/curses.h>"
else
ifeq (/usr/include/ncurses.h, $(wildcard /usr/include/ncurses.h))
        HOSTCFLAGS += -DCURSES_LOC="<ncurses.h>"
else
	HOSTCFLAGS += -DCURSES_LOC="<curses.h>"
endif
endif
endif


OBJS = $(patsubst %.c,$(OBJ_DIR)/%.o,$(wildcard *.c))

all:: ncurses $(OBJ_DIR)/lxdialog

$(OBJ_DIR)/%.o: %.c
	$(HOST_CC) $(HOSTCFLAGS) -c -o $@ $<

$(OBJ_DIR)/lxdialog: $(OBJS)
	$(HOST_CC) -o $@ $(OBJS) $(LIBS)

ncurses:
	$(VERBOSE)echo "main() {}" > lxtemp.c
	$(VERBOSE)if $(HOST_CC) -lncurses lxtemp.c ; then \
		rm -f lxtemp.c a.out; \
	else \
		rm -f lxtemp.c; \
		echo -e "\007" ;\
		echo ">> Unable to find the Ncurses libraries." ;\
		echo ">>" ;\
		echo ">> You must have Ncurses installed in order" ;\
		echo ">> to use 'make menuconfig'" ;\
		echo ;\
		exit 1 ;\
	fi

clean:
	rm -f core $(OBJ_DIR)/*.o $(OBJ_DIR)/lxdialog

cleanall: clean
	rm -rf $(OBJ_DIR)/*.d

-include $(OBJS:.o=.d)
