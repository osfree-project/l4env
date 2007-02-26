#! /bin/sh

SUBDIR="$1"
CONTRIB="../contrib/qt-embedded-free"
CONTRIB=`cd "${CONTRIB}" && pwd`
LINKLIST=".make-links_$SUBDIR"

export SUBDIR
export CONTRIB

# speed it up a bit: only recreate links if some are missing
[ -f "${LINKLIST}" ] && cat "${LINKLIST}" | sort > "${LINKLIST}.old" || echo "empty" > "${LINKLIST}.old"
find "${SUBDIR}" -type l | sort > "${LINKLIST}"
diff -q "${LINKLIST}" "${LINKLIST}.old" > /dev/null && { rm -f "${LINKLIST}.old"; exit 0; }

# link list files differ, so recreate everything
# setup directories
find "${CONTRIB}" -path '*CVS' -prune -o \( -type d -a -printf "%P\n" \) | \
    xargs mkdir -p

# remove all links in SUBDIR first
find "${SUBDIR}" -type l | xargs rm -f

# update links
find "${CONTRIB}/${SUBDIR}" -path '*CVS' -prune -o \( ! -type d -a -printf "%P\n" \) | \
    (
	cd "${SUBDIR}"
        read a
        while [ ! -z "$a" ]
        do	
            [ -f "$a" ] || ln -s "${CONTRIB}/${SUBDIR}/$a" "$a"
            read a
        done
    )

# create new list of links after recreation
find "${SUBDIR}" -type l | sort > "${LINKLIST}"
rm -f "${LINKLIST}.old"

exit 0

