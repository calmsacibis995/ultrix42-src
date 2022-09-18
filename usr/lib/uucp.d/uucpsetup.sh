#!/bin/sh
#  @(#)uucpsetup.sh	4.1 ULTRIX	7/2/90
#
#	uucpsetup  -  configure uucp related files
#
#	Copyright and edit history at end of file
#
HOSTNAME=`hostname`
case "$HOSTNAME" in
"")	echo "Your system does not have a name."
	exit 1
esac

#DEBUG=y ; export DEBUG
case "$DEBUG" in
y)
	MYROOT=/usr/staff1/XXX
	UULIB=$MYROOT
	UUHOME=$MYROOT/spool.uucppublic
	UUSPOOL=$MYROOT/spool.uucp
	DIR1=$UUSPOOL/sys/DEFAULT/D.$HOSTNAME
	DIR2=${DIR1}X
	CRONTAB=$MYROOT/crontab
	NULL=$MYROOT/dev.null
	;;
*)
	UULIB=/usr/var/uucp
	UUHOME=/usr/spool/uucppublic
	UUSPOOL=/usr/spool/uucp
	DIR1=$UUSPOOL/sys/DEFAULT/D.$HOSTNAME
	DIR2=${DIR1}X
	CRONTAB=/usr/lib/crontab
	NULL=/dev/null
	;;
esac
PATH="$UULIB:/usr/ucb:/bin:/etc:/usr/bin:";export PATH
readonly UULIB UUHOME UUSPOOL DIR1 DIR2 PATH

USAGE="usage: uucpsetup [-moia]
	-m	add modems for uucp and tip
	-o	add outgoing systems
	-i	add incoming systems
	-a	initialize directories and turn on -m -o and -i flags
	-q	quite mode: suppress verbose messages
	no flags implies -o and -i "

CHILDABORT=5

## For now, die on interrupts
trap 'echo Interrupt... Exiting uucpsetup; exit 1' 1 2 3 

# Require it to be run by root
case "`whoami`" in
"root") 	: ;;
*)	echo "$0: you must be root to run uucpsetup."
	exit 1
esac


# Process options
case "$1" in	# no options => -o -i
"")	OUTFLG=y INFLG=y
esac
while :
do
    case "$1" in
    -*) for opt in `echo $1 |
	awk '{for(i=2;i<=length($0);i++)printf"%s ",substr($0,i,1)}'`
	do
		case "$opt" in
		a)	ONETIMEFLG=y MODEMFLG=y OUTFLG=y INFLG=y
			;;
		m)	MODEMFLG=y
			;;
		o)	OUTFLG=y
			;;
		i)	INFLG=y
			;;
		q)	QUIET=y ; export QUIET
			;;
		*)	echo "uucpsetup: unknown option: $opt"
			echo "$USAGE"
			exit 1
		esac
	done
	shift
	;;
    *)	break
	;;
    esac
done
case "$1" in  # check for oldstyle "install" flag
install)  ONETIMEFLG=y MODEMFLG=y OUTFLG=y INFLG=y ;;
"")	: ;;
*)	echo "uucpsetup: unknown arguments"
	echo "$USAGE"
	exit 1
esac

#  If called with the -a flag, the defunct "install" flag, or if uucp is 
#  found not to be setup (default directories don't exist), then do 
#  one-time setups.

[ -d $DIR1 -a -d $DIR2 ] || ONETIMEFLG=y

case "$QUIET" in y) : ;; *) echo "
Whenever a default selection is given for a question
[shown in square brackets] press the RETURN key to 
select the default choice.
"
esac

case "$ONETIMEFLG" in
"y")
	##  PART 0 : INITIAL SETUP

	# Create default directories with makefile
	(cd $UULIB; make mkdirs 2>$NULL 1>&2 )

	case "$DEBUG" in  # make would normally have done this...
	y)  mkdir $UUSPOOL $UUSPOOL/sys $UUSPOOL/sys/DEFAULT $DIR1 $DIR2 2>$NULL
	esac

	touch $UULIB/L_stat $UULIB/R_stat $UUSPOOL/LOGFILE $UUSPOOL/SYSLOG
	chown uucp $UULIB/L_stat $UULIB/R_stat $UUSPOOL/LOGFILE $UUSPOOL/SYSLOG 
	chmod 644 $UULIB/L_stat $UULIB/R_stat $UUSPOOL/LOGFILE $UUSPOOL/SYSLOG 

	#   Set up cron entries for polling systems hourly and
	#	 cleaning up log file daily.
	egrep  -s '^#.*%UUCPSTART%' $CRONTAB ||
	{
		# Put in entries
echo "# %UUCPSTART%  - uucp related entries, put here by uucpsetup
30 * * * * su uucpa < $UULIB/uucp.hour
0 6 * * * su uucpa < $UULIB/uucp.day
0 12 * * * su uucpa < $UULIB/uucp.noon
0 3 * * 1 su uucpa < $UULIB/uucp.week
0 2 * * * su uucpa < $UULIB/uucp.night
#30 2 * * 1 su uucpa < $UULIB/uucp.longhall
# %UUCPEND%" >> $CRONTAB
	} 

	# Set up default entries in USERFILE, if not already there.
	WRITE=
	[ -f $UULIB/USERFILE ] &&
	{
		grep -s "remote" $UULIB/USERFILE ||
			WRITE="remote,	X0	$UUHOME
"
		grep -s "local" $UULIB/USERFILE ||
			WRITE="${WRITE}local,	X9	/
"
		echo -n "$WRITE" >> $UULIB/USERFILE
	}

	;;
esac  # $ONETIME

trap ':' 1 2 3 15 # Let commands in $UULIB deal with signals

case "$MODEMFLG" in y)
	#
	##  PART 1 : ADDING MODEMS
	#
	case "$QUIET" in y) : ;; *)  echo "
INSTALLING MODEMS:
"
	esac
	$UULIB/addmodems
	case "$?" in
	0)	: ;;
	$CHILDABORT)	
		exit $CHILDABORT
		;;
	*)	echo "Unrecoverable error encountered while installing modems."
		exit 1
	esac
esac
case "$OUTFLG" in y)
	#
	## PART II : SYSTEMS TO CALL OUT TO
	#
	case "$QUIET" in y) : ;; *) echo "
OUTGOING UUCP CONNECTIONS:
	"
	esac
	$UULIB/addoutsys
	case "$?" in
	0)	: ;;
	$CHILDABORT)	
		exit $CHILDABORT
		;;
	*)	echo "Unrecoverable error adding outgoing uucp connections."
		exit 1
	esac
esac
case "$INFLG" in y)
	#
	## PART III : SYSTEMS WHO CALL US
	#
	case "$QUIET" in y) : ;; *) echo "
INCOMING UUCP CONNECTIONS:
	"
	esac
	$UULIB/addinsys
	case "$?" in
	0)	: ;;
	$CHILDABORT)	exit $CHILDABORT;;
	*)	echo "Unrecoverable error adding incoming uucp connections."
		exit 1
	esac
esac
	
## EPILOGUE
case "$QUIET" in y) : ;; *) echo "
Finished with UUCP setup."
esac
exit 0
#ifdef notdef
# 			Copyright (c) 1984 by				
# 		Digital Equipment Corporation, Maynard, MA		
# 			All rights reserved.				
# 									
#	This software is furnished under a license and may be used and	
#	copied  only  in accordance with the terms of such license and	
#	with the  inclusion  of  the  above  copyright  notice.   This	
#	software  or  any  other copies thereof may not be provided or	
#	otherwise made available to any other person.  No title to and	
#	ownership of the software is hereby transferred.			
# 									
#	The information in this software is subject to change  without	
#	notice  and should not be construed as a commitment by Digital	
#	Equipment Corporation.						
# 									
#	Digital assumes no responsibility for the use  or  reliability	
#	of its software on equipment which is not supplied by Digital.	
#
#
#	derived from 1.2 uucpsetup.sh
#		
#
#	REVISION HISTORY:
#
#		1-JUN-1986	XXX
#		Rewrite for vers 2.0 ...
#		I broke out uucpsetup into three subprograms which
#		live in /usr/lib/uucp.  The subprograms are:
#
#			1) addmodems - 
#			   Configure dialers for use with uucp and tip.
#
#			2) addoutsys -
#			   Add systems to call out to.
#
#			3) addinsys -
#			   Add systems allowed to call us.
#
#		The one-time setup stuff is still done here directly.
#		This includes making default spool directories, adding
#		cron entries for uucp.* scripts, and insuring default
#		USERFILE entries.  The rest just frontends the new
#		programs in /usr/lib/uucp. 
#
#		16-Feb-1988 lea  /usr/lib/uucp -> /usr/var/uucp
#		11-Aug-1989 lea  Change crontab entries for uucp.week
#			and longhall, they were setup to execute once
#			a month and not once a week
#		
#endif
