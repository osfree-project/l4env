#!/bin/sh

# ################################################################
TEMPLATE="$1.template"
QMAKECONF="$1"
CC="$2"
CXX="$3"
L4INCDIRS="$4"

SEDSCRIPT=qmake-conf.sed

# ################################################################
cat > "$SEDSCRIPT" <<EOF
# This file is autogenerated by make-qmake-conf.sh. Do not edit it.

s�@CC@�$CC�g
s�@CXX@�$CXX�g
s�@L4INCDIRS@�$L4INCDIRS�g
EOF

# ################################################################

sed -f "$SEDSCRIPT" "$TEMPLATE" > "$QMAKECONF"
rm -f "$SEDSCRIPT"
