#! /bin/bash
#

CONFIG_FILE=${0%/*}/config.local

if [ -z "$CONFIG_FILE" ];
then
  echo "I need a configuration!"
  echo "Copy config.template to config.local and modify to your configuration."
  exit 1
fi

set -e
#set -x

. $CONFIG_FILE

if [ ! -d "$FIASCODIR" ];
then
  echo "$FIASCODIR doesn't exist or isn't a directory!"
  exit 1
fi

if [ ! -f "$FIASCODIR/kernel.image" -o ! -f "$FIASCODIR/irq0" -o \
     ! -e "$FIASCODIR/fiasco" ];
then
  echo "Your supplied FIASCODIR $FIASCODIR"
  echo "doesn't look like a Fiasco/UX build directory, it's missing"
  echo "at least the kernel image or irq0!"
  exit 1
fi

if [ ! -e "$L4PATH/rmgr-ux" -o ! "$L4PATH/sigma0-ux" ];
then
  echo "Your L4PATH $L4PATH"
  echo "doesn't look like an L4 binary directory, it's missing at least"
  echo "sigma0-ux or rmgr-ux!"
  exit 1
fi

for p in rmgr-ux sigma0-ux; do
  if [ ! -e "$FIASCODIR/$p" ];
  then
    echo "$p is missing in the Fiasco/UX directory ($FIASCODIR)"
    echo "Linking it in..."
    ( cd $FIASCODIR && ln -s $L4PATH/$p )
  fi
done

[ -z "$FIASCO" ] && FIASCO="$FIASCODIR/fiasco"

# generate a tftpboot-path from L4PATH (highly specific)
# L4PATH needs to be of the form /home/user/boot/blah...
BOOTDIR=${L4PATH/\/home\//\/tftpboot\/}
BOOTDIR=${BOOTDIR/\/boot\///}
if [ "$BOOTDIR" = "$L4PATH" ];
then
  BOOTDIR="/UNDEFINED"
else
  BOOTDIR="(nd)$BOOTDIR"
fi

# chdir to FIASCO directory so that we find rmgr-ux, sigma0-ux and the
# other tools Fiasco/UX expects in its working directory
cd ${FIASCODIR}

[ -n "$UXMEM" ] && FIASCO="$FIASCO -m $UXMEM"
[ -n "$UXRES" ] || UXRES="800x600@16"

fiasco() {
  while [ -n "$1" ]; do
    case "$1" in
      -l)
         # drop -l
         shift

         # if argument to -l is relative path, set it relative to L4PATH
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

  if [ -n "$LOGFILE" ];
  then
    # create dir to logfile if it's non-existent
    mkdir -p ${LOGFILE%/*}

    $FIASCO "${P[@]}" 2>$LOGFILE
  else
    $FIASCO "${P[@]}"
  fi

  # maybe useful
  #killall -9 fiasco || true
}

# vi:set ft=sh:
