#!/bin/sh
# @(#)make_aliases.sh	4.1	(ULTRIX)	7/2/90
#									
# 			Copyright (c) 1989-1990 by
# 		Digital Equipment Corporation, Maynard, MA		
# 			All rights reserved.				
# 									
# Usage:	make_aliases [-v] [etc_source aliases_dest]
# Environment:	Bourne shell script
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
# 11-Dec-89	sue
#	Made a fix to handle existing quotes in an alias, i.e. the
#	default /etc/aliases from the kit.  Also handle continuation
#	lines better in "yp" format.
#
# 19-Sep-89	sue
#	Added code to check if the aliases file is in the sendmail
#	style with colons and whitespace on continuation lines or
#	YP format with spaces and backslashes for continuation lines.
#	Then different awk scripts are run accordingly.
#
# 11-Aug-89	sue
#	Changed the location of hesiod database.  Now check if $HESIOD
#	exists.  If it does, increment the SOA serial number, else
#	make a new SOA.
#
# 12-Jun-89	logcher
#	Created script that takes a UNIX style aliases file and 
#	converts it's info to Hesiod format.
#

PROG=$0
HESTMP=/tmp/hes.$$
ALITMP=/tmp/ali.$$
ALITMP2=/tmp/ali2.$$
NAMEDBDIR=/var/dss/namedb
SRCDIR=src
#
# Set up interrupt handlers:
#
QUIT='
	if [ -r $ALITMP ]
	then
		rm $ALITMP
	fi
	if [ -r $ALITMP2 ]
	then
		rm $ALITMP2
	fi
	if [ -r $HESTMP ]
	then
		rm $HESTMP
	fi
	echo "$PROG terminated with no installations made."
	exit 1
'

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

case $1 in
DEBUG)
	shift
	SOURCE=/tmp/aliases
	HESIOD=/tmp/aliases.db
	RESOLVC=/tmp/resolv.conf
	RCFILE=/tmp/rc.local
echo "Running in DEBUG mode ...
"
	;;
*)
	SOURCE=$NAMEDBDIR/$SRCDIR/aliases
	HESIOD=$NAMEDBDIR/aliases.db
	RESOLVC=/etc/resolv.conf
	RCFILE=/etc/rc.local
	;;
esac
 
verbose=
PATH=$PATH
export PATH

SUBDOMAIN=aliases
START_KEY="; %ALIASES_START% - entries added by"
END_KEY="; %ALIASES_END%"

DOM="domain"
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
			echo "usage: $PROG [etc_source aliases_dest]"
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
The aliases database has already been installed but can not be
reconfigured automatically.  To change the current aliases configuration,
edit the file $HESIOD to remove the old aliases database
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
				300	; Refresh - 5 minutes
				60	; Retry - 1 minute
				1209600	; Expire - 2 weeks
				43200 )	; Minimum - 12 hours
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
	cp $HESTMP $HESIOD
	echo "$START_KEY $PROG" > $HESTMP
fi
#
# Remove beginning blanks and tabs which mark continued lines
#
sed -e "s/^[ 	]*//" $SOURCE > $ALITMP
sed -e "s/::/@@/g" $ALITMP > $ALITMP2
#
# Check if in YP format with spaces or sendmail with colons
#
grep -v "^#.*" $ALITMP2 > $ALITMP
egrep -s ".*:[ 	]*.*" $ALITMP
if [ $? -eq 0 ]
then
	sed -e "s/:[ 	]*/:/" $ALITMP2 > $ALITMP
	# Colon-space format:
	#
	# Add end trailer to source file
	#
	echo "foobar:foo,bar" >> $ALITMP
	cat $ALITMP | awk '
BEGIN { 
	FS = "[ \t]*"
	first = 0
	quoted = 0
}
{
	if ($1 ~ /#/ || NF == 0) {
		if (first == 1) {
			if (quoted == 0)
				printf "\""
			printf "\n"
			first = 0
		}
		if (length($1) > 1)
			printf ";%s", substr($1, 2, length($1))
		else
			printf ";"
		for (i = 2; i <= NF; i++)
			printf " %s", $i
		printf "\n"
		next
	}
	m = split($0, key, ":")
	if (m > 1) {
		if (first == 1) {
			if (quoted == 0)
				printf "\""
			printf "\n"
			first = 0
		}
		keyname = key[1]
		rest = key[2]
		if (keyname == "foobar" && rest == "foo,bar")
			next
		if (length(keyname) < 8)
			printf "%s\t\tHS\tTXT\t", keyname
		else
			printf "%s\tHS\tTXT\t", keyname
		if (rest ~ /^"/ && rest ~ /"$/)
			quoted = 1
		else {
			quoted = 0
			printf "\""
		}
	}
	else
		rest = $0
	if ("rest") {
		printf "%s", rest
		first = 1
	}
	}' > $ALITMP2
else
	# Space format:
	cat $ALITMP2 > $ALITMP
	cat $ALITMP | awk '
BEGIN { 
	first = 0
	quoted = 0
	slashend = 0
}
{
	if ($1 ~ /#/ || NF == 0) {
		if (first == 1) {
			if (quoted == 0)
				printf "\""
			printf "\n"
			first = 0
		}
		if (length($1) > 1)
			printf ";%s", substr($1, 2, length($1))
		else
			printf ";"
		for (i = 2; i <= NF; i++)
			printf " %s", $i
		printf "\n"
		next
	}
	if (first == 0) {
		if (length($1) < 8)
			printf "%s\t\tHS\tTXT\t", $1
		else
			printf "%s\tHS\tTXT\t", $1
		if ($2 ~ /^"/ && $NF ~ /"$/) {
			printf "%s", $2
			for (i=3; i <= NF; i++)
				printf " %s", $i
			quoted = 1
			first = 0
		}
		else {
			quoted = 0
			printf "\""
		}
		if (quoted == 0) {
			str = $2
			if ($2 ~ /\\$/)
				slashend = 1
			else
				slashend = 0
			if ($3 == "\\" || slashend == 1)
				first = 1
			else
				first = 0
		}
	}
	else {
		str = $1
		if ($1 ~ /\\$/)
			slashend = 1
		else
			slashend = 0
		if ($2 == "\\" || slashend == 1)
			first = 1
		else
			first = 0
	}
	if (quoted == 0) {
		m = split(str, key, ",")
		printf "%s", key[1]
		if (slashend == 1)
			m--
		for (i=2; i <= m; i++)
			printf ",%s", key[i]
		if (first == 1 && slashend == 1)
			printf ","
	}
	if (first == 0) {
		if (quoted == 0)
			printf "\""
		printf "\n"
	}
	}' > $ALITMP2
fi
sed -e "s/@@/::/g" $ALITMP2 >> $HESTMP
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
if [ -r $ALITMP ]
then
	rm $ALITMP
fi
if [ -r $ALITMP2 ]
then
	rm $ALITMP2
fi
if [ -r $HESTMP ]
then
	rm $HESTMP
fi
exit 0
