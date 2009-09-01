#!/bin/bash
#
# use_compcache (run as *root*)
# - Loads compcache and related modules.
# - Sets up swap device.
#
# Usage: use_compcache.sh <size in KB>
#

MOD_PARAM=""
SIZE_KB="$1"

LSMOD_BIN=/sbin/lsmod
INSMOD_BIN=/sbin/insmod
MODPROBE_BIN=/sbin/modprobe
SWAPON_BIN=/sbin/swapon
UDEVADM_BIN=/sbin/udevadm

INSMOD()
{
	MOD_NAME="$1"
	PARAMS="$2"
	EXIST=`$LSMOD_BIN | grep "$MOD_NAME"`
	if [ "$EXIST" != "" ]; then
		echo "Module: $MOD_NAME already loaded."
	else
		if [ ! -f $MOD_NAME.ko ]; then
			echo "Module: $MOD_NAME not found in current directory"
		else
			$INSMOD_BIN $MOD_NAME.ko "$PARAMS"
		fi
	fi
}

MEM_SIZE_KB=`grep MemTotal: /proc/meminfo  | sed s/[^0-9]//g`

if [ -z "$SIZE_KB" ]; then
	echo "compcache size not given. Using default (25% of RAM)."
	SIZE_KB=$((MEM_SIZE_KB / 4))
fi

echo "Setting compcache size to ~$((SIZE_KB / 1024)) MB ..."

if [ $(($MEM_SIZE_KB*2)) -lt $SIZE_KB ]
then 
	echo "There is little point creating a compcache of greater than twice"
	echo "the size of memory since we expect a 2:1 compression ratio"
	echo "Note that compcache uses about 0.1% of the size of the swap"
	echo "device when not in use so a huge compcache is wasteful."
	echo
	echo "Memory Size: $MEM_SIZE_KB kB"
	echo "Size you selected: $SIZE_KB kB"
	if [ $(($MEM_SIZE_KB * 8)) -lt $SIZE_KB ]
	then
		echo
		echo "This is over 8 times the size of memory."
		echo "You have probably addded too many zeros."
		echo "Aborting."
	else 
		echo
		echo "Loading compcache anyway."
	fi
	exit 1
fi

# Some distos already have LZO de/compress modules
echo "Loading modules ..."
$MODPROBE_BIN -q lzo_compress  || INSMOD lzo1x_compress
$MODPROBE_BIN -q lzo_decompress || INSMOD lzo1x_decompress
INSMOD xvmalloc

MOD_PARAM_COMPCACHE="compcache_size_kbytes=$SIZE_KB"
INSMOD compcache "$MOD_PARAM_COMPCACHE"

# /dev/ramzswap0 is not available immediately after insmod returns
# So, let udev complete its work before we do swapon
if [ -f "$UDEVADM_BIN" ]; then
	$UDEVADM_BIN settle
else
	sleep 2
fi

# Set it as swap device with highest priority
EXIST=`cat /proc/swaps | grep compcache`
if [ "$EXIST" = "" ]; then
	echo "Setting up swap device ..."
	$SWAPON_BIN /dev/ramzswap0
	if [ "$?" = "0" ]; then
		echo "Done!"
	else
		echo "Could not add compcache swap device."
	fi
else
	echo "compcache swap device already active."
fi

exit 0

