#!/bin/sh5
#  @(#)addoutsys.sh	4.1	ULTRIX	7/2/90
#
#	addoutsys
#
#       SEE END OF FILE FOR COPYRIGHT NOTICE, comments, and edit history
#
# Set up Shell file parameters.
#	dir path for uucp: var, home, and spool.
#	2 default directories needed for uucp.

STDSIGS="1 2 3 15"
trap '' $STDSIGS  # till we setup fault handler
case "`echo -n`" in  # handle sh or sh5 echo's
"-n")	MN=  BC='\c' ;;
   *)	MN='-n' BC= ;;
esac
#DEBUG=y ; export DEBUG
case "$DEBUG" in
y)
	MYROOT="/usr/staff1/XXX"; UULIB=$MYROOT
	UUHOME=$MYROOT/spool.uucppublic ; UUSPOOL=$MYROOT/spool.uucp
	NULL=dev.null
	;;
*)
	UULIB=/usr/var/uucp ; UUHOME=/usr/spool/uucppublic
	UUSPOOL=/usr/spool/uucp ; NULL=/dev/null
	;;
esac
MYNAME=$UULIB/addoutsys
LDEVICES=$UULIB/L-devices
PATH="$UULIB:/usr/ucb:/bin:/etc:/usr/bin:";export PATH
HOSTNAME=`hostname`
readonly UULIB UUHOME UUSPOOL PATH

MSG_SKIPPED="
System skipped...
"
MSG_ALREADYEXISTS="
This system is already in the L.sys file.  Do you wish to:

	1   Add an additional entry for this system
	2   Update the existing entry
	3   Skip this system and continue on to another"

## Fault routines ##
REPEATORDIE='
	echo $MN " 
Interrupt !!!  "$BC
	while :
	do
echo $MN "Enter \"q\" to quit, or \"c\" to continue. [q]: "$BC
	read buff
	case "$buff" in
	c)
		echo "
Current system skipped.  Continuing...
"
		exec $MYNAME $0
		# NOTREACHED #
		;;
	q|"")
		exit 5
		;;
	esac
	done
'
IGNORE=':'
# Setup fault handler
ONINTR=$REPEATORDIE
trap 'eval "$ONINTR"' $STDSIGS

DEF_CLASS=1200  # default baud rate remote system is called at

# See what speeds are available
SPEEDS=`awk '$1 == "ACU" {print $4}' $LDEVICES 2>$NULL` || {
	echo "$0: Can't open $LDEVICES" ; exit 2
}
case "$SPEEDS" in
"")	echo "You have no modems configured for use with uucp."
	;;
esac
# Make a default speed to call out with - try 1200 first, 300 next
if echo $SPEEDS | grep -ws 1200
then
	DEF_CLASS=1200
elif echo $SPEEDS | grep -ws 300
then
	DEF_CLASS=300
else
	set - $SPEEDS 
	DEF_CLASS=$1
fi

: ${ADDEDSYSTEM=""}
: ${ne=ne}  ${a=a}  # plays tricks with the next sentence
export ne a ADDEDSYSTEM
while :
do
	echo $MN "Enter the name of $a remote system to call out to,
press RETURN if no$ne: "$BC
	case $ne in ne) ne=" more" ; a=another ;; esac
	read RSYSTEM
	case "$RSYSTEM" in
	"")	break
		;;
	esac

	##  See if an entry already exists
	UPDATE=	SKIP=
	[ -f $UULIB/L.sys ] && set - `grep -w "^$RSYSTEM" $UULIB/L.sys` &&
	{
		case "$2" in
		[Nn]ever|[Ii]ncoming)
			UPDATE=y
			;;
		*)
			echo "$MSG_ALREADYEXISTS"
			while : # prompt
			do
			echo $MN "
Please enter the number of your selection (1/2/3) [1]: "$BC
			read JUNK
			case "$JUNK" in
			1|"")	break
				;;
			2)	UPDATE=y 
				break
				;;
			3)	echo "$MSG_SKIPPED"
				SKIP=y
				break
				;;
			esac
			done   # prompt

			case $SKIP in y) continue ;; esac
			;;
		esac
	}
		
	echo "
Next you must enter the times when your system is ALLOWED to call $RSYSTEM.
The four selections are:

	1   Any time of any day
	2   Evenings (Mon-Fri 5pm - 8am, Sat & Sun all day )
	3   Nights   (Mon-Fri 11pm - 8am,  Sat all day  &  Sun until 5pm)
	4   Never"
	# This next part doesn't cause the remote system to
	#  be polled. It only sets up the times uucp is ALLOWED
	#  to call the remote system. 
	# TODO - should add polling capability (add sys to UUCP.*)
	while :
	do
		echo $MN "
Please enter the number of your selection (1/2/3/4) [1] ? "$BC

		read JUNK
		case "$JUNK" in
		1|"")	TIME=Any 
			TIMEMSG="Any time of any day"
			;;
		2)	TIME="Sa|Su|Wk1705-2359|Wk0000-0755"
		      	TIMEMSG="Evenings (Mon-Fri 5pm - 8am, Sat & Sun all day )"
			;;
		3)	TIME="Sa|Su0000-1655|Wk2305-2359|Wk0000-0755"
			TIMEMSG="Nights   (Mon-Fri 11pm - 8am,  
				 Sat all day  &  Sun until 5pm)"
			;;
		4)	TIME=Never
			TIMEMSG="Never"
			;;
		*)	
			continue
			;;
		esac
		break
	done	# while :



	# Get the line speed (CLASS).  
	while :
	do
		echo $MN "
Enter the line speed for system $RSYSTEM [$DEF_CLASS] : "$BC
		read CLASS
		case "$CLASS" in
		"")	CLASS=$DEF_CLASS
			;;
		esac
		echo $SPEEDS | grep -ws $CLASS && break
		echo "
There is no modem configured at that speed.  The list of available
modem speeds is: $SPEEDS

Please enter one of these numbers."
	done

	# Get the phone #.
	while : # eternally
	do
		echo $MN "
Enter the phone number for system $RSYSTEM,
If directly connected, specify which tty line: "$BC
		read PHONE_NUM
		case "$PHONE_NUM" in
		"")	
			continue
			;;
		esac
		# remove any blanks
		set - $PHONE_NUM; PHONE_NUM=$1  
		break
	done


case "$PHONE_NUM" in
	tty*)	ACU="$PHONE_NUM"
		;;
	*)	ACU="ACU"
		;;
esac

	# Get login name for your connection on remote system
	DEF_LOGIN=`echo U$HOSTNAME | dd bs=8 count=1 2>$NULL`
	while :
	do
		echo $MN "
Enter your uucp login name on system $RSYSTEM [$DEF_LOGIN]: "$BC
		read LOGIN
		case $LOGIN in "") LOGIN=$DEF_LOGIN ;; esac
		numchars=`echo "$LOGIN"|wc -c`
		set - $numchars; numchars=$1
		case "$numchars" in
		"1")	# NULL
			continue
			;;
		[2-9])	break #ok
			;;
		*)
			echo "
The login name must not be greater than eight characters.
"
			;;
		esac
	done

	# Get password for your connection on remote system
	while : # true
	do
		echo $MN "
Enter the password for login \"$LOGIN\" on system \"$RSYSTEM\": "$BC
		read PASSWORD
		case "$PASSWORD" in
		"")	
			continue
			;;
		esac
		break
	done
	MSG_VERIFY="
Name of remote system: $RSYSTEM
Time allowed to call: $TIMEMSG
Line speed: $CLASS
Phone number: $PHONE_NUM
Login name on remote system: $LOGIN
Password on remote system: $PASSWORD
"
        echo "
The following is a summary of your responses for system \"$RSYSTEM\". "
	echo "$MSG_VERIFY"
	while :
	do
		DOIT=
		echo \
"Please verify the above information and choose one of the following:

	1    Add the system
	2    Skip the system
	3    Redisplay the summary and repeat this prompt
"
echo $MN "Please enter your selection (1/2/3) [1]: "$BC
		read BUFF
		case "$BUFF" in
		1|"") DOIT="y"
			break
			;;
		"2")	echo "$MSG_SKIPPED"
			break
			;;
		"3")	echo "$MSG_VERIFY"
			continue
			;;
		*)	echo " "
			continue
			;;
		esac
	done

# redefine some strings
MSG_ADDED="
System $RSYSTEM added for outgoing connections
"

ENTRY="$RSYSTEM $TIME $ACU $CLASS $PHONE_NUM "'"" \\r ogin:-\\r-ogin:-BREAK-ogin:'" $LOGIN ssword: $PASSWORD"

EDSTRING=\
"H
/$RSYSTEM/
c
$ENTRY
.
w
q
"

	# Append (modify) entry to the L.sys file.
	case "$DOIT" in
	y) 	
		trap '' $STDSIGS  # We're committed
		ADDEDSYSTEM=y
		case "$UPDATE" in
		y)	
			echo "$EDSTRING" | ed - $UULIB/L.sys 1>>$NULL 2>&1
			;;
		*)
			echo "$ENTRY" >> $UULIB/L.sys
			;;
		esac
		echo "$MSG_ADDED"
		trap 'eval "$ONINTR"' $STDSIGS
	esac

done	# while :

# Make sure sendmail knows about new systems.
case "$ADDEDSYSTEM" in "y")
	case "$DEBUG" in "y") echo " - sendmail -bz" ;; esac
	/usr/lib/sendmail -bz
esac
exit 0
