#!/bin/sh
# @(#)make_hosts.sh	4.1	(ULTRIX)	7/2/90
#									
# 			Copyright (c) 1989-1990 by
# 		Digital Equipment Corporation, Maynard, MA		
# 			All rights reserved.				
# 									
# Usage:	make_hosts [-v] [etc_source bind_hosts bind_rev]
# Environment:	Bourne shell script
# 
# Modification History:
#
# 31-Jan-90	sue
#	Added code to grab the domain name from /bin/hostname if it's
#	not in /etc/resolv.conf.
#
# 08-Jan-90	sue
#	Chmod the destination database files to root only.
#
# 11-Aug-89	sue
#	Check if $DHOSTS and $DREV exist.  If they do, increment the
#	SOA serial number, else make a new SOA.
#
# 12-Jun-89	logcher
#	Created script that takes a UNIX style hosts file and converts
#	it's info to BIND format.  Taken from Richard Johnsson's 
#	"hostobind" script.
#

PROG=$0
HOSTSTMP=/tmp/hosts.$$
REVTMP=/tmp/rev.$$
NAMEDBDIR=/var/dss/namedb
SRCDIR=src
#
# Set up interrupt handlers:
#
QUIT='
	if [ -r $HOSTSTMP ]
	then
		rm $HOSTSTMP
	fi
	if [ -r $REVTMP ]
	then
		rm $REVTMP
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
	DEBUG=1
	SOURCE=/tmp/hosts
	DHOSTS=/tmp/hosts.db
	DREV=/tmp/hosts.rev
	RESOLVC=/tmp/resolv.conf
	RCFILE=/tmp/rc.local
echo "Running in DEBUG mode ...
"
	;;
*)
	DEBUG=0
	SOURCE=$NAMEDBDIR/$SRCDIR/hosts
	DHOSTS=$NAMEDBDIR/hosts.db
	DREV=$NAMEDBDIR/hosts.rev
	RESOLVC=/etc/resolv.conf
	RCFILE=/etc/rc.local
	;;
esac
 
START_KEY="; %HOSTS_START% - entries added by"
END_KEY="; %HOSTS_END%"

DOM="domain"
hfirst_time="y"
rfirst_time="y"
NULL=/dev/null
ADMIN=postmaster
verbose=
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
			DHOSTS=$2
			if [ -n "$3" ]
			then
				DREV=$3
			else
				echo "usage: $PROG [-v] [etc_source bind_hosts bind_rev]"
				eval "$QUIT"
			fi
		else
			echo "usage: $PROG [-v] [etc_source bind_hosts bind_rev]"
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
DOTDOMAIN=.$DOMAIN
#
# If $SOURCE does not exist, quit
#
if [ ! -f $SOURCE ]
then
	echo "Source file $SOURCE is not a file."
	eval "$QUIT"
fi

#
# If $DHOSTS exists, see if this is a re-install
#
if [ -f $DHOSTS ]
then
	egrep -s "$START_KEY" $DHOSTS
	if [ $? -eq 0 ]
	then
		egrep -s "$END_KEY" $DHOSTS
		if [ $? -ne 0 ]
		then
			echo "
The hosts database has already been installed but can not be
reconfigured automatically.  To change the current hosts configuration,
edit the file $DHOSTS to remove the old hosts database
and run $PROG again."
			eval "$QUIT"
		else
			hfirst_time="n"
		fi
	fi
fi
#
# If $DREV exists, see if this is a re-install
#
if [ -f $DREV ]
then
	egrep -s "$START_KEY" $DREV
	if [ $? -eq 0 ]
	then
		egrep -s "$END_KEY" $DREV
		if [ $? -ne 0 ]
		then
			echo "
The hosts database has already been installed but can not be
reconfigured automatically.  To change the current hosts configuration,
edit the file $DREV to remove the old hosts information
and run $PROG again."
			eval "$QUIT"
		else
			rfirst_time="n"
		fi
	fi
fi
trap "" 1 2 3 15

if [ $verbose ]
then
       	echo "Updating $DHOSTS"
       	echo "Updating $DREV"
fi

if [ "$hfirst_time" = "y" ]
then
	echo ";
; Data file of hostnames in this zone.
;
@	IN	SOA	$HOST.$DOMAIN. $ADMIN.$HOST.$DOMAIN. (
			1	; Serial
			300	; Refresh - 5 minutes
			60	; Retry - 1 minute
			1209600	; Expire - 2 weeks
			43200 )	; Minimum - 12 hours
	IN	NS	$HOST.$DOMAIN.
;
$START_KEY $PROG" > $DHOSTS
else
	cat $DHOSTS | awk '
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
			printf "\t\t\t%d\t", $1
			for (i = 2; i <= NR; i++)
				printf "%s ", $i
			printf "\n"
		}
		else
			print $0
	}
	}' > $HOSTSTMP
	cp $HOSTSTMP $DHOSTS
	echo "$START_KEY $PROG" >> $DHOSTS
fi
if [ "$rfirst_time" = "y" ]
then
	echo ";
; Data file for reverse address to hostname.
;
@	IN	SOA	$HOST.$DOMAIN. $ADMIN.$HOST.$DOMAIN. (
			1	; Serial
			300	; Refresh - 5 minutes
			60	; Retry - 1 minute
			1209600	; Expire - 2 weeks
			43200 )	; Minimum - 12 hours
	IN	NS	$HOST.$DOMAIN.
;
$START_KEY $PROG" > $DREV
else
	cat $DREV | awk '
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
			printf "\t\t\t%d\t", $1
			for (i = 2; i <= NR; i++)
				printf "%s ", $i
			printf "\n"
		}
		else
			print $0
	}
	}' > $REVTMP
	cp $REVTMP $DREV
	echo "$START_KEY $PROG" >> $DREV
fi
echo $HOSTSTMP $REVTMP $DOTDOMAIN | awk '
BEGIN {
	state = 0
	number = 0
	}
	{
	if (state == 0) {		# set variables
		dhosts = $1
		drev = $2
		domain = $3
		state = 1
	}
	else {
		if ($1 ~ /#/ || NF == 0) {		# print comments to dhosts
			printf ";" > dhosts
			for (i = 2; i <= NF; i++)
				printf " %s", $i > dhosts
			printf "\n" > dhosts
			next
		}
		pname = $2;		# primary name
		if (pname ~ /#/)
			next
		n = split(pname,dname,".")
		printa = 0
		if (n > 1) {
			#
			# Domain name is being used.  Check if local
			# domain or other.
			#
			m = split(domain,thisdomain,".")
			i = 0
			if (n == m)
				for (i = 2; i <= m; i++)
					if (dname[i] != thisdomain[i])
						break
			if (i == 0 || (i > 0 && i <= m)) {
				#
				# This is another domain.  Print
				# CNAME record.
				#
				printf "%-23s IN\tCNAME\t\t%s.\n", dname[1], pname > dhosts
				fullname = pname
				pname = dname[1]
			}
			else
				if (i > m) {
					#
					# Same domain.  
					#
					fullname = pname
					pname = dname[1]
					printa = 1
				}
		}
		else {
			fullname = pname domain
			printa = 1
		}
		if (printa == 1) {
			#
			# Print A record and PTR.
			#
			printf "%-23s IN\tA\t\t%s\n", pname, $1 > dhosts
			n = split($1,addr,".")
			if (n == 4 && pname != "localhost") {
				printf "%d.%d.%d.%d.in-addr.arpa.\tIN\tPTR\t%s.\n", addr[4], addr[3], addr[2], addr[1], fullname > drev
			}
			printf "\"hosts-%d\"\t\tIN\tCNAME\t\t%s\n", number, pname > dhosts
			number += 1
		}
		for (i=3; i<= NF; i++) {
			if ($i ~ /#/)
				break		# comment begins
			if ($i ~ /\./)
				continue	# ignore domainized aliases
			if ($i == pname)
				continue	# already done
			printf "%-23s IN\tCNAME\t\t%s\n", $i, pname > dhosts
		}
	}
	}' - $SOURCE
echo "$END_KEY" >> $HOSTSTMP
echo "$END_KEY" >> $REVTMP
case $hfirst_time in
y)
	#
	# If first time, then put at end of $DHOSTS
	#
	cat $HOSTSTMP >> $DHOSTS
	;;
n)
	ex - $DHOSTS << END >> $NULL
/$START_KEY/,/$END_KEY/ d
.r $HOSTSTMP
w
q
END
	;;
esac
case $rfirst_time in
y)
	#
	# If first time, then put at end of $DREV
	#
	cat $REVTMP >> $DREV
	;;
n)
	ex - $DREV << END >> $NULL
/$START_KEY/,/$END_KEY/ d
.r $REVTMP
w
q
END
	;;
esac
chmod 600 $DHOSTS
chmod 600 $DREV
#
# Cleanup
#
if [ -r $HOSTSTMP ]
then
	rm $HOSTSTMP
fi
if [ -r $REVTMP ]
then
	rm $REVTMP
fi
exit 0
