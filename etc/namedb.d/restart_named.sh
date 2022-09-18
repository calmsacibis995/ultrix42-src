#!/bin/sh
# @(#)restart_named.sh	4.1	(ULTRIX)	7/2/90
#									
# 			Copyright (c) 1989-1990 by
# 		Digital Equipment Corporation, Maynard, MA		
# 			All rights reserved.				
# 									
# Usage:	restart_named [-v]
# Environment:	Bourne shell script
# 
# Modification History:
#
# 31-Jan-90	sue
#	Changed the signal from HUP to XFSZ per Bill's change to named.
#
# 16-Aug-89	sue
#	Removed code to update the serial numbers.  The make_hosts.sh
#	script now does this.  Changed the kill -9 to a kill -HUP.
#
# 12-Jun-89	logcher
#	Created script to look in named.hosts and named.rev and update
#	the serial number, and then kill and restart the named.
#

PROG=$0
#
# Set up interrupt handlers:
#
QUIT='
	echo "$PROG terminated with no installations made."
	exit 1
'
#
# Trap ^c signal, etc.
#
trap 'eval "$QUIT"' 1 2 3 15

#
# Be sure network has already been set up, and host has a name!!
#

HOST=`/bin/hostname`
if [ $? -ne 0 ]
then
	echo "
Bring the system to multi-user mode before running $PROG."
	eval "$QUIT"
fi

DEBUG=""
NAMEDBDIR=/var/dss/namedb

case $1 in
DEBUG)
	shift
	DEBUG=1
	NAMEDPID=/tmp/named.pid
	RCFILE=/tmp/rc.local
echo "Running in DEBUG mode ...
"
	;;
*)
	NAMEDPID=/etc/named.pid
	RCFILE=/etc/rc.local
	;;
esac

case $1 in
-v)
	verbose=y
	shift
	;;
?*)
	echo "usage: restart_named [-v]"
	eval "$QUIT"
	;;
esac

if [ $DEBUG ]
then
	echo Check tmp files
else
	if [ $verbose ]
	then
		echo "Restarting BIND named"
	fi
	if [ -s $NAMEDPID ]
	then
		kill -XFSZ `cat $NAMEDPID`
	fi
fi
exit 0
