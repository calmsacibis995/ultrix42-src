#!/bin/sh
# @(#)svcsetup.sh	4.1	(ULTRIX)	7/2/90
#									
# 			Copyright (c) 1989 by
# 		Digital Equipment Corporation, Maynard, MA		
# 			All rights reserved.				
# 									
#    This software is furnished under a license and may be used and	
#    copied  only  in accordance with the terms of such license and	
#    with the  inclusion  of  the  above  copyright  notice.   This	
#    software  or  any  other copies thereof may not be provided or	
#    otherwise made available to any other person.  No title to and	
#    ownership of the software is hereby transferred.			
# 									
#    The information in this software is subject to change  without	
#    notice  and should not be construed as a commitment by Digital	
#    Equipment Corporation.						
# 									
#    Digital assumes no responsibility for the use  or  reliability	
#    of its software on equipment which is not supplied by Digital.	
#
# Purpose:	Set up /etc/svc.conf
# Usage:	svcsetup
# Environment:	Bourne shell script
# 
# Remarks:
#    Sets up files:
#	/etc/svc.conf
#
# Modification History:
#
# 11-Dec-89	sue
#	Added silent client mode specifically for dms.  Allows a
#	diskless manager to give a client a default svc.conf file of
#	either "local", "local,yp", or "local,bind" for all databases.
#
# 24-Jul-89	logcher
#	Created.
#

# files
case $1 in
DEBUG)
	shift
	DEBUG=1
	SVC=/tmp/svc.conf
	RCFILE=/tmp/rc.local
echo "Running in DEBUG mode ...
"
	;;
*)
	SVC=/etc/svc.conf
	RCFILE=/etc/rc.local
	;;
esac
#
# Other declarations
#
umask 022
NULL=/dev/null
SVCTMP=/tmp/svc.$$
DBTMP=/tmp/svc_db.$$
ALL="0 1 2 3 4 5 6 7 8 9"
SVCORDER1="local"
SVCORDER2="yp"
SVCORDER3="bind"
SVCORDER4="local,yp"
SVCORDER5="local,bind"
SVCORDER6="yp,local"
SVCORDER7="bind,local"
DB0=aliases
DB1=auth
DB2=group
DB3=hosts
DB4=netgroup
DB5=networks
DB6=passwd
DB7=protocols
DB8=rpc
DB9=services

verbose=y

#
# Set up interrupt handlers:
#
QUIT='
	if [ -r $SVCTMP ]
	then
		rm $SVCTMP
	fi
	if [ -r $DBTMP ]
	then
		rm $DBTMP
	fi
	echo "svcsetup terminated with no installations made."
	exit 1
'

#
# Trap ^c signal, etc.
#
trap 'eval "$QUIT"' 1 2 3 15

#
# PHASE ONE: Gather data!!
#
if [ -n "$1" ]
then
	while [ -n "$1" ]
	do
		#
		# Run fast and silent for DMS client setup.
		#
		case $1 in
		-d)
			shift
			if [ -d $1 ]
			then
				if [ -f $1$SVC ]
				then
					CLIENTROOT=$1
					shift
				else
					echo "$1$SVC is not a file."
					eval "$QUIT"
				fi
			else
				echo "$1 is not a directory."
				eval "$QUIT"
			fi
			;;
		-o)
			shift
			verbose=""
			if [ $1 ]
			then
				case $1 in
				"$SVCORDER1"|"$SVCORDER4"|"$SVCORDER5" )
					LIST=$ALL
					for i in $LIST
					do
						db=DB$i
						eval DB=\$$db
						echo $DB=$1 >> $DBTMP
					done
					shift
					;;
				*)
					echo -n '"'
					echo -n $1
					echo '" is not a supported name service selection for all databases.'
					echo 'Use "local", "local,yp", or "local,bind"'
					eval "$QUIT"
					;;
				esac
			fi
			;;
		*)
			echo "usage: svcsetup [[ -d directory ] -o name_service_selection ]"
			eval "$QUIT"
			;;
		esac
	done
	#
	# Require it to be run by root
	#
	if [ \! -w $RCFILE ]
	then
		rm $DBTMP
		exit 1
	fi
fi

if [ $verbose ]
then
	#
	# Require it to be run by root
	#
	if [ \! -w $RCFILE ]
	then
		echo "Su to root first."
		eval "$QUIT"
	fi

	#
	# Be sure network has already been set up, and this baby has a name!!
	#

	hname=`hostname`
	if [ $? -ne 0 ]
	then
		echo "
Bring the system to multi-user mode before running bindsetup."
		eval "$QUIT"
	fi
	#
	# Introduction
	#
	echo "
The svcsetup command allows you to print and modify the database
selections in the $CLIENTROOT$SVC file on the current system.
This file must be modified when adding or removing a naming service,
such as Yellow Pages or BIND/Hesiod.  Run the secsetup command if
you want to change the security parameters.  Changes take effect
immediately.

[ Press the RETURN key to continue ]: "
	read junk

	done=
	while test -z "$done"
	do
		echo "
	Configuration Menu for the $CLIENTROOT$SVC file

	Modify File      => m
	Print File       => p
	Exit             => e"

		echo
		echo -n "Enter your choice [m]: "
		read action
		case $action in
		m|M|e|E|"")
			done=done
			;;
		p|P)
			#
			# Print file contents and check
			#
			echo "
The $CLIENTROOT$SVC file on \"`hostname`\" currently contains the following settings:"
			echo
			cat $CLIENTROOT$SVC | awk '{
				if ($1 ~ /#/ || NF == 0)
					next
				else
					print $0
			}'
			;;
		esac
	done
	case $action in
	[mM]|"")
		done=
		while test -z "$done"
		do
			echo "
	Change Menu for the $CLIENTROOT$SVC file

	aliases		  => 0
	auth		  => 1
	group		  => 2
	hosts		  => 3
	netgroup	  => 4
	networks	  => 5
	passwd		  => 6
	protocols	  => 7
	rpc		  => 8
	services	  => 9

	all of the above  => 10
	none of the above => 11"
			echo
			echo -n "Enter your choice(s).  For example [0 3 5] : "
			read X
			case $X in
			"")
				;;
			*)
				for I in $X
				do
					#
					# Is it a number?
					#
					J=`expr $I : '\([0-9][0-9]*\)'`
					case $I in
					10)
						LIST=$ALL
						done=y
						;;
					11)
						eval "$QUIT"
						;;
					$J)
						#
						# is it in range?
						#
						if [ $I -gt 11 ]
						then
							echo "
		Invalid Choice: $I (out of range)"
							continue
						else
							LIST="$LIST $I"
						fi
						done=y
						;;
					*)
						echo "
		Invalid choice: $I (malformed number)"
						;;
					esac
				done
				;;
			esac
		done
		for i in $LIST
		do
			case $i in
			0)
				db=aliases
				;;
			1)
				db=auth
				;;
			2)
				db=group
				;;
			3)
				db=hosts
				;;
			4)
				db=netgroup
				;;
			5)
				db=networks
				;;
			6)
				db=passwd
				;;
			7)
				db=protocols
				;;
			8)
				db=rpc
				;;
			9)
				db=services
				;;
			esac
			if [ $i -ge 0 -a $i -le 9 ]
			then
				done=
				while test -z "$done"
				do
					echo "
	local		=> 1
	yp		=> 2
	bind		=> 3
	local,yp	=> 4
	local,bind	=> 5
	yp,local	=> 6
	bind,local	=> 7"
					echo
					echo -n "Enter the naming service order for the \"$db\" database [5]: "
					read svc_order
					case $svc_order in
					1|2|3|4|5|6|7)
						done=done
						;;
					"")
						svc_order=5
						done=done
						;;
					esac
				done
				ORDER=SVCORDER$svc_order
				eval ORD=\$$ORDER
				echo $db=$ORD >> $DBTMP
			fi
		done
		;;
	[eE])
		eval "$QUIT"
		;;
	esac

fi
#
# PHASE TWO... Update file
#
trap "" 1 2 3 15

if [ $verbose ]
then
        echo ""
        echo "Updating file:"
	echo "	$CLIENTROOT$SVC"
fi
for i in $LIST
do
	case $i in
	0|1|2|3|4|5|6|7|8|9)
		db=DB$i
		eval DB=\$$db
		line=`grep "^$DB" $DBTMP`
		if [ -n "$line" ]
		then
			ed - $CLIENTROOT$SVC << END >> $NULL
/^$DB
d
-
a
$line
.
w
q
END
		fi
		;;
	esac
done
#
# Clean up
#
if [ -r $SVCTMP ]
then
	rm $SVCTMP
fi
if [ -r $DBTMP ]
then
	rm $DBTMP
fi
if [ $verbose ]
then
	echo ""
	echo "***** SVCSETUP COMPLETE *****"
fi
exit 0
