#!/bin/bash
# A simple shell script to setup and perform some IO testing on a
# lightnvm block IO device. For this code we follow the instructions
# laid out at:
#
#   https://github.com/OpenChannelSSD/linux/wiki
# 

DEVICE=nullb0
TARGET=sanity
TYPE=rrpc
START=0
END=0

IOENGINE=libaio
NUM_JOBS=1
SIZE=1G
IO_DEPTH=1
BLOCK_SIZE=4k
RUNTIME=60
FIOEXE=fio
FORCE=

  # Accept some key parameter changes from the command line.
while getopts "x:d:r:b:n:t:i:s:e:f" opt; do
    case "$opt" in
    x)  FIOEXE=${OPTARG}
            ;;
    d)  DEVICE=${OPTARG}
            ;;
    r)  RUNTIME=${OPTARG}
            ;;
    b)  BLOCK_SIZE=${OPTARG}
            ;;
    n)  NUM_JOBS=${OPTARG}
            ;;
    t)  TARGET=${OPTARG}
            ;;
    i)  IO_DEPTH=${OPTARG}
            ;;
    s)  SIZE=${OPTARG}
            ;;
    e)  IOENGINE=${OPTARG}
            ;;
    f)  FORCE=1
            ;;
    \?)
        echo "Invalid option: -$OPTARG" >&2
        exit 1
        ;;
    :)
        echo "Option -$OPTARG requires an argument." >&2
        exit 1
        ;;
    esac
done
FILENAME=/dev/${TARGET}
CONFIG=/sys/block/${DEVICE}/nvm/configure

  # Perform some error checking
if [ ! -e "$CONFIG" ]; then
     echo "sanity.sh: $CONFIG does not exist!"
     exit 1
fi

if [ ! -e "$FILENAME" ]; then
    echo "$TYPE $TARGET $START:$END" > ${CONFIG}
else
    if [ -z "$FORCE" ]; then
	echo "sanity.sh $FILENAME already exists, use -f to force test."
	exit 1
    else
	echo "sanity.sh: Forcing run on an existing block device."
    fi
fi

FILENAME=${FILENAME} SIZE=${SIZE} NUM_JOBS=${NUM_JOBS} IO_DEPTH=${IO_DEPTH} \
    BLOCK_SIZE=${BLOCK_SIZE} RUNTIME=${RUNTIME} IOENGINE=${IOENGINE} \
    ${FIOEXE} ./sanity.fio
