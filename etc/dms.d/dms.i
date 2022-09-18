#!/bin/sh5
# @(#)dms.i	4.4	(ULTRIX)	3/6/91
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
# Abstract:
# There are two functions in this script: 
#	- show diskless env. 
#	- install software
#
# Modification History:
# 
# 001 - Sep 1990 - Tungning Cherng
#	Bug fix.  Installing multiple layered products, the dmsinit can be set.
#
# 000 - Jan 1989 - Tungning Cherng Created
# 	Based on V2.2 DMS

Ticker()
{
	(
		while :
		do
			echo "Working...`date`"
			sleep 120
		done
	)&
	TICKPID=$!
}

:       Unticker -
#
Unticker()
{
	[ -n $TICKPID ] &&
	{
		(
		kill -15 $TICKPID
		wait $TICKPID
		TICKPID=
		)&
	}
}

: descript
#
# show the diskless environment
#
descript()
{
	C_R=$1
	cd $C_R/usr/etc/subsets
	Prod=
	for i in  *.lk
	do
		S_ctrl=`expr $i : '\(.*\).lk'`
		[ -s $S_ctrl.ctrl ] || continue
		D_S=`egrep "NAME=" $S_ctrl.ctrl | 
			sed -e "s/NAME=//" | sed -e "s/${S_ctrl}//"`
		[ -f $C_R/usr/diskless/prodesc ] || 
			> $C_R/usr/diskless/prodesc
		grep -s "${D_S}" $C_R/usr/diskless/prodesc || 
			echo "    $D_S" >> $C_R/usr/diskless/prodesc
	done
}

case $1 in
-s )
	# show the diskless environment
	#
	echo "\nShow the Diskless Environment: "
	index=0
	for i in `ls -d /dlenv*/root*`
	do
		index=`expr $index + 1`
		echo "\n$index  $i"
		echo "	`cat $i/usr/diskless/prodesc`"
	done
	exit 0
	;;
esac
	
: setnetup
setnetup()
{
	trap '' 1 2 3 #don't mess up rc.local&exports
	case `ps x | grep mop_mom | grep -v grep` in
	"" )
		echo "Setting up mop_mom in rc.local"
		echo "/etc/mop_mom &" >>/etc/rc.local
		/etc/mop_mom &
		;;
	esac
	# nfssetup
	case `ps x | grep mountd | grep -v grep` in
	"" )
		echo "Setting up nfs daemons in rc.local"
		/etc/nfssetup server 
	
		[ -f /etc/portmap ] && { 
			/etc/portmap ; echo ' portmap' 
		}
		[ -f /etc/mountd ] && {
			/etc/mountd ; echo  ' mountd' 
		}
		[ -f /etc/nfsd ] && {
			/etc/nfsd 4 ; echo  ' nfsd' 
		}
		[ -f /etc/biod ] && {
			/etc/biod 4 ; echo  ' biod' 
		}
		;;
	esac
	trap 'exit' 1 2 3 
}

: getclients
# check whether has an existing clients
getclients()
{
	##
	# Any clients in the chosen environment.
	##
	DLclients=
	test -d /dlclient* || return 
	for i in /dlclient*
	do
		cd $i 
		for j in `ls -d *.root 2>/dev/null`
		do
			grep -s $INSDIR $j/etc/dlparam &&
			{
				A_A=`expr $j : '\(.*\).root'`
				case $A_A in
				"" )	continue ;;
				esac	
				DLclients="$DLclients $A_A" 
			}
		done
	done
}

menu()
{ 
	while :
	do
		INDEX=0
		for i in $DIRS
		do
			INDEX=`expr $INDEX + 1`
			echo "	$INDEX  $i" 1>&2
		done
		echo "\nEnter your choice : \c" 1>&2
		read CHOICE
		case $CHOICE in
		[1-9]|[1-9][0-9]* )
			if test $CHOICE -ge 1 -a $CHOICE -le $INDEX
			then
				set $DIRS
				eval ansDIR=$"$CHOICE"
				[ -d $ansDIR ] && break
			fi
			;;
		*)	continue ;;
		esac
	done
	echo $ansDIR
}

: CheckDevice
CheckDevice()
{
while :
	do
		echo "
Enter the device special file name or mount point of the distribution 
media, for example, /dev/rmt0h: \c"
		read DEV
		case $DEV in
		"" )	;;
		/dev/* )
			mt -f $DEV rew && break	
			;;
		* )
			[ -d $DEV ] && break
			echo "No such directory $DEV."
			;;
		esac
	done
}
	
: InstallNew
####
#		INSTALL A NEW ROOT AREA	
####
InstallNew()
{
	echo "
You have chosen to install a new diskless environment.
These are the available file systems to contain the environment: \n"
	DIRS=`ls -d /dlenv*`
	case $DIRS in
	"" )	echo "No $DIRS directory available"
		exit 1	;;
	esac
	INSFS=`menu $DIRS`

	INSDIR=$INSFS/root.TMP

	CheckDevice

	trap 'rm -r $INSDIR; exit' 1 2 3 
	mkdir $INSDIR
	mkdir $INSDIR/usr
	mkdir $INSDIR/var
	cd $INSDIR/usr
	ln -s ../var var
	trap 'exit' 1 3 

	trap 'rm -r $INSDIR; exit' 2  #don't leave 1/2  around
#soft mount?
	setld $INSDIR -l $DEV || {
		 echo "
The installation has failed.  All of the subsets you chose
have been removed."
		 rm -r $INSDIR
		 exit 1
	}

	[ -r $INSDIR/bin/machine ] && ARC=`$INSDIR/bin/machine`
	case $ARC in 
	vax | mips ) ;;
	* )
		while :
		do
			echo "
Enter three letter to stand for the environment: \c"
			read ARC
			[ "$ARC" ] && break
		done
	esac

	NUM=0
	while : 
	do
		if test -d "$INSFS/root$NUM.$ARC"
		then 
			NUM=`expr $NUM + 1`
		else 
			NEWDIR=$INSFS/root$NUM.$ARC
		     	break
		fi
	done

	mv  $INSDIR $NEWDIR

  	[ -d $NEWDIR/usr/diskless ] || echo "
The server kit you are installing from is incorrect.  
Please read \"Guide to Server Setup\" document.
  
You should remove the software you have installed in $NEWDIR.
 "
	#
	## MAKEHOSTS in ./usr/hosts
	# /usr/hosts/MAKEHOSTS $NEWDIR/usr/hosts
	
	echo "\nThe base system software is installed as:\n"
	descript $NEWDIR
	cat $NEWDIR/usr/diskless/prodesc
	echo "\nThe new environment is in $NEWDIR."
}

: setdmsinit
setdmsinit()
{
	KEY=$1
	shift
	Newsubs=$*
	if [ -s $CLIROOT0/etc/dmsinit ]
	then
		grep -v "rm -f /etc/dmsinit" $CLIROOT0/etc/dmsinit > /tmp/tn$$	
		mv /tmp/tn$$ $CLIROOT0/etc/dmsinit
	else
		echo "#!/bin/sh5" >> dmsinit
	fi
	for i in $Newsubs
	do
		echo "/etc/setld -c $i $KEY" >> dmsinit
	done
	echo "rm -f /etc/dmsinit" >> dmsinit
	chmod 744 dmsinit
}

: FindFiles
# Find files and list out
FindFiles()
{
	FL=$1
	> $FL
	Ticker
	for i in `find . -print|sort` 
	do
		if (test -f $i -a -r $i)
		then 
			echo "$i `sum $i`" >> $FL
		else 
			echo $i >> $FL
		fi
	done
	Unticker
}

: PropagateCli
## In propagate installations, added files existed before, 
#  so find cksums then compare before and after.  
# If file has changed it will appear in the diff.
#  We don't cksum directories, because if one file inside 
#  a directory has changed 
# so will the cksum for the whole directory.

PropagateCli()
{
	INSDIR=$1
	shift
	DLclients=$*
	cd $INSDIR/usr/etc/subsets
	LK1=
	for i in `ls *.lk` 
	do
		Sub=`expr $i : '\(.*\).lk'`
		LK1="$LK1 $Sub" 
	done	

	trap '
mv $INSDIR/../USR $INSDIR/usr; rm -f $INSDIR/../before; exit' 1 2 3 
	cd $INSDIR/.. ; mv $INSDIR/usr USR  # nobody touches
	cd $INSDIR
	FindFiles ../before
	mv $INSDIR/../USR $INSDIR/usr
	trap 'exit' 1 2 3 
	
	setld $INSDIR -l $DEV || { 
		echo "setld failed"  
		exit 1
	}
	descript $INSDIR # put info into prodesc

# figure out what layered product has just been installed and
# $CLIROOT = $INSDIR
# run setld, and run scp on that client's root - $CLIROOT0

	cd $INSDIR/usr/etc/subsets
	Newsubs=
	for i in `ls *.lk`
	do
		New_S=`expr $i : '\(.*\).lk'`
		found=1
		for j in $LK1
		do
			case $j in
			$New_S )	
				found=0
				break	;;
			esac
		done
		case $found in
		1 )
			Newsubs="$Newsubs $New_S"
			;;
		esac
	done

	trap 'mv $INSDIR/../USR $INSDIR/usr; exit' 1 2 3 
	cd $INSDIR/.. ; mv $INSDIR/usr USR  # nobody touches
	cd $INSDIR
	FindFiles ../after
	mv $INSDIR/../USR $INSDIR/usr
	trap 'exit' 1 2 3 

	cd $INSDIR/..
	TARLST=`diff before after | grep '^>' | egrep -v "fverifylog" |
			awk '{print $2}'`
	rm before
	rm after
	case "$TARLST" in
	"" )
		;;
	* )
		for EACHCLI in $DLclients
		do
			. /dlclient*/$EACHCLI.root/etc/dlparam
			#don't mess up a client's area
			trap '' 1 2 3 

			echo "Propagating to $EACHCLI ...\n"
			(cd $INSDIR;  tar cpf - $TARLST) | 
				(cd $CLIROOT0; tar xpf - )
			#
			# pass to setld -c INSTALL rule
			#
		
			cd $CLIROOT0/etc
			setdmsinit INSTALL $Newsubs 
			trap 'exit' 1 2 3 
		done
	esac	
}

InstallOld()
{
	echo "
You have chosen to install additional software into an existing diskless 
environment.  These are the available installation directories:\n"
	DIRS=`ls -d /dlenv*/root*`
	case $DIRS in 
	"" )
		echo "NO installation directories available."
		exit 1	;;
	esac

	INSDIR=`menu $DIRS`

	#INSDIR=`expr $INSDIR : '\(.*\)$'`
	test -d "$INSDIR" || { 
		echo "$INSDIR is not a directory"
		exit
	}

	INSFS=`expr $INSDIR : '\(.*\)/root.*$'`

	CheckDevice
		
	cd $INSDIR
	getclients
	case $DLclients in
	"" )	
		setld $INSDIR -l $DEV || { 
			echo "setld failed"  
			exit 1
		}
		descript $INSDIR # put info into prodesc
		exit 0
		;;
	* )
		while :
		do
			echo "
The product software will automatically be propagated to every 
registered client.  Is that alright? (y/n): \c"
			read ans
			case $ans in
			[Nn] )	exit 0	;;
			[Yy] )
				PropagateCli $INSDIR $DLclients
				break
			esac
		done
	esac	
}

# 
#install the software 
# main program
#

test -d /dlenv* || { 
	echo "No installation file systems have been created"
	exit 1
}

#
# subroutine : make sure the network is right.
#

setnetup

while :
do
	echo "
The menu below offers you two software installation alternatives:

   1)  You can install an operating system to a new diskless area.

   2)  You can install additional software to an existing diskless
       area that already contains an operating system. 

Diskless Area Software Installation Menu:

        1  Install Operating System to New Area
        2  Add software to Existing Area
        3  Return to Previous Menu

Enter your choice: \c"
	read ANS
	case $ANS in
	1 )	
		InstallNew
		break	;;
	2 )
		InstallOld
		break	;;
	3 )
		break   ;;
	esac
done
