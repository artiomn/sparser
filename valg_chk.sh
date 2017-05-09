#!/bin/sh

[ -z "$1" ] && { echo "$(basename $0) <program>"; exit 1; }

PRG="./$(basename $1)"
PRG_DIR="$(dirname $1)"

shift

#VG_LOG=$2
VG_LOG=${VG_LOG:-"checking/vglog.log"}

cd $PRG_DIR
valgrind --log-file="../$VG_LOG" --leak-check=full --show-reachable=yes --free-fill=0 \
   --undef-value-errors=yes --read-var-info=yes --verbose \
   --malloc-fill=0xff --track-origins=yes "$PRG" $@
