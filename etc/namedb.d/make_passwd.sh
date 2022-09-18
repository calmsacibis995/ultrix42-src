#!/bin/sh
# @(#)make_passwd.sh	4.1	(ULTRIX)	7/2/90
#									
# 			Copyright (c) 1989-1990 by
# 		Digital Equipment Corporation, Maynard, MA		
# 			All rights reserved.				
# 									
# Purpose:	Script that takes a UNIX style passwd file and converts
#		it's info to Hesiod format.
# Usage:	make_passwd [-v] [etc_source passwd_dest]
# Environment:	Bourne shell script
# 
# Remarks:
#
# Modification History:
#
# 31-Jan-90	sue
#	Added code to grab the domain name from /bin/hostname if it's
#	not in /etc/resolv.conf.
#
# 08-Jan-90	sue
#	Chmod the destination database file to root only.
#
# 21-Sep-89	sue
#	Changed the default times on the SOA.  Secondary refresh is now
#	at 60 seconds and ttl is at 3600 seconds.
#
# 11-Aug-89	sue
#	Changed the location of hesiod database.  Now check if $HESIOD
#	exists.  If it does, increment the SOA serial number, else
#	make a new SOA.
#
# 12-Jun-89	logcher
#	Created script that takes a UNIX style passwd file and 
#	converts it's info to Hesiod format.
#

PROG=$0
HESTMP=/tmp/hes.$$
NAMEDBDIR=/var/dss/namedb
SRCDIR=src
#
# Set up interrupt handlers:
#
QUIT='
	if [ -r $HESTMP ]
	then
		rm $HESTMP
	fi
	echo "$PROG terminated with no installations made."
	exit 1
'
#
# Be sure network has already been set up - this host has a name!!
#

HOST=`/bin/hostname`
if [ $? -ne 0 ]
then
	echo "
Bring the system to multi-user mode before running $PROG."
	eval "$QUIT"
fi

case $1 in
DEBUG)
	shift
	SOURCE=/tmp/passwd
	HESIOD=/tmp/passwd.db
	RESOLVC=/tmp/resolv.conf
	RCFILE=/tmp/rc.local
echo "Running in DEBUG mode ...
"
	;;
*)
	SOURCE=$NAMEDBDIR/$SRCDIR/passwd
	HESIOD=$NAMEDBDIR/passwd.db
	RESOLVC=/etc/resolv.conf
	RCFILE=/etc/rc.local
	;;
esac
 
SUBDOMAIN=passwd
START_KEY="; %PASSWD_START% - entries added by"
END_KEY="; %PASSWD_END%"

DOM="domain"
verbose=
first_time="y"
NULL=/dev/null
ADMIN=postmaster
PATH=$PATH
export PATH

#
# Trap ^c signal, etc.
#
trap 'eval "$QUIT"' 1 2 3 15

#
# PHASE ONE: Gather data!!
#
if [ -n "$1" ]
then
	case $1 in
	-v)
		verbose=y
		shift
	esac
fi
if [ -n "$1" ]
then
	if [ -f $1 ]
	then
		SOURCE=$1
		if [ -n "$2" ]
		then
			HESIOD=$2
		else
			echo "usage: $PROG [-v] [etc_source passwd_dest]"
			eval "$QUIT"
		fi
	else
		echo "Source file $1 is not a file."
		eval "$QUIT"
	fi
fi

#
# Require it to be run by root
#
if [ \! -w $RCFILE ]
then
	echo "Su to root first."
	eval "$QUIT"
fi
#
# If $RESOLVC does not exist, quit
#
if [ ! -f $RESOLVC ]
then
	echo "File $RESOLVC is not a file.  Cannot get BIND domain name."
	eval "$QUIT"
fi
set xx `(grep "$DOM" $RESOLVC)`
if [ -z "$3" ]
then
	DOMAIN=`expr $HOST : '[^.]*\.\(.*\)'`
	if [ -z "$DOMAIN" ]
	then
		echo "BIND domain name not set.  Check $RESOLVC file or /bin/hostname."
		eval "$QUIT"
	fi
else
	DOMAIN=$3
fi
HOST=`echo $HOST | sed s/.$DOMAIN//`
#
# If $SOURCE does not exist, quit
#
if [ ! -f $SOURCE ]
then
	echo "Source file $SOURCE is not a file."
	eval "$QUIT"
fi

#
# If $HESIOD exists, see if this is a re-install
#
if [ -f $HESIOD ]
then
	egrep -s "$START_KEY" $HESIOD
	if [ $? -eq 0 ]
	then
		egrep -s "$END_KEY" $HESIOD
		if [ $? -ne 0 ]
		then
			echo "
The passwd database has already been installed but can not be
reconfigured automatically.  To change the current passwd configuration,
edit the file $HESIOD to remove the old passwd database
and run $PROG again."
			eval "$QUIT"
		else
			first_time="n"
		fi
	fi
fi

trap "" 1 2 3 15
if [ $verbose ]
then
       	echo "Updating $HESIOD"
fi
if [ "$first_time" = "y" ]
then
	echo "\$origin $SUBDOMAIN.$DOMAIN.
@		HS	SOA	$HOST.$DOMAIN. $ADMIN.$HOST.$DOMAIN. (
				1	; Serial
				60	; Refresh - 1 minute
				60	; Retry - 1 minute
				1209600	; Expire - 2 weeks
				3600 )	; Minimum - 1 hour
		HS	NS	$HOST.$DOMAIN.
;
$START_KEY $PROG" > $HESTMP
else
	cat $HESIOD | awk '
BEGIN { state = 0 }
	{
	if (state == 2) {
		print $0
		next
	}
	if (state == 0 && $3 == "SOA") {
		state = 1
		print $0
	}
	else {
		if (state == 1 && ($1 >= 0 && $3 == "Serial")) {
			state = 2
			$1 +=1
			printf "\t\t\t\t%d\t", $1
			for (i = 2; i <= NR; i++)
				printf "%s ", $i
			printf "\n"
		}
		else
			print $0
	}
	}' > $HESTMP
	mv $HESTMP $HESIOD
	echo "$START_KEY $PROG" > $HESTMP
fi
cat $SOURCE | awk -F: '
BEGIN { number = 0 }
		{
		if ($1 !~ /^\+/ && $1 !~ /^\-/) {
			if (length($1) < 8)
				printf "%s\t\tHS\tTXT\t\"%s\"\n", $1, $0
			else
				printf "%s\tHS\tTXT\t\"%s\"\n", $1, $0
			printf "\"%s\"\t\tHS\tCNAME\t%s\n", $3, $1
			printf "\"passwd-%d\"\tHS\tCNAME\t%s\n", number, $1
			number += 1
		}
		}' >> $HESTMP
echo "$END_KEY" >> $HESTMP

case $first_time in
y)
	#
	# If first time, then put at end of $HESIOD
	#
	cat $HESTMP >> $HESIOD
	;;
n)
	ex - $HESIOD << END >> $NULL
/$START_KEY/,/$END_KEY/ d
.r $HESTMP
w
q
END
	;;
esac
chmod 600 $HESIOD
#
# Clean up
#
if [ -r $HESTMP ]
then
	rm $HESTMP
fi
exit 0
