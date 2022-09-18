#! /bin/sh
#
#	@(#)ranlib.sh	4.1	(ULTRIX)	7/17/90
#
#  simulate "ranlib" with mips ar ts
#  (mainly for makefile compatibility)
#
PATH=/bin:/usr/bin
Myname=`basename "$0"`
case "$#" in
	0)
		echo "$Myname: usage : $Myname filename..."
		exit 1
		;;
esac

for afile in "$@"
{
    ar ts "$afile" >/dev/null
}
