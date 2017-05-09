#!/bin/sh

#-------------------------------------------------------------------------------
# Script for MIX emulation. Sort messages by files for viw in DTX.
#-------------------------------------------------------------------------------

# mix.sh [directory]

SLEEP_TIME=2
FILES_DIR="ftproot"

_MONTH=$(date +%m|sed 's/^0//')
_DAY=$(date +%e|sed 's/^ //')
_HOUR=$(date +%H)

MSG_FMT="raw"

D_MDH="$_MONTH$_DAY$_HOUR"
D_DM="$_DAY$_MONTH"

P_NAME="$(basename $0)"

# Ports
P_COMMON="7001"
P_RCH="7004"
#-------------------------------------------------------------------------------
usage()
# $1 - program name.
{
   echo "$1 [-p <common port>] [-r <RCH port>] [-f {prn|csw|raw}] [-h] [directory]"
}
#-------------------------------------------------------------------------------
mk_filter()
# Make fiter string.
# $1 - Message type, $2 - message file, $3 - message format.
# Print string to the terminal.
# Return always 0.
{
   echo "msgtype=$1,file=$2,format=$3"
}

#-------------------------------------------------------------------------------
# Message filters.
#-------------------------------------------------------------------------------

# 0x0523
F_DEF=$(mk_filter "0x0523" "def.$D_MDH" "$MSG_FMT")
# ?
F_DNC="dnc.$D_MDH"
# 0x0475 (headers)
F_DNCH=$(mk_filter "0x0475" "dnch.$D_MDH" "$MSG_FMT")
# 0x0504
F_MODE=$(mk_filter "0x0504" "mode.$D_MDH" "$MSG_FMT")
# 0x0473 (port 7004)
F_RCH=$(mk_filter "0x0473" "rch.$D_MDH" "$MSG_FMT")
# 0x0502
F_RT=$(mk_filter "0x0502" "rt.$D_MDH" "$MSG_FMT")
# ?
F_SHN=$(mk_filter "0" "shn.$D_MDH")
# 0x0501, 0x0511, 0x0512
F_TS=$(mk_filter "0x0501" "tc.$D_MDH" "$MSG_FMT")
F_TS="$F_TS;$(mk_filter 0x0511 "tc.$D_MDH" $MSG_FMT)"
F_TS="$F_TS;$(mk_filter 0x0512 "tc.$D_MDH" $MSG_FMT)"
# 0x0513,0x0503
F_TI=$(mk_filter "0x0513" "ti.$D_DM" "$MSG_FMT")
F_TI="$F_TI;$(mk_filter 0x0503 "ti.$D_DM" $MSG_FMT)"
# 0x04F6
F_UDP=$(mk_filter "0x04f6" "udp.$D_MDH" "$MSG_FMT")
# 0x0505
F_UT=$(mk_filter "0x0505" "ut.$D_MDH" "$MSG_FMT")
#-------------------------------------------------------------------------------
add_dir()
# Add directory to the filter string.
# $1 - filter string.
# Print corrected string to the terminal.
# Return status of the string parser.
{
   echo "$1"|sed "s&\(file=\)\([^,;]*\)&\1$FILES_DIR/\2&g"
}

#-------------------------------------------------------------------------------
# Main.
#-------------------------------------------------------------------------------

eval set -- "$(getopt -n $P_NAME -o"p:r:f:h" -- $@)"
if [ $? != 0 ] ; then echo "Terminating..." >&2 ; exit 1 ; fi

while true; do
   case $1 in
      -p)
         [ -z "$2" ] && { usage "$P_NAME"; exit 1; }
         P_COMMON="$2"
         shift 2
      ;;
      -r)
         [ -z "$2" ] && { usage "$P_NAME"; exit 1; }
         P_RCH="$2"
         shift 2
      ;;
      -f)
         [ -z "$2" ] && { usage "$P_NAME"; exit 1; }
         MSG_FMT="$2"
         shift 2
      ;;
      -h)
         usage "$P_NAME";
         exit 0
      ;;
      --)
         shift;
         break;
      ;;
      *)
         usage "$P_NAME";
         exit 1
      ;;
   esac
done
#exit

FILES_DIR=${1:-$FILES_DIR}

if [ ! -d "$FILES_DIR" ]; then
   mkdir -p "$FILES_DIR" || exit 1
else
#   echo "Files in directory \"$FILES_DIR\" will be overwritten!"
   : ;
fi

FLT_STR="$F_DEF;$F_DNCH;$F_MODE;$F_RT;$F_TS;$F_TI;$F_UDP;$F_UT"
PATH="$PATH:."

F_RCH=$(add_dir "$F_RCH")
FLT_STR=$(add_dir "$FLT_STR")

FILES=$(echo "$FLT_STR;$F_RCH"| awk 'BEGIN {RS = ";"; FS = ","; }
   {
      for (i = 1; (i <= NF) && ($i !~ /file=.*/); i++);
      if (i <= NF)
      {
         split($i, f, "=");
         print "\""f[2]"\"";
      }
   }' | uniq)

#sed 's/[^;,]*,file=\([^;,]*\).*/\1/'

echo "========================================================================="
echo "MIX emulator"
echo "========================================================================="
echo -e "Port for messages:\t\t\t$P_COMMON\nPort for message type 0x0473:\t\t$P_RCH"
echo -e "Folder for message files:\t\t$FILES_DIR"
echo -e "Output format:\t\t\t\t$MSG_FMT"
echo "========================================================================="
echo -e "Decoding started...\nPress Ctrl+C for exit."

(sparser -f"$FLT_STR" -n"port=$P_COMMON" >/dev/null)&
#[ $? -ne 0 ] && { echo "Common parser loading error!" >&2; exit 1; }

PID_S0=$!

(sparser -f"$F_RCH" -n"port=$P_RCH" >/dev/null)&
#[ $? -ne 0 ] && { echo "Parser for 0x0473 loading error!" >&2; kill $PID_S0; exit 1; }
PID_S1=$!

trap "{ kill -9 $PID_S0 $PID_S1; exit 0; }" SIGINT

while true; do
   PS_LIST="$(ps -Ao pid)"
   sleep $SLEEP_TIME;
   echo $PS_LIST|grep -q "$PID_S0" || { echo "Common parser was down!" >&2; kill -9 $PID_S1; exit 1; }
   echo $PS_LIST|grep -q "$PID_S1" || { echo "Parser for 0x0473 was down!" >&2; kill -9 $PID_S0; exit 1; }
   echo -e "Decoding in process...\nMessage files:"
   eval du -h $FILES
   echo "-------------------------------------------------------------------------"
done;
#-------------------------------------------------------------------------------

