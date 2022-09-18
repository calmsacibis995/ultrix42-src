#! /bin/sh
# @(#)ypsetup.sh	4.3   (ULTRIX)        9/7/90
#									
# 			Copyright (c) 1986 by				
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
# Purpose:	Set up Yellow Pages
# Usage:	ypsetup [ {domainname} client [ {file} ] ]
# Environment:	Bourne shell script
# Date:		6/30/86
# Author:	Fred L. Templin
# 
# Remarks:
#    Sets up files:
#	/etc/rc.local
#	/usr/lib/crontab
#	/etc/yp/{domainname}/*
#	/etc/svcorder
#
#
# Modification History
#
#	9/06/90  -- terry
#		Added message to remind user that master files should be
#		in the directory /var/yp/src.
#		Modified yppasswdd information to use /var/yp/src instead
#		of /etc.
#
#	7/17/90  -- terry
#		Added -X option (initial bind) to ypbind.
#
#       12/1/89  -- terry
#		Added code to remove /etc/yp/src/passwd.tmp on boot if it
#		exists.  
#
#	10/17/89 -- terry
#		Added -S option to ypbind.
#
#	07/24/89 -- sue
#		Added message to tell user to edit the $SVC file.
#
#	11/10/88 -- logcher
#		Updated with V3.0 changes.
#
#	08/29/88 -- logcher
#		Added "local" to svcorder file for consistency
#
#	06/13/88 -- logcher
#		Changed silent client adding to take the root directory
#		pathname and modify /etc files rc.local and svcorder
#		relatively, on the client.
#
#	04/18/88 -- logcher
#		Modified yp check in /etc/svcorder file for ^yp and ^YP
#
#	02/24/88 -- fglover
#		Modify editorial text for /etc/svcorder file
#
#	01/26/88 -- fglover
#		Add support for /etc/svcorder file
#

#
# Set up interrupt handlers:
#
QUIT='
	if [ -r $YPTMP ]
	then
		rm $YPTMP
	fi
	if [ -r $RCTMP ]
	then
		rm $RCTMP
	fi
	if [ -r $CRTMP ]
	then
		rm $CRTMP
	fi
	echo "Ypsetup terminated with no installations made."
	exit 1
'
QMSG='
	echo "Please clean up ${YPDIR}/${ypdomain} !!"
	eval "$QUIT"
'

#
# Trap ^c signal, etc.
#
trap 'eval "$QUIT"' 1 2 3 15

# files
YPTMP=/tmp/ypsetup.yp.$$
RCTMP=/tmp/ypsetup.rc.$$
CRTMP=/tmp/ypsetup.cr.$$
HOSTS=/etc/hosts
YPDIR=/etc/yp
YPLOG=ypxfr.log
RCFILE=/etc/rc.local
CRFILE=/usr/lib/crontab
NULL=/dev/null
SVCORDER=/etc/svcorder
SVCSETUP=/usr/etc/svcsetup
SVC=/etc/svc.conf
CLIENTROOT=""

# commands
MAKE="make -f /etc/yp/Makefile"
MAKEDBM=/etc/yp/makedbm
YPXFR=/etc/yp/ypxfr
YPSETUP=/etc/ypsetup

# defines
LOCAL_KEY="echo -n 'local daemons:'"
YPSTART_KEY="# %YPSTART% - Yellow Pages daemons added by \"ypsetup\""
YPEND_KEY="# %YPEND%"
NFSSTART_KEY="# %NFSSTART% - NFS daemons added by \"nfssetup\""
PATH=$PATH:$YPDIR
export PATH
DEFMAPS="group.bygid group.byname hosts.byaddr hosts.byname \
mail.aliases netgroup netgroup.byuser netgroup.byhost networks.byaddr \
networks.byname passwd.byname passwd.byuid protocols.byname protocols.bynumber \
services.byname ypservers"
S_OPT=""

ypdomain=""
startup=""
flavor=""
verbose=y
first_time=y
scratch_maps=y

#
# PHASE ONE: Gather data!!
#
if [ $1 ]
then
	#
	# Run fast and silent for DMS client setup.
	#
	ypdomain=$1
	if [ $2 ] && [ $2 = "client" ]
	then
		verbose=""
		flavor=c
		if [ $3 ] 
		then
			CLIENTROOT=$3
		fi
		#
		# Require it to be run by root
		#
		if [ \! -w $CLIENTROOT$RCFILE ]
		then
			exit 1
		fi

		#
		# Run it multi-user
		#
		hname=`hostname`
		if [ $? -ne 0 ] || [ \! -d $YPDIR ]
		then
			exit 1
		fi

		#
		# See if this is a re-install
		#
		egrep -s "$YPSTART_KEY" $CLIENTROOT$RCFILE
		if [ $? -eq 0 ]
		then
			egrep -s "$YPEND_KEY" $CLIENTROOT$RCFILE
			if [ $? -ne 0 ]
			then
				exit 1
			fi
			first_time=""
		fi
	else
		echo "usage: ypsetup [ {domainname} client [ {file} ]  ]"
		eval "$QUIT"
	fi
fi

if [ $verbose ]
then
	#
	# Require it to be run by root
	#
	if [ \! -w $CLIENTROOT$RCFILE ]
	then
		echo "Please su to root first."
		eval "$QUIT"
	fi

	#
	# Be sure network has already been set up, and this baby has a name!!
	#

	hname=`hostname`
	if [ $? -ne 0 ] || [ \! -d $YPDIR ]
	then
		echo "
Please bring the system to multi-user mode before running ypsetup."
		eval "$QUIT"
	fi

	#
	#
	# See if this is a re-install
	#
	egrep -s "$YPSTART_KEY" $CLIENTROOT$RCFILE
	if [ $? -eq 0 ]
	then
		egrep -s "$YPEND_KEY" $CLIENTROOT$RCFILE
		if [ $? -ne 0 ]
		then
			echo "
The Yellow Pages environment for this host has already been installed
but can not be reconfigured automatically. To change the current YP
configuration, edit the file $RCFILE to remove the old YP environment
and run ypsetup again."
			eval "$QUIT"
		fi
		echo "
The Yellow Pages environment for this host has already been installed.
Would you like to change the current YP configuration?"
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

	if [ $first_time ]
	then
		echo -n "
The ypsetup command configures the Yellow Pages (YP) environment
for your system.  Yellow Pages provides a distributed data lookup
service for sharing information between systems on the network.
Information is kept in database files known as YP maps. These maps
are organized in YP domains which are collections of maps located
on certain systems on the network known as servers. For each domain,
there are three flavors of systems on the network:

	- a master YP server is a system which is responsible for
	  maintaining the master copy of the domain's database.
	  There should be ONLY ONE master server for a domain.

	- a slave YP server is a system which periodically receives
	  updated versions of the master server's maps. The slave
	  server can look up and return information in its private
	  collection of maps, and can take over for the master
	  server in the event of a failure.

	- a YP client is a system which has no local copies of the
	  domain's database, but can look up database information
	  by requesting service from a master or slave server.

[ Press the RETURN key to continue ] : "
		read junk
	fi
	echo "
ypsetup will take you through a configuration process to set the
default YP domain name of your system, determine the flavor of
your Yellow Pages environment and construct the default YP map
files for this YP domain.  Default answers are shown in square
brackets ([]).  To use a default answer, press the RETURN key."

	#
	# did he specify a new domainname?
	#
	if [ -n "$ypdomain" ]
	then
		echo "
Using \"$ypdomain\" as the default YP domain name for this system."
	else
		#
		# Ask him for a name...
		#
		again=y
		echo "
In order to use the Yellow Pages service, your host must have a
default YP domain name."
		while [ $again ]
		do
			echo -n "
Please enter a name to be used as the default YP domain name : "
			read ypdomain
			case $ypdomain in
			"")
				;;
			*)
				again=""
				;;
			esac
		done
	fi

	#
	# Set domain name
	#
	domainname $ypdomain

	prompt=y
	echo "
Will you be configuring your system as a master YP server,
slave YP server, or YP client for the domain \"${ypdomain}\" ?"
	while [ $prompt ]
	do
		prompt=""
		echo -n "
Enter \"m\" for MASTER, \"s\" for SLAVE, or \"c\" for CLIENT [c]: "
		read flavor
		case $flavor in
		[mM]*)
			echo "
Before configuring your system as a master YP server, you
must first be sure that NO OTHER SYSTEM ON THE NETWORK IS
CONFIGURED AS A MASTER SERVER FOR THIS DOMAIN!! If a master
YP server is already configured, or you are unsure, please
exit ypsetup now."
			chk=y
			while [ $chk ]
			do
				chk=""
				echo -n "
Enter \"e\" to EXIT or \"c\" to CONTINUE [no default]: "
				read query
				case $query in
				[eE]*)
					eval "$QUIT"
					;;
				[cC]*)
					;;
				*)
					chk=y
					;;
				esac
			done
			echo "
As the master YP server for the domain ${ypdomain}, you may choose
to run the yppasswdd(8yp) server daemon to allow remote password
updates to the master copy of the passwd file (/var/yp/src/passwd). "
			run_yppasswdd=n
			chk=y
			while [ $chk ]
			do
				chk=""
				echo -n "
Would you like to run the yppasswdd daemon [n]? "
				read query
				case $query in
				[yY]*)
					run_yppasswdd=y
					;;
				[nN]*|"")
					;;
				*)
					chk=y
					;;
				esac
			done
			if [ -d $YPDIR/$ypdomain ]
			then
				echo "
The YP maps have already been initialized for the domain
\"${ypdomain}\". If you are changing your current YP config-
uration from a slave server or a client to a master server,
the YP maps MUST be remade!  Please be sure that the YP files
that you will be distributing are in the directory /var/yp/src.

Would you like ypsetup to remake the maps in the ${YPDIR}/${ypdomain} directory? "
				again=y
				while [ $again ]
				do
					again=""
					echo -n "
Enter \"y\", \"n\", or \"e\" to EXIT ypsetup [no default]: "
					read ans
					case $ans in
					[yY]*)
						scratch_maps=y
						;;
					[eE]*)
						eval "$QUIT"
						;;
					[nN]*)
						scratch_maps=""
						;;
					*)
						again=y
						;;
					esac
				done
			fi
			if [ $scratch_maps ]
			then
			echo "
You will now be asked to list the names of other hosts which will
be configured as servers for maps in this YP domain.  (This host
is included in the list by default). This list will be used to name
the recipients of any updates to your hosts YP maps. Enter only the
names of known hosts which have been initialized in the /etc/hosts
file and press the RETURN key to terminate the list:
"
			not_done=y
			while [ $not_done ]
			do
				query=y
				#
				# Enter this host first...
				#
				echo $hname > $YPTMP
				while [ $query ]
				do
					echo -n "	Name of host: "
					read servname
					if [ $servname ]
					then
						good=`cat $HOSTS | awk "
							BEGIN { found = 0 }
							/[ \t]${servname}[ \t]/ { found = 1; print \"y\" }
							END { if ( found == 0 ) print \"n\" }"`
				#
				# had to put this in to recognize hostnames followed by EOL
				#
						good1=`cat $HOSTS | awk "
							BEGIN { found = 0 }
							/[ \t]${servname}$/ { found = 1; print \"y\" }
							END { if ( found == 0 ) print \"n\" }"`
						if [ $good = y ] || [ $good1 = y ]
						then
							if [ $servname = $hname ]
							then
								echo "	Can't name this host!"
								echo ""
							else 
								echo $servname >> $YPTMP
							fi
						else
							echo "	\"$servname\" NOT a known host!"
							echo ""
						fi
					else
						query=""
					fi
				done
		
				#
				# Ask for verification...
				#
				echo "The list of Yellow Pages servers is:"
				awk '
				{
					printf "\t%s\n", $0 
				}' $YPTMP
					echo "
You may now redo this list, exit the setup procedure, or continue.
If you choose to continue, the default set of YP maps for your host
will now be initialized.  Please be sure that the YP files that you will 
be distributing are in the /var/yp/src directory.

THIS PROCEDURE TAKES TIME!!"
				query=y
				while [ $query ]
				do
					echo -n "
Enter \"r\" to REDO the servers list, \"e\" to EXIT ypsetup
procedure,  or \"c\" to CONTINUE [no default]: "
					read resp
					case $resp in
					[cC]*)
						not_done=""
						query=""
						;;
					[rR]*)
						query=""
						;;
					[eE]*)
						eval "$QUIT"
						;;
					*)
						;;
					esac
				done
			done
			
			trap 'eval "$QMSG"' 1 2 3 15

			#
			# Create Yellow Pages Domain
			#
			if [ $scratch_maps ]
			then
				rm -rf $YPDIR/$ypdomain
			fi
			mkdir $YPDIR/$ypdomain
			if [ $? -ne 0 ]
			then
				echo "
Couldn't create the Yellow Pages directory: ${YPDIR}/${ypdomain}."
				eval "$QUIT"
			fi

			#
			# Make default maps...
			#
			echo "
Making default YP maps. Please wait..."
			$MAKEDBM $YPTMP $YPDIR/$ypdomain/ypservers
			if [ $? -ne 0 ]
			then
				echo "
Couldn't make the \"ypservers\" map"
				eval "$QMSG"
			fi
			(cd $YPDIR/$ypdomain; $MAKE NOPUSH=1)
			if [ $? -ne 0 ]
			then
				echo "
Couldn't make the default YP maps"
				eval "$QMSG"
			fi
			echo "Installation of default maps complete..."
			fi
			;;
		[sS]*)

			if [ -d $YPDIR/$ypdomain ]
			then
				echo "
The YP maps have already been initialized for the domain
\"${ypdomain}\". If you are changing your current YP config-
uration from a master server or a client to a slave server,
the YP maps MUST be remade! Would you like ypsetup to remake
the maps in the ${YPDIR}/${ypdomain} directory?
"
				again=y
				while [ $again ]
				do
					again=""
					echo -n "
Enter \"y\", \"n\", or \"e\" to EXIT ypsetup [no default]: "
					read ans
					case $ans in
					[yY]*)
						scratch_maps=y
						;;
					[eE]*)
						eval "$QUIT"
						;;
					[nN]*)
						scratch_maps=""
						;;
					*)
						again=y
						;;
					esac
				done
			fi
			if [ $scratch_maps ]
			then
			echo "
Before configuring your system as a slave YP server, you must
first know the name of the master YP server for domain \"${ypdomain}\"
and be sure that it is up.  If no master YP server is configured,
you will not be able to retrieve a set of YP maps for your system."
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
			# get master server's name.
			#
			answer=y
			while [ $answer ]
			do
				echo -n "
Please enter the name of the master YP server for domain \"${ypdomain}\": "
				read master
				if [ $master ]
				then
					good=`cat $HOSTS | awk "
						BEGIN { found = 0 }
						/[ \t]${master}[ \t]/ { found = 1; print \"y\" }
						END { if ( found == 0 ) print \"n\" }"`
					good1=`cat $HOSTS | awk "
						BEGIN { found = 0 }
						/[ \t]${master}$/ { found = 1; print \"y\" }
						END { if ( found == 0 ) print \"n\" }"`
					if [ $good = y ] || [ $good1 = y ]
					then
						if [ $master = $hname ]
						then
							echo "	Can't name this host!"
							echo ""
						else
							answer=""
						fi
					else
						echo "	\"${master}\" NOT a known host!"
						echo ""
					fi
				fi
			done
		
			trap 'eval "$QMSG"' 1 2 3 15

			#
			# Create Yellow Pages Domain
			#
			if [ $scratch_maps ]
			then
				rm -rf $YPDIR/$ypdomain
			fi
			mkdir $YPDIR/$ypdomain
			if [ $? -ne 0 ]
			then
				echo "
Couldn't create the Yellow Pages directory: ${YPDIR}/${ypdomain}."
				eval "$QUIT"
			fi

			#
			# Copy master's maps to slave
			#
			echo "
Copying YP maps for this domain from host \"${master}\".
THIS PROCEDURE TAKES TIME!!"
			noerrs=y
			for mname in $DEFMAPS
			do
				$YPXFR -h $master -c -d $ypdomain $mname 2>> $YPDIR/$YPLOG
				if [ $? -ne 0 ]
				then
					echo "	Couldn't transfer map \"${mname}\""
					noerrs=n
				fi
			done
			if [ $noerrs = n ]
			then
				echo "
Some YP maps were not initialized. See the file \"/etc/yp/ypxfr.log\"
for reasons."
			fi
			fi
			;;
		[cC]*|"")
			#
			echo "
Before configuring your system as a YP client, you should
first be sure that there IS at least one system on the
network configured as either a master or slave YP server
for this domain!! If no server is configured, you will not
be able to access the YP maps!"
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
			;;
		*)
			#
			# Bad response...
			#
			prompt=y
			;;
		esac
	done

	#
	# Security (-S) option
	#
	echo "
The -S option locks the domain name and servers list.  A 
single domain name followed by up to four servers can be specified.
Once this option is run the machine will not switch domains
and will only use the servers specified. All the specified
servers must be in the /etc/hosts file."
	
        okay=y
	while [ $okay ]
	do
		okay=""
		echo -n " 
Would you like to add the -S option to ypbind [n] ? "
		read s_option
        	case $s_option in
		[yY]*)
           		S_OPT="-S $ypdomain"
                        bad=y
			while [ $bad ]
			do
				bad=""
	                	echo -n "
How many servers do you wish to specify [1] ? "
               			read num_serv
				case $num_serv in
   				"")
					num_serv=1
					;;
				[1-4])
					;;
				*)
					bad=y
					;;
				esac
			done
				orig=`expr $num_serv + 1`
                     		while [ $num_serv -gt 0 ]
				do 
                                	echo -n " 
Server `expr $orig - $num_serv` name: " 
					read serv_name
					S_OPT=${S_OPT},$serv_name 
				   	num_serv=`expr $num_serv - 1`
				done
				;;
		[nN]*|"")
			;;
		*)
			okay=y
			;;
		esac
	done

	#
	# Initial Bind Option (-X)
	#
	echo "
The Initial Bind option (-X) binds YP to a server at boot time.  
YP will exit after several minutes if this initial binding fails. 
If the Initial Bind option is not specified, your system may hang
on boot or login if a YP server is not available."
	
	okay=y
	while [ $okay ]
	do
		okay=""
		echo -n " 
Would you like to use the Initial Bind option with ypbind [n] ?"
		read s_option
        	case $s_option in
		[yY]*)
			X_OPT="-X "
			;;
		[nN]*|"")
			;;
		*)
			okay=y
			;;
		esac
	done
fi

#
# PHASE TWO... Update files!!
#
trap "" 1 2 3 15

if [ $verbose ]
then
	echo "
Installing Yellow Page daemons..."
fi
#
# See if the port mapper is already there...
#
egrep -s '[ 	]*/etc/portmap' $CLIENTROOT$RCFILE
if [ $? -ne 0 ]
then
	#
	# Not there... put it in!!
	#
	echo "# RPC portmap daemon
echo -n 'RPC port mapper:'					>/dev/console
[ -f /etc/portmap ] && {
	/etc/portmap ; echo ' portmap.'				>/dev/console
}" >> $RCTMP
	startup=`echo $startup "/etc/portmap ; " | cat`
fi

#
# Banner, domainname, and ypbind...
#
echo $YPSTART_KEY >> $RCTMP
echo "/bin/domainname	${ypdomain}" >> $RCTMP
case $flavor in
[mM]*)
	echo "echo -n 'YP daemons:'						>/dev/console
[ -f /etc/portmap -a -f /usr/etc/ypserv ] && {
	/usr/etc/ypserv ; echo -n ' ypserv'			>/dev/console
}
[ -f /etc/portmap -a -f /etc/ypbind ] && {
	/etc/ypbind $S_OPT $X_OPT; echo -n ' ypbind $S_OPT $X_OPT'			>/dev/console
}" >> $RCTMP
	startup=`echo $startup "/usr/etc/ypserv ; /etc/ypbind $S_OPT $X_OPT; " | cat`
	if [ $run_yppasswdd = y ]
	then
		echo "
[ -s /etc/yp/src/passwd -a -f /etc/yp/src/passwd.tmp ] && {
  	rm -f /etc/yp/src/passwd.tmp
}
[ -f /etc/portmap -a -f /usr/etc/rpc.yppasswdd ] && {
	/usr/etc/rpc.yppasswdd /var/yp/src/passwd -m passwd \\
	DIR=/var/yp/src ; echo -n ' yppasswdd' >/dev/console
}" >> $RCTMP
		startup=`echo $startup "/usr/etc/rpc.yppasswdd /var/yp/src/passwd -m passwd DIR=/var/yp/src ; " | cat`
	fi
	;;
[sS]*)
	echo "echo -n 'YP daemons:'						>/dev/console
[ -f /etc/portmap -a -f /usr/etc/ypserv ] && {
	/usr/etc/ypserv ; echo -n ' ypserv'			>/dev/console
}
[ -f /etc/portmap -a -f /etc/ypbind ] && {
	/etc/ypbind $S_OPT $X_OPT; echo -n ' ypbind $S_OPT $X_OPT'				>/dev/console
}" >> $RCTMP
	startup=`echo $startup "/usr/etc/ypserv ; /etc/ypbind $S_OPT $X_OPT" | cat`
	#
	# Set updating entries into crontab. (Don't add these for a re-install
	# of a slave server.
	#
	egrep -s '/etc/yp/ypxfr' $CRFILE
	if [ $? -ne 0 ]
	then
		echo "
# Local Yellow Pages environment
30 * * * * sh /etc/yp/ypxfr_1perhour
31 1,13 * * * sh /etc/yp/ypxfr_2perday
32 1 * * * sh /etc/yp/ypxfr_1perday" >> $CRTMP
	fi
	;;
[cC]*|"")
	echo "echo -n 'YP daemons:'						>/dev/console
[ -f /etc/portmap -a -f /etc/ypbind ] && {
	/etc/ypbind $S_OPT $X_OPT; echo -n ' ypbind $S_OPT $X_OPT '				>/dev/console
}" >> $RCTMP
	startup=`echo $startup "/etc/ypbind $S_OPT $X_OPT; " | cat`
	;;
esac

echo "echo '.'							>/dev/console" >> $RCTMP
echo $YPEND_KEY >> $RCTMP

if [ $first_time ]
then
	#
	# If NFS is there, start YP first!!
	#
	egrep -s "$NFSSTART_KEY" $CLIENTROOT$RCFILE
	if [ $? -eq 0 ]
	then
		ed - $CLIENTROOT$RCFILE << END >> $NULL
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
		egrep -s "$LOCAL_KEY" $CLIENTROOT$RCFILE
		if [ $? -eq 0 ]
		then
			ed - $CLIENTROOT$RCFILE << END >> $NULL
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
			cat $RCTMP >> $CLIENTROOT$RCFILE
		fi
	fi
else
	ed - $CLIENTROOT$RCFILE << END >> $NULL
/$YPSTART_KEY/,/$YPEND_KEY/ d
-
.r $RCTMP
w
q
END
fi
rm $RCTMP

if [ $verbose ]
then
	if [ -f $CRTMP ]
	then
		cat $CRTMP >> $CRFILE
		rm $CRTMP
	fi

	if [ $first_time ]
	then
		if [ -n "$startup" ]
		then
			echo "
The necessary Yellow Page daemon entries have been placed in the file
$CLIENTROOT${RCFILE}.  In order to begin using Yellow Pages, you must now
start the daemons and add YP escape characters to the \"/etc\" files
corresponding to the maps for this domain.  You may either allow ypsetup
to start these daemons automatically or invoke them by hand, but in either
case they will be started automatically on subsequent reboots.
"
			answer=y
			while [ $answer ]
			do
				echo -n "
Would you like ypsetup to start the daemons automatically [y]? "
				answer=""
				read ans
				case $ans in
				[yY]*|"")
					if [ $DEBUG ]
					then
						echo $startup
					else
						eval "$startup"
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
	else
		echo "
The necessary Yellow Page daemon entries have been placed in the file
$CLIENTROOT${RCFILE}. You may start any additional YP daemons add by hand,
or allow the new YP configuration to take effect on the next reboot."
	fi
fi

#
# Clean up
#
if [ -r $YPTMP ]
then
	rm $YPTMP
fi
echo "
YP is not used on this system until the $SVC file is edited
and \"yp\" is added to the database lists.  Failure to do so results
in the continued use of the current $SVC file.  Please run
$SVCSETUP or edit $SVC manually.  See svcsetup(8)."

if [ ! -f $CLIENTROOT$SVCORDER ]
then
	echo "
#
#	The /etc/svcorder file designates the order and selection of
#	ULTRIX name services to be queried in the resolution of host
#	names and addresses.  This file is not required for /etc/hosts
#	(local) access, but is required to access host names from 
#	the current ULTRIX Yellow Pages and BIND services.
#
#	Note that additional preparation is required to set up each 
#	name service.  Consult the documentation for further information.
#

yp
local" >> $CLIENTROOT$SVCORDER
fi

if [ $verbose ]
then
	echo ""
	echo "***** YPSETUP COMPLETE *****"
fi
exit 0
