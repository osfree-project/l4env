#! /bin/bash
#

# default enabled values
USE_SYMBOLS=y
USE_LINES=y

CONFIG_FILE=${0%/*}/config.local

if [ ! -f "$CONFIG_FILE" ]
then
  echo "I need a configuration!"
  echo "Copy config.template to config.local and modify to your configuration."
  exit 1
fi

set -e
#set -x

. $CONFIG_FILE

[ -z "$FIASCO" ] && FIASCO="fiasco"

if [ ! -d "$FIASCODIR" ];
then
  echo "$FIASCODIR doesn't exist or isn't a directory?!"
  exit 1
fi

if [ ! -e "$FIASCODIR/irq0" -o ! -e "$FIASCODIR/$FIASCO" ];
then
  echo "Your supplied FIASCODIR $FIASCODIR"
  echo "doesn't look like a Fiasco-UX build directory, it's missing"
  echo "at least the kernel image or irq0!"
  exit 1
fi

if [ -n "$USE_ROOTTASK" ];
then
  ROOTTASK=roottask
else
  ROOTTASK=rmgr-ux
fi

if [ -n "$USE_SYMBOLS" ];
then
  SYMBOLS_ARG="-symbols ${FIASCODIR}/Symbols"
  RMGR_ARGS="-symbols"
fi

if [ -n "$USE_LINES" ];
then
  LINES_ARG="-lines ${FIASCODIR}/Lines"
  RMGR_ARGS="$RMGR_ARGS -lines"
fi

if [ ! -e "$L4PATH/$ROOTTASK" -o ! -e "$L4PATH/sigma0-ux" ];
then
  echo "Your L4PATH $L4PATH"
  echo "doesn't look like an L4 binary directory, it's missing at least"
  echo "sigma0-ux!"
  exit 1
fi

[ -n "$UXMEM" ] && FIASCO="$FIASCO -m $UXMEM"
[ -n "$UXRES" ] || UXRES="800x600@16"

fiasco() {
  while [ -n "$1" ]; do
    case "$1" in
      -l)
         # drop -l
         shift

         # if the argument to -l is a relative path, set it relative to L4PATH
	 P[${#P[*]}]="-l"
	 if [ "${1#/}" = "$1" ];
	 then
	   P[${#P[*]}]="$L4PATH/$1"
	 else
	   P[${#P[*]}]="$1"
	 fi

         ;;
      *)
         # just append
	 P[${#P[*]}]="$1"
         ;;
    esac
    shift
  done

  echo $FIASCODIR/$FIASCO				\
    $SYMBOLS_ARG					\
    $LINES_ARG						\
    -rmgr       ${L4PATH}/$ROOTTASK" $RMGR_ARGS" 	\
    -sigma0     ${L4PATH}/sigma0-ux 			\
    -fb_program ${FIASCODIR}/ux_con			\
    -irq0	${FIASCODIR}/irq0			\
    "${P[@]}"

  [ -n "$RUN_GDB" ] && gdb_wrap="gdb $GDB_ARGS --args"

  $gdb_wrap $FIASCODIR/$FIASCO				\
    $SYMBOLS_ARG					\
    $LINES_ARG						\
    -rmgr       ${L4PATH}/$ROOTTASK" $RMGR_ARGS" 	\
    -sigma0     ${L4PATH}/sigma0-ux 			\
    -fb_program ${FIASCODIR}/ux_con			\
    -irq0	${FIASCODIR}/irq0			\
    "${P[@]}"
}

# vi:set ft=sh:
