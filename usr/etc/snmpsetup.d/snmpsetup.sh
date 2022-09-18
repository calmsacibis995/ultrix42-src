#! /bin/sh
# @(#)snmpsetup.sh	4.2	(ULTRIX)	9/11/90
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
# Purpose:	Set up Simple Network Management (SNMP)
# Usage:	snmpsetup
# Environment:	Bourne shell script
# 
# Remarks:	Sets up files:
#		/etc/rc
#		/etc/snmpd.conf
#
#
# Modification History:
#
# 06/01/89	R. Bhanukitsiri
#		Initial Release.
#
# 09/10/90      Mary Walker
#		added in more verications

ECHO="/bin/echo -n"

#
# Default files location
#
keep=n
case $1 in
 debug)	shift
	DEBUG=1
	RC=/tmp/rc
	CONF=/tmp/snmpd.conf
	echo "snmpsetup running in DEBUG mode ..."
	if [ -r /etc/rc ]
		then
		cp /etc/rc $RC
	else
		echo "snmpsetup: need /etc/rc"
		exit 1
	fi
	if [ -r /etc/snmpd.conf ]
		then
		cp /etc/snmpd.conf $CONF
	else
		echo "snmpsetup: need /etc/snmpd.conf"
		exit 1
	fi
	;;
 keep)	shift
	keep="y"
	RC=/etc/rc
	CONF=/etc/snmpd.conf
	;;
 *)	keep="n"
	RC=/etc/rc
	CONF=/etc/snmpd.conf
	;;
esac
echo "

			***** SNMPSETUP *****

"

#
# snmpsetup must be run by root
#
if [ \! -w $RC ]
	then
	echo "snmpsetup: must be run by root."
	exit 1
fi
if [ `whoami` != root ]
	then
	echo "snmpsetup: must be run by root."
	exit 1
fi

#
# Default working file locations
#
RCTMP=/tmp/snmpd.rc.$$.tmp
CONFTMP=/tmp/snmpd.conf.$$.tmp
SYSTMP=/tmp/snmpd.sys.$$.tmp
INTFTMP=/tmp/snmpd.intf.$$.tmp
NULL=/dev/null

#
# define symbols
#
sysDescr=
ifName=

#
# Set up interrupt handlers:
#
QUIT='
	if [ "$keep" = "n" ]
		then
		for i in $RCTMP $CONFTMP $SYSTMP $INTFTMP
		do 
			if [ -r $i ]
				then
				rm $i
			fi
		done
	fi
	echo "snmpsetup: terminated with no installations made."
	echo
	exit 1
'

#
# Trap ^C signal, etc.
#
trap 'eval "$QUIT"' 1 2 3 15

#
# Gather system MIB default configuration
#
mach=`/bin/machine`
if [ "$mach" = "mips" ]
	then
	dbx -k /vmunix /dev/mem > $SYSTMP << EOF
print version
print cpu
quit
EOF
	# read
	vers=
	cpu=
else
	adb -k /vmunix /dev/mem > $SYSTMP << EOF
_version/s
_cpu/X
quit
EOF
	# read
	vers=
	cpu=
fi
sysDescr="`/bin/hostname`:$cpu:$vers"

#
# Create default configuration file
#
if [ -f $CONFTMP ]
	then
	rm -f $CONFTMP
fi
cat $CONF | while read field1 line
do
	if [ "$field1" = "#" ]
		then
		echo "$field1 $line" >> $CONFTMP
	fi
done

#
# Gather interface MIB default configuration
#
netstat -i > $INTFTMP
cat $INTFTMP | while read ifn
do
	ifName=`echo $ifn | awk '{ print substr($1,0,2) }'`
done

#
# Determine run-time mode.
#
if [ -n "$1" ]
	then
	echo "snmpsetup: non-interactive mode not currently supported."
	exit 1
fi

#
# Be sure network has already been set up, and this host has a name!
#
host=`hostname`
if [ $? -ne 0 ]
then
	echo "snmpsetup: network must be setup before running snmpsetup."
	eval "$QUIT"
fi
echo "
The snmpsetup command configures the SNMP Network Management Agent
for  your  system.  SNMP is  a  network  management  protocol that
enables your system to be managed by the network manager.

The snmpsetup command first displays the  default configuration database
and allows you to tailor the configuration database to suit your system.
Default answers  are  shown  in  square brackets ([]).  To use a default
answer, press the RETURN key.

Note:	snmpsetup command  modifies the /etc/snmpd.conf file.
"
$ECHO "[ Press the RETURN key to continue ]: "
read junk

#
# System MIB configuration
#
echo "

The ULTRIX sysDescr contains three fields separated by a ":".  These
fields are:

	host-name:hardware-description:software-description

This is your default system description (sysDescr):

	$sysDescr
"
$ECHO "Do you wish to change your system description (sysDescr) [n]? "
read ans
case $ans in
 [yY]*)echo "
System description string that you enter must not be separated by
any blank space(s).
"
	again=y
	while [ $again ]
	do
		$ECHO "Enter new sysDescr? "
		read sysDescr
		if [ "$sysDescr" = "" ]
			then
			continue
		fi
		set `echo $sysDescr`
		if [ "$2" != "" ]			# check for blanks
			then
			echo "sysDescr contains blank space(s), please retry"
			continue
		fi
		$ECHO "Your sysDescr is: $sysDescr.  Is this correct [y]? "
		read ans
		case $ans in
		  [yY]*|"")echo "sysDescr	$sysDescr" >> $CONFTMP
			  again=""
			  ;;
		  [nN]*)again=y
			;;
		 *)	again=y
		esac
	done
	;;
 [nN]*)
	break;;
 *)
	break;;
esac

#
# Interface configuration
#
echo "

The following network interfaces, if present, will be automatically
configured by the SNMP Network Management Agent:

	de ln lo ni qe scs xna.

If you have network interfaces, not shown on this list, you must
configure them manually.   The following  network interfaces are
currently present on your system:
"
$ECHO "	"
cat $INTFTMP | while read ifn
do
	ifName=`echo $ifn | awk '{ print $1}'`
	if [ "$ifName" != "Name" ]
		then
		$ECHO "$ifName "
	fi
done
echo "
"
$ECHO "Do you wish to add network interfaces [n]? "
read ans
case $ans in
 [yY]*)echo >> $CONFTMP
	again=y
	while [ $again ]
	do
		$ECHO "Enter new interface name (ifName)? "
		read ifName
		$ECHO "Enter interface type (ifType) [6]? "
		read ifType
		if [ "$ifType" = "" ]
			then
			ifType=6
		fi
		$ECHO "Enter interface speed (ifSpeed) [10000000]? "
		read ifSpeed
		if [ "$ifSpeed" = "" ]
			then
			ifSpeed=10000000
		fi
		echo
		echo "You have entered: ifName=$ifName ifType=$ifType ifSpeed=$ifSpeed."
		$ECHO "Is this correct [y]? "
		read ans
		case $ans in
		 [yY]*|"")echo "interface	$ifName type $ifType" >> $CONFTMP
			echo "interface	$ifName speed $ifSpeed" >> $CONFTMP
			again=""
			;;
		 [nN])	again=y
			;;
		 *)	again=y
		esac
		echo
		if [ "$again" = "" ]
			then
			$ECHO "Do you wish to add another network interface [n]? "
			read ans
			case $ans in
			 [yY]*|"") again=y  ;;
			 [nN]*)	   again="" ;;
			 *)	   again="" ;;
			esac
		fi
	done
	;;
 [nN]*)break;;
 *)	break;;
esac

#
# Community configuration
#
echo "

A  community name  is used by the SNMP protocol to authenticate request
from a network manager.  A  community  can be read-only, read-write, or
traps.  If  a community  is read-write,  the network manager can affect
the  behavior of the network on your host. Your system can be monitored
by any network manager  if the IP address associated with the community
is 0.0.0.0.  For example,

		community	public 0.0.0.0 read-only

The community information is mandatory and must be configured.
"
echo >> $CONFTMP
pub=f
again=y
while [ $again ]
do
	$ECHO "Enter community name [RETURN when done]? "
	read comm
	if [ "$comm" = "" ]
		then
		break
	fi
	$ECHO "Enter IP address associated with community $comm [0.0.0.0]? "
	read ipaddr
	if [ "$ipaddr" = "" ]
		then
		ipaddr="0.0.0.0"
	fi
	$ECHO "Select community type (read-only,read-write,traps) [read-only]? "
	read commtype
	case $commtype in
	 read-only)	;;
	 read-write)	;;
	 *)		commtype="read-only" ;;
	esac
	echo "community	$comm	$ipaddr	$commtype" >> $CONFTMP
	echo
	if [ $comm = public -a $ipaddr = 0.0.0.0 -a $commtype = read-only ]
		then
		pub=t
	fi
	again=y
	while [ $again ]
	do
		$ECHO "Do you wish to add another community [n]? "
		read ans
		case $ans in
		 [yY]*)again=y  ;;
		 [nN]*|"")again="" ;;
		 *)	again="" ;;
		esac
	done
done
echo
egrep -s community $CONFTMP
if [ $? = 1 ]
	then
	echo "
You have not configured any community.  Without a community, the SNMP
Network Management will NOT respond to any network management request
from anyone."
fi
if [ $pub = f ]
	then
	echo "
A public community accessible by ANYONE can be configured for you as:

	community 	public	0.0.0.0	read-only

We caution you that this means readable by ANYONE!
"
	again=y
	while [ $again ]
	do
		$ECHO "Do you wish to proceed to add the public community [y]? "
		read ans
		case $ans in
		 [yY]*|"")echo "community	public	0.0.0.0	read-only" >> $CONFTMP
			again=""
			;;
		 [nN]*)again="" ;;
		 *)	again=y ;;
		esac
	done
else
	echo "
A public community accessible by ANYONE was configured by you as:

	community 	public	0.0.0.0	read-only

We caution you that this means readable by ANYONE!
"
fi
#
# Extensible Agent configuration
#
echo "

ULTRIX extends the capability of the SNMP Network Management Agent
with  extensible agents.  An  extensible agent is  usually a user-
written daemon that  provides more  network management  capability
specific to the user.
"
again=y
while [ $again ]
do
	$ECHO "Do you wish to include a user-written extended agent [n]? "
	read ans
	case $ans in
	 [yY]*)    ext_agent=y; again="" ;;
	 [nN]*|"") ext_agent=""; again="";;
	 *)        ext_agent=""; again=y ;;
	esac
done
echo >> $CONFTMP
while [ $ext_agent ]
do
	$ECHO "Enter full path name of the extended agent? "
	read eapath
	if [ "$eapath" = "" ]
		then
		continue
	fi
	def_eaname=`echo $eapath | awk -F/ '{ print $NF}'`
	$ECHO "Enter name of the extended agent [$def_eaname]? "
	read eaname
	if [ "$eaname" = "" ]
		then
		eaname=$def_eaname
	fi
	echo
	again=y
	while [ $again ]
	do
		echo  "You have entered: path=$eapath name=$eaname."
		$ECHO "Is this correct [y]? "
		read ans
		case $ans in
		 [yY]*|"")ext_agent=""; again="" ;;
		 [nN]*) ext_agent=y; again="" ;;
		 *)	again=y ;;
		esac
	done
	echo "extension	$eapath	$eaname" >> $CONFTMP
	echo
	if [ "$ext_agent" = "" ]
	then
		again=y
		while [ $again ]
		do
			$ECHO "Do you wish to include another extended agent [n]? "
			read ans
			case $ans in
			 [yY]*)   ext_agent=y; again="" ;;
			 [nN]*|"")ext_agent=""; again="" ;;
			 *)	again=y ;;
			esac
		done
	fi
done

#
# Add /etc/snmpd to /etc/rc if not defined.
#
egrep -s "snmpd" $RC
if [ $? = 1 ]
	then
	cp $RC $RCTMP
	ed $RCTMP > $NULL << EOF
/inetd
/}
.+1
i
[ -f /etc/snmpd ] && {
	/etc/snmpd;		echo -n ' snmpd'		>/dev/console
}
.
w
q
EOF
fi

#
# Update the system files
#
if [ -r $RCTMP ]
	then
	mv $RCTMP $RC
fi
mv $CONFTMP $CONF

#
# Cleanup.
#
if [ "$keep" = "n" ]
	then
	for i in $RCTMP $CONFTMP $SYSTMP $INTFTMP
	do 
		if [ -r $i ]
			then
			rm $i
		fi
	done
fi

#
# All done.
#
echo
echo "		***** SNMPSETUP COMPLETE *****

"
exit 0
