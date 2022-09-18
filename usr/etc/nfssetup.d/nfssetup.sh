#! /bin/sh
#  @(#)nfssetup.sh	4.2	(ULTRIX)	10/16/90
#									
# 			Copyright (c) 1986 by				
# 		Digital Equipment Corporation, Maynard, MA		
# 			All rights reserved.				
# 									
#    This software is furnished under a license and may be used and	
#    copied  only  in accordance with the terms of such license and	
#    with the  inclusion  of  the  above  copyright  notice.    This	
#    software  or  any  other copies thereof may not be provided or	
#    otherwise made available to any other person.   No title to and	
#    ownership of the software is hereby transferred.			
# 									
#    The information in this software is subject to change  without	
#    notice  and should not be construed as a commitment by Digital	
#    Equipment Corporation.						
# 									
#    Digital assumes no responsibility for the use  or  reliability	
#    of its software on equipment which is not supplied by Digital.	
#
# Purpose:	Set up NFS environment
# Usage:	nfssetup [client] [server]
# Environment:	Bourne shell script
# Date:		6/11/86
# Author:	Fred L. Templin
# 
# Remarks:
#    Sets up files:
#	/etc/rc.local
#	/etc/exports
#	/etc/fstab
#
#    Much of this has been borrowed from "netsetup.sh"
#

#
# Modification History:
#
#       03-Dec-89 lebel
#               Added -i option to mountd for ip address checking
#
#	28-Jul-88 Fred Glover
#		Add newlines, error checking for #nfsds, #biods
#
#	16-Feb-88 fglover
#		Add support for NFS locking
#

#
# Set up interrupt handlers:
#
QUIT='
	if [ -r $EXTMP ]
	then
		rm $EXTMP
	fi
	if [ -r $FSTMP ]
	then
		rm $FSTMP
	fi
	if [ -r $RCTMP ]
	then
		rm $RCTMP
	fi
	echo "Nfssetup terminated with no installations made."
	exit 1
'
#
# Trap ^c signal, etc.
#
trap 'eval "$QUIT"' 1 2 3 15

EXTMP=/tmp/nfssetup.ex.$$
FSTMP=/tmp/nfssetup.fs.$$
RCTMP=/tmp/nfssetup.rc.$$
VMUNIX=/vmunix
RCFILE=/etc/rc.local
FSFILE=/etc/fstab
EXFILE=/etc/exports
NFILE=/etc/networks
NFSSETUP=/etc/nfssetup
LOCAL_KEY="echo -n 'local daemons:'"
NFSSTART_KEY="# %NFSSTART% - NFS daemons added by \"nfssetup\""
NFSEND_KEY="# %NFSEND%"
NFSLOCKSTART_KEY="# %NFSLOCKSTART%"
NFSLOCKEND_KEY="# %NFSLOCKEND%"
USR_BIN=/usr/bin

nnfsd=0
nbiod=4
rwall=n
rlock=n
serving=""
startup=""
verbose=y
first_time=y
full_config=n


#
# PHASE ONE: Gather data!!
#

if [ $1 ]
then
	#
	# Set up default environments for client or server. Runs fast
	# and silent for ease of use in other scripts.
	#
	if [ $1 = "client" ]
	then
		verbose=""
		nbiod=4
	elif [ $1 = "server" ]
	then
		verbose=""
		nnfsd=4
		nbiod=4
		serving=y
	else
		echo "usage: nfssetup [ client ] [ server ]"
		eval "$QUIT"
	fi

	#
	# Require it to be run by root
	#
	if [ \! -w $RCFILE ]
	then
		exit 1
	fi

	#
	# Run it multi-user
	#
	if [ \! -d $USR_BIN ]
	then
		exit 1
	fi

	#
	# See if this is a re-install
	#
	egrep -s "$NFSSTART_KEY" $RCFILE
	if [ $? -eq 0 ]
	then
		egrep -s "$NFSEND_KEY" $RCFILE
		if [ $? -ne 0 ]
		then
			exit 1
		fi
		first_time=""
	fi
else
	if [ \! -w $RCFILE ]
	then
		echo "Please su to root first."
		eval "$QUIT"
	fi

	if [ \! -d $USR_BIN ]
	then
		echo "
Please bring the system to multi-user mode before running nfssetup."
		eval "$QUIT"
	fi

	egrep -s "$NFSSTART_KEY" $RCFILE
	if [ $? -eq 0 ]
	then
		egrep -s "$NFSEND_KEY" $RCFILE
		if [ $? -ne 0 ]
		then
			echo "
The network file system has already been installed but can not
be reconfigured automatically. To change the current NFS config-
uration, edit the file $RCFILE to remove the old NFS environment
and run nfssettup again."
			eval "$QUIT"
		fi
		echo "
The network file system has already been installed.  Would
you like to change the current NFS configuration?"
		again=y
		while [ $again ]
		do
			again=""
			echo -n "
Enter \"y\" or \"n\" [n]: "
			read ans
			case $ans in
			[yY]*)
				first_time=""
				;;
			[nN]*|"")
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
		echo "Checking kernel configuration..."
		nm $VMUNIX | grep -s 'nfs_namei' 
		if [ $? -ne 0 ]
		then
			echo "
In order to make use of the network file system (NFS) services,
you must first configure the NFS support code into your ULTRIX-32
kernel.  Please consult the System Management Guide for information
on how to configure and bootstrap the new ULTRIX-32 kernel."
			eval "$QUIT"
		fi
		echo "
The nfssetup command configures the network file system (NFS)
environment for your system.  All systems using NFS facilities
must run the Remote Procedure Call (RPC) port mapper daemon.
An entry for this daemon is placed in the /etc/rc.local file
along with entries for the optional daemons you select."
	else
		echo "
Changing the current NFS configuration."
	fi

	echo "
You will be asked a series of questions about your system.
Default answers are shown in square brackets ([]).  To use a
default answer, press the RETURN key."
	#
	# Ask if NFS locking should be enabled
	#
	echo ""

	echo "
	Local locking supports local file and file region locking. Local
	locking is the default.  NFS locking supports local and remote (NFS) 
	file and file region locking. If you would like to enable the NFS
	locking functionality, then answer 'y' to the following question."

	again=y
	while [ $again ]
	do
		again=""
		echo ""
		echo -n "	NFS locking to be enabled [n] ? "
		read rlock
		case $rlock in
		[yY]*)
			rlock=y
			;;
		[nN]*|"")
			rlock=n
			;;
		*)
			again=y
			;;
		esac
	done

	#
	# Ask if he wants to run a full NFS Configuration...
	#
	if [ -z "$first_time" ]
	then
	again=y
	while [ $again ]
	do
		again=""
		echo ""
		echo -n "	Would you like to change the rest of your NFS configuration [n] ? "
		read full_config
		case $full_config in
		[yY]*)
			full_config=y
			;;
		[nN]*|"")
			full_config=n
			;;
		*)
			again=y
			;;
		esac
	done
	if [ $full_config = n ]
	then
	#
	# Copy curent rc.local to temp file
	# stripping %NFSLOCKSTART% to %NFSLOCKEND%
	#
	cat $RCFILE | sed "/%NFSLOCKSTART%/,/%NFSLOCKEND%/ d" > $RCTMP
	cp $RCTMP $RCFILE
	rm $RCTMP
	if [ $rlock = y ]
	then
	echo $NFSLOCKSTART_KEY >> $RCTMP
	echo "echo -n 'NFS Locking: '						>/dev/console" >> $RCTMP
	echo "[ -f /usr/etc/nfssetlock ] && {
	/usr/etc/nfssetlock on & echo -n 'enabled; '	>/dev/console
}" >> $RCTMP
	echo "[ -f /usr/etc/statd ] && {
	/usr/etc/statd & echo -n 'daemons: statd '			>/dev/console
}" >> $RCTMP
	echo "[ -f /usr/etc/lockd ] && {
	/usr/etc/lockd & echo 'and lockd'				>/dev/console
}" >> $RCTMP
	echo $NFSLOCKEND_KEY >> $RCTMP
	ed - $RCFILE << END >> /dev/null
/%NFSEND%/-1
.r $RCTMP
-
w
q 	
END
	rm $RCTMP
        else
	ed - $RCFILE << END >> /dev/null
/$NFSLOCKSTART_KEY/,/$NFSLOCKEND_KEY/ d
-
w
q 	
END
	fi
	echo
	echo -n "Nfssetup terminated with NFS locking "
	if [ $rlock = y ]
	then
		echo "enabled"
	else
		echo "disabled"
	fi	
	echo "and prior NFS configuration maintained."
	echo
	exit 0
	fi
	#
	# End of if not full config
	#
	fi
	#
	# End of if not first time
	#

	#
	#   Determine state of this NFS machine.  PURE client (no exports made),
	# or exporter.
	#
	again=y
	while [ $again ]
	do
		again=""
		echo ""
		echo -n "	Will you be exporting any directories [n] ? "
		read serving
		case $serving in
		[yY]*)
			serving=y
			;;
		[nN]*|"")
			serving=""
			;;
		*)
			again=y
			;;
		esac
	done

	if [ $serving ]
	then
	#
	# Since we'll be serving up directories, need to put "nfsd"
	# in the local rc script along with the others.  We'll ask
	# to see if the user wants more or less daemons than usual.
	#
		nnfsd=4
		echo ""
		if [ $first_time ]
		then
			echo "
	Systems that export NFS directories must run /etc/nfsd to
	handle NFS requests from clients.  You can configure up
	to 20 nfsd daemons, but for average workload situations,
	4 is a good number to run."
		fi
		flag=y
		while [ $flag ]
		do
			flag=""
			echo ""
			echo -n "	Enter the number of nfsd servers to run [4] : "
			read num
			if [ $num ]
			then
				if [ $num -le 0 ]
				then
					flag=y
					echo "	Number must be greater than zero"
				elif [ $num -ge 20 ]
				then
					nnfsd=20
				else
					nnfsd=$num
				fi
			fi
		done

	fi
	#
	# Ask if he wants any "biod" daemons.
	#
	if [ $first_time ]
	then
		echo ""
		echo "
	NFS clients can use block I/O daemons for buffering
	data transfers, although their use is not required.
	You can configure up to 5 biod daemons on your system
	based upon the workload you expect, but for average
	workload situations, 4 is a good number to run."
	fi
	flag=y
	while [ $flag ]
	do
		flag=""
		echo ""
		echo -n "	Enter the number of block I/O daemons to run [4] : "
		read num
		if [ $num ]
		then
			if [ $num -lt 0 ]
			then
				flag=y
				echo "	Number must be greater than or equal zero"
			elif [ $num -ge 5 ]
			then
				nbiod=5
			else
				nbiod=$num
			fi
		fi
	done

	#
	# Ask if he wants to run the rwalld daemon...
	#
	if [ $first_time ]
	then
		echo ""
		echo "
	NFS clients that rely heavily on having certain NFS
	directories mounted may wish to be notified in the
	event of NFS servers going down.  In order for users
	on your system to receive notifications, you must run
	the remote wall daemon. (rwalld)"
	fi
	again=y
	while [ $again ]
	do
		again=""
		echo ""
		echo -n "	Would you like to run the rwalld daemon [n] ? "
		read rwall
		case $rwall in
		[yY]*)
			rwall=y
			;;
		[nN]*|"")
			rwall=n
			;;
		*)
			again=y
			;;
		esac
	done

	#
	# He's exporting directories.  Find out which ones and validate them
	# but don't add them to "/etc/exports" just yet!
	#
	if [ $serving ]
	then
		if [ $first_time ] || [ \! -f $EXFILE ]
		then
			echo "
You are now setting up your directory export list.  Enter the
full pathnames of the directories to be exported.  For each
pathname, enter the network group names and/or machine names to
be given access permission to this directory, or a null list to
indicate general permission.  (Network groups are ONLY available
on machines using Yellow Pages).  This information is placed in the
/etc/exports file.  Press the RETURN key to terminate the pathname
and permissions lists."
			more_paths=y
		else
			echo "
	Would you like to add any directory pathnames to the ${EXFILE} file?"
			again=y
			while [ $again ]
			do
				again=""
				echo -n "
	Enter \"y\" or \"n\" [n]: "
				read ans
				case $ans in
					[yY]*)
					more_paths=y
					;;
					[nN]*|"")
					more_paths=""
					;;
					*)
					again=y
					;;
				esac
			done
		fi
		while [ $more_paths ]
		do
			more_paths=""
			permlist=""
			echo ""
			echo -n "Enter the directory pathname: "
			read dirname
			if [ $dirname ]
			then
				more_paths=y
				if [ -d $dirname ]
				then
					more_perms=y
					while [ $more_perms ]
					do
						more_perms=""
						echo -n "	Netgroup/Machine name: "
						read permname
						if [ -n "$permname" ]
						then
							more_perms=y
							permlist=`echo $permlist $permname | cat`
						fi
					done
					echo "$dirname		$permlist" >> $EXTMP
				else 
					echo "
The pathname: ${dirname}
is not a valid directory.
"
				fi
			else
			echo "Directory export list complete..."
			fi
		done
	fi

	#
	# Find out which file systems from which machines are to be imported.
	#
	if [ $first_time ]
	then
		more_hosts=y
		echo "
You will now be asked to provide information about the remote file
systems you wish to access.  First list the name of the remote host
serving the directories you wish to mount, then give the full directory
pathnames.  Also, for each remote directory, you must specify the full
directory pathname of the mount point on the local machine and whether
the mount is read-only or read-write.  (Nfssetup will create the mount
point directory if it does not already exist.)  Press the RETURN key to
terminate the host and directory pathname lists:"
	else
		echo "
	Would you like to add any remote file systems to be mounted?"
		again=y
		while [ $again ]
		do
			again=""
			echo -n "
	Enter \"y\" or \"n\" [n]: "
			read ans
			case $ans in
				[yY]*)
				more_hosts=y
				;;
				[nN]*|"")
				more_hosts=""
				;;
				*)
				again=y
				;;
			esac
		done
	fi
	newdirs=""
	while [ $more_hosts ]
	do
		more_hosts=""
		echo ""
		echo -n "Enter the remote host name: "
		read hostid
		if [ $hostid ]
		then
			more_hosts=y
			more_paths=y
			while [ $more_paths ]
			do
				more_paths=""
				echo -n "
	Enter the remote directory pathname: "
				read rdir
				if [ -n "$rdir" ]
				then
					more_paths=y
					again=y
					while [ $again ]
					do
						again=""
						echo -n "	Enter the local mount point: "
						read ldir
						if [ -z "$ldir" ]
						then
							again=y
						elif [ -f $ldir ]
						then
							echo "
	${ldir}: File exists! Please choose a new mount point."
							again=y
						elif [ \! -d $ldir ]
						then
							echo "
	${ldir}: Directory does not exist, but will be created."
							newdirs=`echo $newdirs $ldir | cat`
						fi
					done

					again=y
					while [ $again ]
					do
						again=""
						echo -n "	Is this a read-only mount [y] ? "
						read readonly
						case $readonly in
						[nN]*)
							echo "${rdir}@${hostid}:${ldir}:rw:0:0:nfs:bg:" >> $FSTMP
							;;
						[yY]*|"")
							echo "${rdir}@${hostid}:${ldir}:ro:0:0:nfs:bg:" >> $FSTMP
							;;
						*)
							again=y
							;;
						esac
					done
				fi
			done
		else
			echo "Remote directory mount list complete..."
		fi
	done

	#
	# Ask user for verification...
	#
	echo "
Please confirm the following information which you
have entered for your NFS environment:
"
	if [ $serving ]
	then
		echo "	${nnfsd} nfsd daemons"
		echo "	${nbiod} biod daemons"
		if [ $rlock = y ]
		then
			echo "	locking daemons installed"
		fi
		if [ $rwall = y ]
		then
			echo "	rwalld daemon installed"
		fi
		if [ -s $EXTMP ]
		then
			echo "
	Directory export list:"
			awk '
			{
				printf "\t\t%s", $1
				if ( NF > 1 ) {
					printf " exported to:"
					for ( i = 2; i <= NF; i++ )
						printf " %s", $i
					printf "\n"
				}
				else
					printf " exported with general permissions\n"
			}' $EXTMP
		else
			echo "
	No directories exported"
		fi
	else
		echo "	${nbiod} biod daemons"
		if [ $rlock = y ]
		then
			echo "	locking daemons installed"
		fi
		if [ $rwall = y ]
		then
			echo "	rwalld daemon installed"
		fi
		echo "
	No directories exported"
	fi
	if [ -s $FSTMP ]
	then
		echo "
	Remote directory mount list:"
		awk '
		BEGIN { FS = ":" }
		{
			if ( $3 == "ro" )
				printf "\t\t%s mounted on: %s (Read Only)\n", $1, $2
			else
				printf "\t\t%s mounted on: %s\n", $1, $2
		}' $FSTMP
	else
		echo "
	No remote directories to mount"
	fi
	again=y
	while [ $again ]
	do
		echo -n "
Enter \"c\" to CONFIRM the information, \"q\" to QUIT nfssetup
without making any changes, or \"r\" to RESTART the procedure [no default]: "
		read conf
		case $conf in
			[qQ]*)
			[ -r $EXTMP ] && rm $EXTMP
			[ -r $FSTMP ] && rm $FSTMP
			eval "$QUIT"
			;;
			[rR]*)
			[ -r $EXTMP ] && rm $EXTMP
			[ -r $FSTMP ] && rm $FSTMP
			exec $NFSSETUP $*
			;;
			[cC]*)
			again=""
			;;
			*)
			again=y
			;;
		esac
	done
fi
#
# PHASE TWO...  Update files!!
#
trap "" 1 2 3 15


if [ $verbose ]
then
	echo ""
	echo "Updating files:"
	echo "	/etc/rc.local"
fi
#
# See if the port mapper is already there.
#
egrep -s '[ 	]*/etc/portmap' $RCFILE
if [ $? -ne 0 ]
then
	#
	# Not there...  put it in!!
	#
	echo "# RPC portmap daemon
echo -n 'RPC port mapper:'					>/dev/console
[ -f /etc/portmap ] && {
	/etc/portmap ; echo ' portmap.'				>/dev/console
}" >> $RCTMP
	startup=`echo $startup "/etc/portmap ; " | cat`
fi

#
# Put in banner...
#
echo $NFSSTART_KEY >> $RCTMP
echo "echo -n 'NFS daemons:'						>/dev/console" >> $RCTMP
#
# Put in mountd IF we're serving...
#
if [ $serving ]
then
echo "[ -f /etc/mountd -a -f /etc/portmap -a -s ${EXFILE} ] && {
	/etc/mountd -i ; echo -n ' mountd -i'				>/dev/console
}" >> $RCTMP
	startup=`echo $startup "/etc/mountd -i; " | cat`
fi
#
# Install optional daemons...
#
if [ $nnfsd -gt 0 ] && [ $nbiod -gt 0 ]
then
	echo "[ -f /etc/nfsd -a -f /etc/portmap ] && {
	/etc/nfsd ${nnfsd} ; echo -n ' nfsd'				>/dev/console
}
[ -f /etc/biod ] && {
	/etc/biod ${nbiod} ; echo ' biod'				>/dev/console
}" >> $RCTMP
	startup=`echo $startup "/etc/nfsd ${nnfsd} ; /etc/biod ${nbiod} ; " | cat`
elif [ $nnfsd -gt 0 ]
then
	echo "[ -f /etc/nfsd -a -f /etc/portmap ] && {
	/etc/nfsd ${nnfsd} ; echo ' nfsd'				>/dev/console
}" >> $RCTMP
	startup=`echo $startup "/etc/nfsd ${nnfsd} ; " | cat`
elif [ $nbiod -gt 0 ]
then
	echo "[ -f /etc/biod ] && {
	/etc/biod ${nbiod} ; echo ' biod'				>/dev/console
}" >> $RCTMP
	startup=`echo $startup "/etc/biod ${nbiod} ; " | cat`
fi
if [ $rlock = y ]
then
	echo $NFSLOCKSTART_KEY >> $RCTMP
	echo "echo -n 'NFS Locking: '						>/dev/console" >> $RCTMP
	echo "[ -f /usr/etc/nfssetlock ] && {
	/usr/etc/nfssetlock on & echo -n 'enabled; '	>/dev/console
}" >> $RCTMP
	echo "[ -f /usr/etc/statd ] && {
	/usr/etc/statd & echo -n 'daemons: statd '			>/dev/console
}" >> $RCTMP
	echo "[ -f /usr/etc/lockd ] && {
	/usr/etc/lockd & echo 'and lockd'				>/dev/console
}" >> $RCTMP
	echo $NFSLOCKEND_KEY >> $RCTMP
fi
if [ $rwall = y ]
then
	echo "[ -f /usr/etc/rwalld -a -f /etc/portmap ] && {
	/usr/etc/rwalld ; echo 'rwall daemon: rwalld'		>/dev/console
}" >> $RCTMP
	startup=`echo $startup "/usr/etc/rwalld ; " | cat`
fi
echo "if [ \! \"\$DISKLESS\" ]
then
	echo -n 'mounting NFS directories:'			>/dev/console
	/etc/nfs_umount -b					>/dev/null 2>&1
	/etc/mount -a -t nfs				      >/dev/console 2>&1
	echo ' done.'						>/dev/console
fi" >> $RCTMP
echo $NFSEND_KEY >> $RCTMP

if [ $first_time ]
then

	egrep -s "$LOCAL_KEY" $RCFILE
	if [ $? -eq 0 ]
	then
		ed - $RCFILE << END >> /dev/null
/$LOCAL_KEY
-
.r $RCTMP
w
q
END
	else
		cat $RCTMP >> $RCFILE
	fi
else
	ed - $RCFILE << END >> /dev/null
/$NFSSTART_KEY/,/$NFSEND_KEY/ d
-
.r $RCTMP
w
q
END
fi
rm $RCTMP

if [ $verbose ]
then
	#
	# Update export list
	#
	if [ -r $EXTMP ]
	then
		echo "	/etc/exports"
		cat $EXTMP >> $EXFILE
		rm $EXTMP
	fi

	#
	# Update fstab
	#
	if [ -r $FSTMP ]
	then
		echo "	/etc/fstab"
		cat $FSTMP >> $FSFILE
		rm $FSTMP
	fi

	#
	# Make new local mount point directories...
	#
	if [ -n "$newdirs" ]
	then
		echo ""
		echo "Creating local mount points:"
	fi
	for dirname in $newdirs
	do
		object=""
		for subdirs in `echo $dirname | awk '
		BEGIN { FS = "/" }
		{
			if (substr($0,1,1) != "/")
				print $1
			for (i = 2; i <= NF; ++i) print "/"$i
		}'` 
		do
			object=$object$subdirs
			if [ -f $object ]
			then
				echo "	Can't create ${object}. File exists!" 
				break
			fi
			if [ \! -d $object ]
			then
				mkdir $object 2> /dev/null
				if [ $? -ne 0 ]
				then
					echo "	Can't create ${object}. Mkdir failed!"
					break
				fi
			fi
		done
		echo "	"$dirname
	done
	if [ $first_time ]
	then
		if [ -n "$startup" ]
		then
			echo "
The necessary NFS daemon entries have been placed in the file ${RCFILE}.
In order to begin using NFS, you must now start the daemons and mount
any remote directories you wish to access.  You may either allow nfssetup
to start these daemons automatically or invoke them by hand, but in either
case they will be started automatically on subsequent reboots.
"
			answer=y
			while [ $answer ]
			do
				echo -n "
Would you like nfssetup to start the daemons automatically [y]? "
				answer=""
				read ans
				case $ans in
				[yY]*|"")
					eval "$startup"
					echo "
The NFS daemons for your machine have been started. In order to mount the
remote directories you wish to access, type the following command after
exiting from nfssetup:

	# /etc/mount -a -t nfs

"
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
The necessary NFS daemon entries have been placed in the file ${RCFILE}.
You may start any additional NFS daemons added by hand, or allow the new
NFS configuration to take effect on the next reboot."
	fi
	echo ""
	echo "***** NFSSETUP COMPLETE *****"
fi
exit 0
