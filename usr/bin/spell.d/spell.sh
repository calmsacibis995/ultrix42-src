#! /bin/sh
#@(#)spell.sh	4.1	(ULTRIX) 7/17/90

#************************************************************************
#									*
#			Copyright (c) 1984 by				*
#		Digital Equipment Corporation, Maynard, MA		*
#			All rights reserved.				*
#									*
#   This software is furnished under a license and may be used and	*
#   copied  only  in accordance with the terms of such license and	*
#   with the  inclusion  of  the  above  copyright  notice.   This	*
#   software  or  any  other copies thereof may not be provided or	*
#   otherwise made available to any other person.  No title to and	*
#   ownership of the software is hereby transferred.			*
#									*
#   This software is  derived  from  software  received  from  the	*
#   University    of   California,   Berkeley,   and   from   Bell	*
#   Laboratories.  Use, duplication, or disclosure is  subject  to	*
#   restrictions  under  license  agreements  with  University  of	*
#   California and with AT&T.						*
#									*
#   The information in this software is subject to change  without	*
#   notice  and should not be construed as a commitment by Digital	*
#   Equipment Corporation.						*
#									*
#   Digital assumes no responsibility for the use  or  reliability	*
#   of its software on equipment which is not supplied by Digital.	*
#									*
#***********************************************************************/
#
#	Modified By :	Aki Hirai	Digital Equipment Corp.
#			31 -May -1985
#			Add interrupt signal handling
#	
#			Pradeep Chetal
#			Jan 26, 1988
#			Added {+user_spellfile} option & stdin acceptance
#			
#			Pradeep Chetal
#			Feb 8, 1989
#			If user specified a non-existing file (except
#			for -h option) or omitted a file name,
#			exit with proper message
#************************************************************************/
#   
#	@(#)spell.sh	1.3	(Berkeley)	83/09/10
#
: V data for -v, B flags, D dictionary, S stop, H history, F files, T temp
: U user spell file,
PATH=/usr/ucb:/bin:/usr/bin
V=/dev/null		B=			F= 
S=/usr/dict/hstop	H=/dev/null		T=/tmp/spell.$$
U=
next="F=$F@"
D=/usr/dict/hlista	#American by default
trap "rm -f $T ${T}a ${T}x ${T}+; exit" 0 2
while test $# -gt 0
do
	case $1 in
	-v)	B="$B@-v"
		V=${T}a ;;
	-x)	B="$B@-x" ;;
	-b) 	D=/usr/dict/hlistb
		B="$B@-b" ;;
	-d)	next="D="
		shift
		if test x = "$1"x
		then
			echo "No user directory specified, exiting"
			echo "Usage:  spell [ -v ] [ -b ] [ -x ]  [ +local_file ] [ -d hlist ] [ -s hstop ] [ -h spellhist ] [file...]"			
			exit
		fi
		if test ! -r "$1"
		then
			echo "$1: No such file or directory"
			exit
		fi
		eval $next"$1"
		next="F=$F@"
		;;
	-s)	next="S="
		shift
		if test x = "$1"x
		then
			echo "No stop list specified, exiting"
			echo "Usage:  spell [ -v ] [ -b ] [ -x ]  [ +local_file ] [ -d hlist ] [ -s hstop ] [ -h spellhist ] [file...]"			
			exit
		fi
		if test ! -r "$1"
		then
			echo "$1: No such file or directory"
			exit
		fi
		eval $next"$1"
		next="F=$F@"
		;;
	-h)	next="H="
#history file can be new, so don't do a test -r as above
		shift
		if test x = "$1"x
		then
			echo "No history file specified, exiting"
			echo "Usage:  spell [ -v ] [ -b ] [ -x ]  [ +local_file ] [ -d hlist ] [ -s hstop ] [ -h spellhist ] [file...]"			
			exit
		fi
		eval $next"$1"
		next="F=$F@"
		;;
	-*)	echo "Bad flag for spell: $1"
		echo "Usage:  spell [ -v ] [ -b ] [ -x ]  [ +local_file ] [ -d hlist ] [ -s hstop ] [ -h spellhist ] [file...]"
		exit ;;
	+*)	U=`expr $1 : '+\(.*\)'`
		if test ! -r "$U"
		then
			echo "$U: No such file or directory"
			echo "Standard spell dictionary will be used."
			U=
		fi
		;;
	*)	if test ! -r "$1"
		then
			echo "$1: No such file or directory"
			exit
		fi
		eval $next"$1"
		next="F=$F@"
		;;
	esac
	shift
done
# Update "D" after all flags are read, if User_Spellfile is there
if test $U
then
	spellin $D <$U >${T}+
	D=${T}+
fi
IFS=@
case $H in
/dev/null)	
	deroff -w $F | sort -u | /usr/lib/spell $S $T |
	/usr/lib/spell $D $V $B > ${T}x ; cat ${T}x |
	sort -u +0f +0 - $T ;;

*)	deroff -w $F | sort -u | /usr/lib/spell $S $T |
	/usr/lib/spell $D $V $B > ${T}x ; cat ${T}x |
	sort -u +0f +0 - $T | tee -a $H
	who am i >> $H 2> /dev/null ;;
esac
case $V in
/dev/null)	exit ;;
esac
sed '/^\./d' $V | sort -u +1f +0
