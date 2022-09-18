#!/bin/sh
# @(#)bindsetup.sh	4.4	(ULTRIX)	2/14/91
#									
# 			Copyright (c) 1988 - 1990 by
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
# Purpose:	Set up Berkeley Internet Name Domain (BIND) and Hesiod
# Usage:	bindsetup
# Environment:	Bourne shell script
# 
# Remarks:
#    Sets up files:
#	/etc/hosts
#	/etc/sendmail.cf
#	/etc/resolv.conf
#	/etc/hesiod.conf
#	/etc/rc.local
#	/var/dss/namedb/named.boot
#	/var/dss/namedb/named.ca
#	/var/dss/namedb/named.local
#	/var/dss/namedb/named.hosts
#	/var/dss/namedb/named.rev
#
#
# Modification History:
#
# 17-Feb-88	logcher
#    This script sets up a BIND Client interactively
#    and silently (diskless setup) modifying /etc/svcorder,
#    /etc/resolv.conf, and /etc/rc.local with the complete 
#    (hostname+binddomain).  For a BIND Primary or Secondary Server
#    setup, it sets up the /etc/svcorder, /etc/rc.local with the new
#    hostname plus the named, and /etc/named.boot file after interactive
#    queries and then asks the user to set up the remaining BIND data
#    files and points to the Network Manager's Guide.
#
# 08-Mar-88	logcher
#    Modifications after Documentation looked at script output
#	plus a bug (: => ;) fix and other cleanup.
#
# 16-Mar-88	logcher
#    Changed a few comments on the RESOLVC and NAMEDBO files to remain
#    consistent with the comments in the default /etc/namedb files on
#    kit.
#
# 31-Mar-88	logcher
#    Removed code that asks for the reverse network number for servers
#    and instead use some dms code and call a c program, revnetnum,
#    so that the reverse network number is extracted from the system
#    per smu-02198.
#
# 14-Apr-88	logcher
#    Changed references of Remote Server to Client.  Added local to
#    $SVCORDER file as well as adding full hostname to $HOSTS per
#    smu-02244.  Fixed bug in grep for "bind" or "BIND" to include
#    a "^" character.  Allow for multiple servers on client setup,
#    both interactively and silently.  Put in missing setup for
#    caching only and slave servers.
#
# 26-Apr-88	logcher
#    Changes needed to use bindsetup -c ... for diskless client /
#    BIND client setup.  Needed to add an argument that contained
#    the directory path of the root filesystem, CLIENTROOT.
#
# 08-Jun-88	logcher
#    Moved the editing of /etc/svcorder to the end of the script so
#    that in a server configuration the local service is the only one
#    defined for the "arp" command and thus avoids the time-out period
#    for a non-active bind entry.
#
# 13-Jun-88	logcher
#    Put the "verbose" check on the /etc/svcorder message for diskless
#    addition.
#
# 01-Dec-88	logcher
#    Added Donnie Cherng's fix that uses netmasks to get the network
#    number.
#
# 24-Feb-89	logcher
#    Bug fixes from u32_qar 640, v24_qar 781, pmax_bar 379.
#    Check if NAMEDDIR is a file and loop.  If not a directory,
#    then mkdir it.  Change ed to cat because of 64 char limit
#    on ed pathnames.  Changed other ed's to cd to the directory.
#    Added code to cleanup old files if changing configuration
#    If server to client, remove BIND stuff from RCFILE and remove
#    NAMEDPID file.  If client to server, mv RESOLVC to RESOLVC.old.  
#
# 28-Feb-89	logcher
#    This is the real bug fix for u32_qar 640.  When configuring a
#    client, check if arg is a directory.  If not, check if first
#    character in arg is a "/".  If so, print a message that not a
#    directory and exit.  Else, it is assumed that there is no client
#    root directory and arg is assumed to be the domain name.
#
# 08-Mar-89	logcher
#    Added umask 022 to make sure a user can read necessary files,
#    like resolv.conf.
#
# 09-May-89	logcher
#    Removed all code that modifies the hostname.  A RESOLVC file is
#    now required for all configurations with at least the "domain"
#    entry.  This change is in conjuction with the gethostent change
#    that strips of the local domain, if it exists.  Added code to make
#    the NAMEDLO and NAMEDCA files and start secondary and slave servers
#    up automatically.  Removed caching only server from list since it's
#    meaning is very little.  Removed code that calls revnetnum().  Only
#    need network number, not network number plus subnets.
#
# 24-Jul-89	sue
#    Added code to automatically setup all configurations.
#
# 03-Aug-89	sue
#	Set auto to y for automatic named startup and only ask if auto
#	setup is desired if auto is y.  Check if
#	/var/dss/namedb/src/hosts is a file before trying to manipulate
#	it.  Change name of sri-nic.arpa. to nic.ddn.mil. per mail from
#	Win Treese.
#
# 16-Aug-89	sue
#	Brought back caching server setup.  It does have meaning.
#	Removed forwarding server from primary and secondary.
#	Changed the names of the hosts databases to hosts.db and
#	hosts.rev.  Removed setup for these files.  It is now performed 
#	in /var/dss/namedb/bin/make_hosts which is run when setting up a
#	primary server.  Added Hesiod setup to primary and secondary.
#
# 22-Aug-89	sue
#	Added Kerberos questions to setup.  Can only be used in
#	conjuction with Kerberos setup which is manually right now.
#
# 19-Sep-89	sue
#	Added code to the primary server setup to check if a passwd
#	file exists in the src directory and then add code to the 
#	RCFILE and start the hesupd automatically.
#
# 04-Oct-89	sue
#	Added cache files to the secondary server named.boot file.
#	Fixed incorrect names in named.ca.
#
# 13-Nov-89	sue
#	Added code to modify the $SENDMAIL file with the domain name.
#
# 12-Dec-89	sue
#	Modified the silent client command line input.  Now conforms to
#	that of svcsetup.  Do checking for the $CLIENTROOT$HOSTS file in
#	silent client mode.  Check and see if diskless client was a 
#	server once before and remove lines from $CLIENTROOT$RCFILE.
#	Add example domain name in prompt.  Added more specifics to
#	greps.  Really brought back caching server.
#
# 07-Feb-90	sue
#	Check to see if user added a dot at the end of the domain name.
#	If so, remove it.  Loop for more than one Kerberos server, and
#	add to /etc/hosts if not there already.  Bring back the long
#	hostname setup.  Setting /bin/hostname, and changing
#	/etc/rc.local.
#
# 28-Feb-90	sue
#	Updated the root name servers in named.ca; re mail from Win.
#
# 17-Jul-90	sue
#	Updated the root name servers in named.ca; re mail from Win.
#
# 15-Oct-90	sue
#	Added a "cd /" before changing the .$SENDMAIL and .$RCFILE
#	files if $CLIENTROOT is null.

# files
case $1 in
DEBUG)
	shift
	DEBUG=1
	RCFILE=/tmp/rc.local
	HOSTS=/tmp/hosts
	SENDMAIL=/tmp/sendmail.cf
	HESIOD=/tmp/hesiod.conf
	RESOLVC=/tmp/resolv.conf
	SVC=/tmp/svc.conf
	SVCORDER=/tmp/svcorder
	NAMEDBO=/tmp/named.boot
	NAMEDPID=/tmp/named.pid
echo "Running in DEBUG mode ...
"
	;;
*)
	RCFILE=/etc/rc.local
	HOSTS=/etc/hosts
	SENDMAIL=/etc/sendmail.cf
	HESIOD=/etc/hesiod.conf
	RESOLVC=/etc/resolv.conf
	SVC=/etc/svc.conf
	SVCORDER=/etc/svcorder
	NAMEDBO=/var/dss/namedb/named.boot
	NAMEDPID=/etc/named.pid
	;;
esac
#
# Default named file locations
#
NAMEDDIR=/var/dss/namedb
NAMEDSRC=$NAMEDDIR/src
NAMEDBIN=$NAMEDDIR/bin
NAMEDLO=named.local
NAMEDCA=named.ca
NAMEDHO=hosts.db
NAMEDRE=hosts.rev
HO=hosts
DATABASES="aliases auth group hosts networks passwd protocols rpc services"
KRBFLAGS=
DOM=domain
SVCSETUP=/usr/etc/svcsetup
#
# Other declarations
#
umask 022
CLIENTROOT=""
RESTMP=/tmp/resolv.tmp.$$
HOSTSTMP=/tmp/hosts.tmp.$$
BINDTMP=/tmp/bindsetup.tmp.$$
RCTMP=/tmp/bindsetup.rc.$$
HOSTNAME=/bin/hostname
NULL=/dev/null

# defines
LOCAL_KEY="echo -n 'local daemons:'"
BINDSTART_KEY="# %BINDSTART% - BIND daemon added by \"bindsetup\""
BINDEND_KEY="# %BINDEND%"
NFSSTART_KEY="# %NFSSTART% - NFS daemons added by \"nfssetup\""
PATH=$PATH:$NAMEDDIR
export PATH

typeserver=""
server=""
slash=""
PRINTMAIL=""
PRINTRC="y"
PRINTHO="y"
verbose=y
first_time=y
scratch_maps=y
auto=y
#
# Set up interrupt handlers:
#
QUIT='
	if [ -r $RESTMP ]
	then
		rm $RESTMP
	fi
	if [ -r $HOSTSTMP ]
	then
		rm $HOSTSTMP
	fi
	if [ -r $BINDTMP ]
	then
		rm $BINDTMP
	fi
	if [ -r $RCTMP ]
	then
		rm $RCTMP
	fi
	echo "bindsetup terminated with no installations made."
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
	#
	# Require it to be run by root
	#
	if [ \! -w $RCFILE ]
	then
		exit 1
	fi
	#
	# Run fast and silent for DMS client setup.
	#
	case $1 in
	-c)
		verbose=""
		typeserver=c
		shift
		;;
	*)
		echo "usage: bindsetup [ -c [ -d directory ] -b binddomain name1,ip1 name2,ip2 ... ]"
		eval "$QUIT"
		;;
	esac
	while [ -n "$1" ]
	do
		case $1 in
		-d)
			shift
			if [ -d $1 ]
			then
				CLIENTROOT=$1
				shift
			else
				echo "$1 is not a directory."
				eval "$QUIT"
			fi
			;;
		-b)
			shift
			if [ -n "$1" ] 
			then
				binddomain=$1
				echo $binddomain | egrep -s "\.$"
				if [ $? -eq 0 ]
				then
					binddomain=`echo $binddomain | sed s/\.$//`
				fi
				shift
			fi
			;;
		*)
			name=`echo $1 | awk -F, '{print $1}'`
			number=`echo $1 | awk -F, '{print $2}'`
			if [ -z "$name" -o -z "$number" ]
			then
				echo "name,ip argument is improperly formatted"
				echo "usage: bindsetup [ -c [ -d directory ] -b binddomain name1,ip1 name2,ip2 ... ]"
				eval "$QUIT"
			else
				echo "nameserver	$number" >> $RESTMP
				if [ -f $CLIENTROOT$HOSTS ]
				then
					egrep -s "[ 	]$name" $CLIENTROOT$HOSTS
					if [ $? -ne 0 ]
					then
						name=`echo $name | sed s/.$binddomain//`
						echo "$number $name.$binddomain $name		# BIND Server" >> $HOSTSTMP
					fi
				else
					echo "$CLIENTROOT$HOSTS is not a file."
					eval "$QUIT"
				fi
				shift
			fi
			;;
		esac
	done
	egrep -s "$BINDSTART_KEY" $CLIENTROOT$RCFILE
	if [ $? -eq 0 ]
	then
		egrep -s "$BINDEND_KEY" $CLIENTROOT$RCFILE
		if [ $? -eq 0 ]
		then
			first_time=""
		fi
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
	# Be sure network has already been set up, and this system has
	# a name!!
	#

	hname=`$HOSTNAME`
	if [ $? -ne 0 ]
	then
		echo "
Bring the system to multi-user mode before running bindsetup."
		eval "$QUIT"
	fi

	echo -n "
The bindsetup command allows you to add, modify, or remove a
configuration of the Berkeley Internet Name Domain (BIND) Server on
your system.  BIND is a network naming service that enables servers
to name resources or objects and share information with other objects
on the network.
"
	echo "
The bindsetup command takes you through the configuration process to
set the default BIND domain name of your system, determine the type
of server or client, and construct the BIND database files for this
BIND domain.  Default answers are shown in square brackets ([]).  To
use a default answer, press the RETURN key.

[ Press the RETURN key to continue ]: "
	read junk

	done=
	while test -z "$done"
	do
		echo "	Berkeley Internet Name Domain (BIND)
	Action Menu for Configuration

	Add              => a
	Modify           => m
	Remove           => r
	Exit             => e"

		echo
		echo -n "Enter your choice [a]: "
		read action
		case $action in
		A|a|M|m|R|r|e|E|"") done=done;;
		esac
	done
	case $action in
	[AaMm]|"")
		#
		# See if this is a re-install
		#
		egrep -s "$BINDSTART_KEY" $RCFILE
		if [ $? -eq 0 ]
		then
			egrep -s "$BINDEND_KEY" $RCFILE
			if [ $? -ne 0 ]
			then
				echo "
The BIND environment for this host has already been installed but
can not be reconfigured automatically.  To change the current BIND
configuration, edit the file $RCFILE to remove the old BIND
environment and run bindsetup again."
				eval "$QUIT"
			fi
			echo "
The BIND environment for this host has already been installed.
Would you like to change the current BIND configuration?"
			again=y
			while [ $again ]
			do
				again=""
				echo -n "
Enter \"y\" or \"n\" [no default]: "
				read ans
				case $ans in
				[yY]*)
					first_time=""
					;;
				[nN]*)
					eval "$QUIT"
					;;
				*)
					again=y
					;;
				esac
			done
		fi

		#
		# Ask for the binddomain
		#
		echo "
You must know the default BIND domain name for your site in order to
continue the configuration process.  The name "dec.com" is an example
BIND domain name.  If you do not know the name, contact your site
administrator."
		answer=y
		while [ $answer ]
		do
			echo -n "
Enter \"e\" to EXIT or \"c\" to CONTINUE [no default] ? "
			read ans
			case $ans in
			[cC]*)
				answer=""
				;;
			[eE]*)
				eval "$QUIT"
				;;
			*)
				;;
			esac
		done
		if [ -f $RESOLVC ]
		then
			set xx `(grep "^$DOM" $RESOLVC)`
			if [ -n "$3" ]
			then
				binddomain=$3
			fi
		fi
		if [ -z "$binddomain" ]
		then
			binddomain=`expr $hname : '[^.]*\.\(.*\)'`
		fi
		again=y
		while [ $again ]
		do
			echo -n "
Enter the default BIND domain name [$binddomain]: "
			read ans
			case $ans in
			"")
				if [ $binddomain ]
				then
					again=""
				fi
				;;
			*)
				again=""
				binddomain=$ans
				echo $binddomain | egrep -s "\.$"
				if [ $? -eq 0 ]
				then
					binddomain=`echo $binddomain | sed s/\.$//`
				fi
				;;
			esac
		done

		done=
		while test -z "$done"
		do
			echo "
	Berkeley Internet Name Domain (BIND)
	Configuration Menu for domain \"${binddomain}\" 

	Primary Server   => p
	Secondary Server => s
	Caching Server   => a
	Slave Server     => l
        Client           => c
	Exit             => e"

			echo
			echo -n "Enter your choice [c]: "
			read typeserver
			case $typeserver in
			P|p|S|s|A|a|L|l|C|c|e|E|"") done=done;;
			esac
		done

		case $typeserver in
		[pP])
			
			cp $NULL $RESTMP
			/bin/ls $NAMEDSRC > $BINDTMP
			if [ -s $BINDTMP ]
			then
				for i in `cat $BINDTMP`
				do
					for j in $DATABASES
					do
						if [ "$i" = "$j" ]
						then
							echo $i >> $RESTMP
						fi
					done
				done
			fi
			if [ -s $RESTMP ]
			then
				echo "
Bindsetup has found the following files in $NAMEDSRC:"
				for i in `cat $RESTMP`
				do
					echo "	$i"
				done
				answer=y
				while [ $answer ]
				do
					echo -n "
Would you like bindsetup to convert the \"etc\" style source files
to the BIND/Hesiod database format?  If you answer \"y\", the source
files remain in $NAMEDSRC and BIND/Hesiod database will be
created in $NAMEDDIR.  If you answer \"n\", the BIND/Hesiod
database will not be created, but all other configuration files will
be created.  (y/n) ? "
					read ans
					case $ans in
					[yY]*)
						auto=y
						answer=""
						;;
					[nN]*)
						auto=n
						answer=""
						;;
					*)
						;;
					esac
				done
			else 
				auto=n
				echo "
Bindsetup has found NO \"etc\" source files in $NAMEDSRC and
therefore cannot create the BIND/Hesiod database."
				answer=y
				while [ $answer ]
				do
					echo -n "
Would you like to continue bindsetup and create the BIND/Hesiod
database from \"etc\" style source files after the setup is complete
(y/n) ? "
					read ans
					case $ans in
					[yY]*)
						answer=""
						;;
					[nN]*)
						eval "$QUIT"
						;;
					*)
						;;
					esac
				done
			fi
			;;
		[sS])
			echo "
Before configuring your system as a BIND secondary server, you
must know the host name and Internet address of the BIND primary
server for this domain."
			answer=y
			while [ $answer ]
			do
				echo -n "
Enter \"e\" to EXIT or \"c\" to CONTINUE [no default] ? "
				read ans
				case $ans in
				[cC]*)
					answer=""
					;;
				[eE]*)
					eval "$QUIT"
					;;
				*)
					;;
				esac
			done
			#
			# Ask for servers ...
			#
			again=y
			while [ $again ]
			do
				again=""
				echo -n "
Enter the host name of the BIND primary server in the \"$binddomain\"
domain: "
				read name
				case $name in
				"")
					again=y
					;;
				esac
			done
			echo $name > $BINDTMP
			egrep -s "$binddomain" $BINDTMP
			if [ $? -eq 0 ]
			then
				name=`echo $name | sed s/.$binddomain//`
			fi
			notgotit=y
			set xx `(grep "[ 	]$name" $HOSTS)`
			if [ -n "$2" ]
			then
				number=$2
				notgotit=""
			fi
			again=y
			while [ $again ]
			do
				again=""
				echo -n "
Enter the Internet address for $name.$binddomain [$number]: "
				read ans
				case $ans in
				"")
					if [ -z "$number" ]
					then
						again="y"
					fi
					;;
				*)
					number=$ans
					;;
				esac
			done
			if [ -n "$notgotit" -o "$2" != "$number" ]
			then
				echo "$number $name.$binddomain $name		# BIND server" >> $HOSTSTMP
			fi
			IPNETPRI=$number
			;;
		[lL])
			echo "
Before configuring your system as a BIND slave server, you must know
the host name and Internet address of the specified BIND server(s) for
this domain."
			answer=y
			FORWSERVER=""
			while [ $answer ]
			do
				echo -n "
Enter \"e\" to EXIT or \"c\" to CONTINUE [no default] ? "
				read ans
				case $ans in
				[cC]*)
					answer=""
					;;
				[eE]*)
					eval "$QUIT"
					;;
				*)
					;;
				esac
			done
			#
			# Ask for servers ...
			#
			echo "
When finished entering BIND server(s), press the RETURN key only."
			again=y
			first=y
			while [ $again ]
			do
				echo -n "
Enter the host name of the BIND server in the \"$binddomain\"
domain: "
				read name
				case $name in
				"")
					if [ $first ]
					then
						echo "
At least one BIND server must be entered."
					else
						echo -n "
Finished entering BIND server(s) (y/n) [n] : "
						read ans
						case $ans in
						[yY]*)
							again=""
							;;
						[nN]*|"")
							;;
						*)
							;;
						esac
					fi
					;;
				*)
					first=""
					echo $name > $BINDTMP
					egrep -s "$binddomain" $BINDTMP
					if [ $? -eq 0 ]
					then
						name=`echo $name | sed s/.$binddomain//`
					fi
					notgotit=y
					set xx `(grep "[ 	]$name" $HOSTS)`
					if [ -n "$2" ]
					then
						number=$2
						notgotit=""
					else
						number=""
					fi
					againn=y
					while [ $againn ]
					do
						againn=""
						echo -n "
Enter the Internet address for $name.$binddomain [$number]: "
						read ans
						case $ans in
						"")
							if [ -z "$number" ]
							then
								againn="y"
							fi
							;;
						*)
							number=$ans
							;;
						esac
					done
					if [ -n "$notgotit" -o "$2" != "$number" ]
					then
						echo "$number $name.$binddomain $name		# BIND server" >> $HOSTSTMP
					fi
					FORWSERVER=`echo "$FORWSERVER $number"`
					;;
				esac
			done
			;;
		[cC]|"")
			typeserver=c
			echo "
Before configuring your system as a BIND client, you should first be
sure that there IS at least one system on the network configured as
either a BIND primary or secondary server for this domain.  You must
know the host name(s) and Internet address(es) of the specified BIND
server(s) for this domain.  If no server is configured, you will not
be able to access the hosts database.  If you do not know whether a
BIND server exists, contact your site administrator."
			answer=y
			while [ $answer ]
			do
				echo -n "
Enter \"e\" to EXIT or \"c\" to CONTINUE [no default] ? "
				read ans
				case $ans in
				[cC]*)
					answer=""
					;;
				[eE]*)
					eval "$QUIT"
					;;
				*)
					;;
				esac
			done
			#
			# Ask for servers ...
			#
			echo "
When finished entering BIND server(s), press the RETURN key only."
			again=y
			first=y
			while [ $again ]
			do
				echo -n "
Enter the host name of the BIND server in the \"$binddomain\"
domain: "
				read name
				case $name in
				"")
					if [ $first ]
					then
						echo "
At least one BIND server must be entered."
					else
						echo -n "
Finished entering BIND server(s) (y/n) [n] : "
						read ans
						case $ans in
						[yY]*)
							again=""
							;;
						[nN]*|"")
							;;
						*)
							;;
						esac
					fi
					;;
				*)
					first=""
					echo $name > $BINDTMP
					egrep -s "$binddomain" $BINDTMP
					if [ $? -eq 0 ]
					then
						name=`echo $name | sed s/.$binddomain//`
					fi
					notgotit=y
					set xx `(grep "[ 	]$name" $HOSTS)`
					if [ -n "$2" ]
					then
						number=$2
						notgotit=""
					else
						number=""
					fi
					againn=y
					while [ $againn ]
					do
						againn=""
						echo -n "
Enter the Internet address for $name.$binddomain [$number]: "
						read ans
						case $ans in
						"")
							if [ -z "$number" ]
							then
								againn="y"
							fi
							;;
						*)
							number=$ans
							;;
						esac
					done
					if [ -n "$notgotit" -o "$2" != "$number" ]
					then
						echo "$number $name.$binddomain $name		# BIND server" >> $HOSTSTMP
					fi
					echo "nameserver	$number" >> $RESTMP
					;;
				esac
			done
			;;
		[eE])
			eval "$QUIT"
			;;
		esac
		;;
	[Rr])
		#
		# Remove BIND Configuration
		#
		echo "
The Remove Option removes BIND from $RCFILE, if it exists, and
moves the $RESOLVC to $RESOLVC.old."
		answer=y
		while [ $answer ]
		do
			echo -n "
Enter \"e\" to EXIT or \"c\" to CONTINUE [no default] ? "
			read ans
			case $ans in
			[cC]*)
				answer=""
				;;
			[eE]*)
				eval "$QUIT"
				;;
			*)
				;;
			esac
		done
		if [ $verbose ]
		then
			echo ""
       			echo "Updating files:"
		fi
		egrep -s "$BINDSTART_KEY" $RCFILE
		if [ $? -eq 0 ]
		then
			egrep -s "$BINDEND_KEY" $RCFILE
			if [ $? -ne 0 ]
			then
				echo "
^GThe BIND environment for this host has already been installed but
can not be removed automatically.  To remove the current BIND
configuration, edit the file $RCFILE to remove the old BIND
environment."
				eval "$QUIT"
                	fi
			if [ $verbose ]
			then
				echo "  $RCFILE"
			fi
			ed - $RCFILE << END >> $NULL
/$BINDSTART_KEY/,/$BINDEND_KEY/ d
w
q
END
                fi
		if [ -f $RESOLVC ]
		then
			if [ $verbose ]
			then
				echo "  $RESOLVC"
			fi
			mv $RESOLVC $RESOLVC.old
		fi
		#
		# Must edit /etc/svc.conf
		#
		echo "
BIND cannot be completely removed from this system unless the
$SVC file is edited and \"bind\" is removed from all database
lists.  Please run $SVCSETUP or edit $SVC manually.  See svcsetup(8)."
		echo
		exit 0
		;;
	[eE])
		eval "$QUIT"
		;;
	esac

fi
#
# Ask if want Kerberos Authenicated named
#
case $typeserver in
[pPsSaAlL])
	answer=y
	kerb=
	while [ $answer ]
	do
		echo -n "
Do you want to run Kerberos Authenicated named (y/n) [n] ? "
		read ans
		case $ans in
		[yY]*)
			kerb=y
			answer=""
			;;
		[nN]*|"")
			answer=""
			;;
		*)
			;;
		esac
	done
	if [ "$kerb" = "y" ]
	then
		answer=y
		kerb=
		echo "
In order to run a Kerberos Authenicated named, you must have already
set up the appropriate Kerberos files.  These file include /etc/krb.conf
and /etc/srvtab.  See the man pages for set up information."
		while [ $answer ]
		do
			echo -n "
Do you already have /etc/krb.conf and /etc/srvtab set up (y/n) [n] ? "
			read ans
			case $ans in
			[yY]*)
				kerb=y
				answer=""
				;;
			[nN]*|"")
				answer=""
				;;
			*)
				;;
			esac
		done

	fi
	#
	# If kerb set, ask if there is a Kerberos master
	#
	if [ "$kerb" = "y" ]
	then
		answer=y
		kerb=
		echo "
In order to run a Kerberos Authenicated named, a Kerberos master must 
already be set up and running on your local area network.  If this is
not so, and you answer "y" to this question, your named will not run 
properly and you will have to rerun bindsetup without selecting the
Kerberos options."
		while [ $answer ]
		do
			echo -n "
Is there a Kerberos master already set up in your local area
network (y/n) [n] ? "
			read ans
			case $ans in
			[yY]*)
				kerb=y
				answer=""
				;;
			[nN]*|"")
				answer=""
				;;
			*)
				;;
			esac
		done
	fi
	if [ "$kerb" = "y" ]
	then
		#
		# Ask for servers ...
		#
		echo "
When finished entering Kerberos server(s), press the RETURN key only."
		again=y
		first=y
		while [ $again ]
		do
			echo -n "
Enter the host name of the Kerberos server: "
			read name
			case $name in
			"")
				if [ $first ]
				then
					echo "
At least one Kerberos server must be entered."
				else
					echo -n "
Finished entering Kerberos server(s) (y/n) [n] : "
					read ans
					case $ans in
					[yY]*)
						again=""
						;;
					[nN]*|"")
						;;
					*)
						;;
					esac
				fi
				;;
			*)
				first=""
				echo $name > $BINDTMP
				egrep -s "$binddomain" $BINDTMP
				if [ $? -eq 0 ]
				then
					name=`echo $name | sed s/.$binddomain//`
				fi
				notgotit=y
				set xx `(grep "[ 	]$name" $HOSTS)`
				if [ -n "$2" ]
				then
					number=$2
					notgotit=""
				else
					number=""
				fi
				againn=y
				while [ $againn ]
				do
					againn=""
					echo -n "
Enter the Internet address for $name.$binddomain [$number]: "
					read ans
					case $ans in
					"")
						if [ -z "$number" ]
						then
							againn="y"
						fi
						;;
					*)
						number=$ans
						;;
					esac
				done
				if [ -n "$notgotit" -o "$2" != "$number" ]
				then
					echo "$number $name.$binddomain $name		# Kerberos server" >> $HOSTSTMP
				fi
				;;
			esac
		done
		KRBFLAGS="-n -a kerberos.one -b "
	fi
	;;
esac
#
# PHASE TWO... Update files!!
#
trap "" 1 2 3 15
if [ $verbose ]
then
	echo ""
       	echo "Updating files:"
fi
#
# If have servers, add to end of $HOSTS
#
if [ -r $HOSTSTMP ]
then
	if [ $verbose ]
	then
       		echo "  $CLIENTROOT$HOSTS"
		PRINTHO=""
	fi
	cat $HOSTSTMP >> $CLIENTROOT$HOSTS
fi
#
# First check to see if hostname has domain in it already, then
# edit $RCFILE and add $binddomain to hostname if it's not there
# already
#
hname=`grep "hostname " $CLIENTROOT$RCFILE | sed 's,/bin/hostname ,,'`
echo $hname > $BINDTMP
egrep -s "$binddomain" $BINDTMP
if [ $? -eq 0 ]
then
	hname=`echo $hname | sed s/.$binddomain//`
else
	if [ $verbose ]
	then
		echo "  $CLIENTROOT$RCFILE"
		PRINTRC=""
	fi
	ed - $CLIENTROOT$RCFILE << END >> $NULL
/hostname/s/hostname.*$/hostname $hname.$binddomain/
w
q
END
fi
#
# Check if $hname.$binddomain is in $HOSTS
# If not, add it
#
egrep -s "$hname.$binddomain" $CLIENTROOT$HOSTS
if [ $? -ne 0 ]
then
	if [ -n "$verbose" -a -n "$PRINTHO" ]
	then
		echo "  $CLIENTROOT$HOSTS"
	fi
	ed - $CLIENTROOT$HOSTS << END >> $NULL
/$hname/s/$hname/$hname.$binddomain $hname/
w
q
END
fi
#
# Only set /bin/hostname if no CLIENTROOT set
#
if [ -z "$CLIENTROOT" ]
then
	$HOSTNAME $hname.$binddomain
fi
#
# Check if domain name is in $SENDMAIL.  If not add it and
# print a message to stop, freeze, and restart sendmail.
#
egrep -s "^DD$binddomain" $CLIENTROOT$SENDMAIL
if [ $? -ne 0 ]
then
	if [ $verbose ]
	then
		echo "  $CLIENTROOT$SENDMAIL"
	fi
	here=`pwd`
	if [ -n "$CLIENTROOT" ]
	then
		cd $CLIENTROOT
	else
		cd /
	fi
	ed - .$SENDMAIL << END >> $NULL
/^DD
d
-
a
DD$binddomain
.
w
q
END
	cd $here
	PRINTMAIL=y
fi
#
# make the $HESIOD file with rhs and lhs
#
if [ $verbose ]
then
       	echo "  $CLIENTROOT$HESIOD"
fi
echo "rhs=.$binddomain
lhs=" > $CLIENTROOT$HESIOD
#
# make the $RESOLVC file with domain entry
#
if [ $verbose ]
then
       	echo "  $CLIENTROOT$RESOLVC"
fi
echo ";
; BIND data file.
;
domain		$binddomain" > $CLIENTROOT$RESOLVC
#
# If server type is client, add nameservers to the $RESOLVC file
#
case $typeserver in
[cC])
	cat $RESTMP >> $CLIENTROOT$RESOLVC
	#
	# first_time is null if $BINDSTART_KEY in RCFILE.
	# This means, previously was server, so remove
	# named since not needed for client setup.
	# Also cd to CLIENTROOT because of ed bug that limits
	# the pathname to 64 characters.
	#
	if [ -z "$first_time" ]
	then
		if [ -n "$verbose" -a -n "$PRINTRC" ]
		then
			echo "  $RCFILE"
		fi
		here=`pwd`
		if [ -n "$CLIENTROOT" ]
		then
			cd $CLIENTROOT
		else
			cd /
		fi
		ed - .$RCFILE << END >> $NULL
/$BINDSTART_KEY/,/$BINDEND_KEY/ d
w
q
END
		cd $here
		#
		# Only kill named if not running in silent client
		# mode for a diskless client.
		#
		if [ -n "$verbose" -a -z "$CLIENTROOT" ]
		then
			if [ -f $NAMEDPID ]
			then
				kill -9 `cat $NAMEDPID` >> $NULL
				rm $NAMEDPID
			fi
		fi
	fi
	;;
esac
#
# Add /usr/etc/named to RCFILE if server type is primary, secondary 
# or slave
#
case $typeserver in
[pPsSaAlL])
	#
	# Add localhost to RESOLVC
	#
	echo "nameserver	127.0.0.1" >> $CLIENTROOT$RESOLVC
	#
	if [ -n "$verbose" -a -n "$PRINTRC" ]
	then
		echo "  $RCFILE"
	fi
	#
	# Banner
	#
	echo $BINDSTART_KEY > $RCTMP
	echo "echo -n 'BIND daemon:'						>/dev/console
[ -f /usr/etc/named ] && {
	/usr/etc/named $KRBFLAGS$NAMEDBO; echo -n ' named'	>/dev/console
}
echo '.'							>/dev/console" >> $RCTMP
	echo $BINDEND_KEY >> $RCTMP
	if [ $first_time ]
	then
		#
		# If NFS is there, start BIND first!!
		#
		egrep -s "$NFSSTART_KEY" $RCFILE
		if [ $? -eq 0 ]
		then
			ed - $RCFILE << END >> $NULL
/$NFSSTART_KEY
-
.r $RCTMP
w
q
END
		else
			#
			# No NFS stuff; put 'em before local daemons
			#
			egrep -s "$LOCAL_KEY" $RCFILE
			if [ $? -eq 0 ]
			then
				ed - $RCFILE << END >> $NULL
/$LOCAL_KEY
-
.r $RCTMP
w
q
END
			else
				#
				# Nothing there; put 'em on the end
				#
				cat $RCTMP >> $RCFILE
			fi
		fi
	else
		ed - $RCFILE << END >> $NULL
/$BINDSTART_KEY/,/$BINDEND_KEY/ d
-
.r $RCTMP
w
q
END
	fi
	rm $RCTMP
	#
	# Add hesupd to RCFILE and add "bindmaster" to server host
	# line in $NAMEDSRC/$HO
	#
	case $typeserver in
	[pP])
		#
		# Add the hesupd to the RCFILE
		#
		if [ -r $NAMEDSRC/passwd ]
		then
			echo "[ -f /usr/etc/hesupd ] && {
	/usr/etc/hesupd; echo -n ' hesupd'			>/dev/console
}" > $RCTMP
			ed - $RCFILE << END >> $NULL
/$BINDEND_KEY/
-
-
.r $RCTMP
w
q
END
			rm $RCTMP
		fi
		#
		# Added bindmaster to hosts database
		#
		if [ -f $NAMEDSRC/$HO ]
		then
			egrep "[ 	]$hname" $NAMEDSRC/$HO > $BINDTMP
			if [ $? -eq 0 ]
			then
				#
				# Remove old bindmaster
				#
				egrep -s "bindmaster" $NAMEDSRC/$HO
				if [ $? -eq 0 ]
				then
					ed - $NAMEDSRC/$HO << END >> $NULL
/bindmaster
s/bindmaster//
w
q
END
				fi
				if [ $verbose ]
				then
					echo "  $NAMEDSRC/$HO"
				fi
				egrep -s "#" $BINDTMP
				if [ $? -eq 0 ]
				then
					ed - $NAMEDSRC/$HO << END >> $NULL
/$hname
s/[ 	]*#/ bindmaster		#/
w
q
END
				else
					ed - $NAMEDSRC/$HO << END >> $NULL
/$hname
s/$/ bindmaster/
w
q
END
				fi
			fi
		fi
		#
		# Add bindmaster to /etc/hosts
		#
		if [ -f $HOSTS ]
		then
			egrep "[ 	]$hname" $HOSTS > $BINDTMP
			if [ $? -eq 0 ]
			then
				#
				# Remove old bindmaster
				#
				egrep -s "bindmaster" $HOSTS
				if [ $? -eq 0 ]
				then
					ed - $HOSTS << END >> $NULL
/bindmaster
s/bindmaster//
w
q
END
				fi
				if [ $verbose ]
				then
					echo "  $HOSTS"
				fi
				egrep -s "#" $BINDTMP
				if [ $? -eq 0 ]
				then
					ed - $HOSTS << END >> $NULL
/$hname
s/[ 	]*#/ bindmaster		#/
w
q
END
				else
					ed - $HOSTS << END >> $NULL
/$hname
s/$/ bindmaster/
w
q
END
				fi
			fi
		fi
		;;
	esac
	case $typeserver in
	[pPsS])
		#
		# Extract the reverse network number of this host
		# by arping.
		#
		#  Get IP address of host.
		#
		2>&1 hostaddr=`/etc/arp "$hname" | sed 's/.*(\(.*\)).*/\1/'`
		case $hostaddr in 
		"") 	echo "Can't find $hname in arp tables."
			eval "$QUIT"
		esac
		set xx `(IFS=.; echo $hostaddr)`
		w=$2; x=$3; y=$4; z=$5
		if [ $w -ge 1 -a $w -le 126 ]
		then
			IPNETREV=$w
		elif [ $w -ge 128 -a $w -le 191 ]
		then
			IPNETREV=$x.$w
		elif [ $w -ge 192 -a $w -le 223 ]
		then
			IPNETREV=$y.$x.$w
		fi
	
		;;
	esac
	#
	# Set up $NAMEDBO
	#
	if [ $verbose ]
	then
        	echo "  $NAMEDBO"
	fi
	case $typeserver in
	[pP])
		echo ";
; BIND data file to boot a primary name server.
;
; directory where all the data files are stored
directory	$NAMEDDIR
;
; type		domain			source host/file
primary		$binddomain		$NAMEDHO
primary		$IPNETREV.in-addr.arpa	$NAMEDRE
;" > $NAMEDBO
		if [ -s $RESTMP ]
		then
			for i in `cat $RESTMP`
			do
				if [ $i = "hosts" ]
				then
					continue
				elif [ $i = "rpc" ]
				then
					echo "primary		$i.$binddomain		$i.db" >> $NAMEDBO
				     else
					echo "primary		$i.$binddomain	$i.db" >> $NAMEDBO
				fi
			done
			echo ";" >> $NAMEDBO
		fi
		echo "primary		0.0.127.in-addr.arpa	$NAMEDLO
;
; load the cache data last
cache		.			$NAMEDCA" >> $NAMEDBO
		if [ -s $RESTMP ]
		then
			if [ "$auto" = "y" ]
			then
				#
				# Make hesiod database from etc files.
				#
				here=`pwd`
				cd $NAMEDDIR
				for i in `cat $RESTMP`
				do
					if [ $verbose ]
					then
       						echo "  $NAMEDDIR/$i.db"
						if [ $i = "hosts" ]
						then
       							echo "  $NAMEDDIR/$i.rev"
						fi
					fi
					$NAMEDBIN/make_$i
				done
				cd $here
			fi
		fi
		
		;;
	[sS])
		echo ";
; BIND data file to boot a secondary name server.
;
; directory where all the data files are stored
directory	$NAMEDDIR
;
; type		domain			source host/file
secondary	$binddomain		$IPNETPRI	hosts.db
secondary	$IPNETREV.in-addr.arpa	$IPNETPRI	hosts.rev
;" > $NAMEDBO
		for i in $DATABASES
		do
			if [ $i = "hosts" ]
			then
				continue
			elif [ $i = "rpc" ]
			then
				echo "secondary	$i.$binddomain		$IPNETPRI	$i.db" >> $NAMEDBO
			     else
				echo "secondary	$i.$binddomain	$IPNETPRI	$i.db" >> $NAMEDBO
			fi
		done
		echo ";
primary		0.0.127.in-addr.arpa	$NAMEDLO
;
; load the cache data last
cache		.			$NAMEDCA" >> $NAMEDBO
		;;
	[aA])
		echo ";
; BIND data file to boot a caching only name server.
;
; directory where all the data files are stored
directory	$NAMEDDIR
;
; type		domain			source host/file
primary		0.0.127.in-addr.arpa	$NAMEDLO
;
; load the cache data last
cache		.			$NAMEDCA" > $NAMEDBO
		;;
	[lL])
		echo ";
; BIND data file to boot a slave name server.
;
; directory where all the data files are stored
directory	$NAMEDDIR
;
; type		domain			source host/file
primary		0.0.127.in-addr.arpa	$NAMEDLO
;
slave" > $NAMEDBO
		if [ -n "$FORWSERVER" ]
		then
			echo "forwarders $FORWSERVER" >> $NAMEDBO
		fi
		;;
	esac
	#
	# Add NAMEDLO
	#
	if [ $verbose ]
	then
		echo "  $NAMEDDIR/$NAMEDLO"
	fi
	echo ";
; BIND data file for local loopback interface. 
;
@	IN	SOA	$hname.$binddomain. postmaster.$hname.$binddomain. (
			1	; Serial
			3600	; Refresh
			300	; Retry
			3600000	; Expire
			3600 )	; Minimum
	IN	NS	$hname.$binddomain.
1	IN	PTR	localhost.
localhost.	IN	A	127.0.0.1" > $NAMEDDIR/$NAMEDLO
	#
	# Add NAMEDCA
	#
	case $typeserver in
	[pPsSaA])
		if [ $verbose ]
		then
			echo "  $NAMEDDIR/$NAMEDCA"
		fi
		echo ";
; BIND data file for initial cache data for root domain servers.
;
.		99999999	IN	NS	    ns.nic.ddn.mil.
.		99999999	IN	NS	    ns.nasa.gov.
.		99999999	IN	NS	    terp.umd.edu.
.		99999999	IN	NS	    a.isi.edu.
.		99999999	IN	NS	    aos.brl.mil.
.		99999999	IN	NS	    gunter-adam.af.mil.
.		99999999	IN	NS	    c.nyser.net.
ns.nic.ddn.mil.	99999999	IN	A	    192.67.67.53
ns.nasa.gov.	99999999	IN	A	    128.102.16.10	; BIND
		99999999	IN	A	    192.52.195.10
a.isi.edu.	99999999	IN	A	    26.3.0.103		; Jeeves
		99999999	IN	A	    128.9.0.107
aos.brl.mil.	99999999	IN	A	    128.20.1.2		; BIND
		99999999	IN	A	    192.5.25.82
gunter-adam.af.mil. 99999999	IN	A	    26.1.0.13		; Jeeves
c.nyser.net.	99999999	IN	A	    192.33.4.12		; BIND
terp.umd.edu.	99999999	IN	A	    128.8.10.90		; BIND" > $NAMEDDIR/$NAMEDCA
		;;
	esac
	#
	# Print out message reminder to edit files manually.
	#
	if [ "$auto" = "y" ]
	then
		echo ""
		answer=y
		while [ $answer ]
		do
			echo -n "Would you like bindsetup to start the BIND daemon automatically [y]? "
			answer=""
			read ans
			case $ans in
			[yY]*|"")
				if [ -f $NAMEDPID ]
				then
					kill -9 `cat $NAMEDPID`
				fi
				/usr/etc/named $KRBFLAGS$NAMEDBO
				egrep -s "/usr/etc/hesupd" $RCFILE
				if [ $? -eq 0 ]
				then
					/usr/etc/hesupd
				fi
				;;
			[nN]*)
				;;
			*)
				answer=y
				;;
			esac
		done
	fi
esac
if [ $verbose ]
then
	if [ $PRINTMAIL ]
	then
		echo "
The $SENDMAIL file has been modified to contain the BIND domain
name, $binddomain.  You must kill the sendmail process, freeze
the sendmail configuration file, and then start sendmail up again
in order to send mail from this system.  See the sendmail(8) manual
page for more information."
	fi
	echo "
BIND is not used on this system until the $SVC file is edited
and \"bind\" is added to the database lists.  Failure to do so results
in the continued use of the current $SVC file.  Please run
$SVCSETUP or edit $SVC manually.  See svcsetup(8)."
fi
if [ ! -f $CLIENTROOT$SVCORDER ]
then
	echo "#
#       The /etc/svcorder file designates the order and selection of
#       ULTRIX name services to be queried in the resolution of host
#       names and addresses.  This file is not required for /etc/hosts
#       (local) access, but is required to access host names from
#       database lookup services such as Yellow Pages and BIND.
#
#       Note that additional preparation is required to set up each
#       service.  Consult the documentation for further information.
#
#

local
bind" > $CLIENTROOT$SVCORDER
fi
#
# Clean up
#
if [ -r $RESTMP ]
then
	rm $RESTMP
fi
if [ -r $HOSTSTMP ]
then
	rm $HOSTSTMP
fi
if [ -r $BINDTMP ]
then
	rm $BINDTMP
fi
if [ $verbose ]
then
	echo ""
	echo "***** BINDSETUP COMPLETE *****"
fi
exit 0
