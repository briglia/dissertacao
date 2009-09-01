#!/bin/bash
# Author: Anderson Briglia <anderson.briglia@gmail.com>
# Based on compcache project official repository
# This script is part of Briglia's master work.
#
# unuse_compcache (run as *root*)
# - Unloads compcache and related modules.
# - Turns off swap device.

LSMOD_BIN=/sbin/lsmod
RMMOD_BIN=/sbin/rmmod
SWAPOFF_BIN=/sbin/swapoff

EXIST=`$LSMOD_BIN | grep compcache`
if [ "$EXIST" = "" ]; then
	echo "compcache module not loaded"
	exit 0
fi

EXIST=`cat /proc/swaps | grep ramzswap`
if [ "$EXIST" != "" ]; then
	echo "Turning off compache swap device ..."
	$SWAPOFF_BIN /dev/ramzswap0
fi

echo "Unloading modules ..."
$RMMOD_BIN compcache
$RMMOD_BIN xvmalloc
echo "Done!"
