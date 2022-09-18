#!/bin/sh
# @(#)lp.sh	4.1      ULTRIX	7/2/90
#
# 	SysV lp program script (X/OPEN Portability)
#
#	chetal	1/19/88
#
PATH=/usr/ucb:/bin:/usr/bin
C=-s		#Default..symlink
F=		#Holds the files
next="F=$F@"

for i in $@
do 
	case $i in
	  -c)	C=
		;;
	  -d)   next="D=-P"
		;;
	  -d*) 	D=`expr $i : '-d\(.*\)'`
		D=-P$D
		;;
	  -n)	next="N=-#"
		;;
	  -n*)	N=`expr $i : '-n\(.*\)'`
		N=-#$N
		;;
	   -)	eval $next"$i"
		next="F=$F@"
		;;
	  -*)	echo lp: Invalid option switch $i 1>&2
		echo usage: lp [ -c ] [ -d dest ] [-n num ] [ - ] [ file... ]
		exit 1
		;;
	   *)	eval $next"$i"
		next="F=$F@"
		;;
	  esac
done
IFS="@"

exec lpr $C $D $N $F
