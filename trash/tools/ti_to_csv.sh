#!/bin/sh

PATH="$PATH:."
SUFFIX=""

sparser -l"$1" -f"msgtype=0x513,format=csv,file=0x513$SUFFIX.csv;msgtype=0x503,format=csv,file=0x503$SUFFIX.csv"
