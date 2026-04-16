#!/usr/bin/env bash
WORKDIR=$(dirname $(realpath "$0"))
if [[ $# -ne 3 ]]; then
	echo "Usage: $0 <buffer_mb> <bit_window_us> <binary_message>"
	exit
fi
BUILDDIR="$WORKDIR/build"
mkdir -p "$BUILDDIR"
gcc -O0 -g -o "$BUILDDIR/cxl_covert_reader" "$WORKDIR/reader.c"
gcc -O0 -g -o "$BUILDDIR/cxl_covert_sender" "$WORKDIR/sender.c"

killall cxl_covert_reader
killall cxl_covert_sender

buffer_mb="$1"
bit_window_us="$2"
binary_message="$3"

numactl --membind=1 "$BUILDDIR/cxl_covert_sender" "$buffer_mb" "$bit_window_us" "$binary_message">/tmp/cxl_covert_sender.log &
numactl --membind=1 "$BUILDDIR/cxl_covert_reader" "$buffer_mb" "$bit_window_us" >/tmp/cxl_covert_reader.log &
