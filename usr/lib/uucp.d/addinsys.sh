#!/bin/sh5
#  @(#)addinsys.sh	4.1	ULTRIX	7/2/90
#
#	addinsys - setup uucp for systems that call or request
#		services (files/execution)
#
#       see end of file for copyright, comments, and edit history
#

STDSIGS="1 2 3 15"
trap "" $STDSIGS   #  Until fault handler is setup
case "`echo -n`" in  # handle sh or sh5 echo's
"-n")	MN=  BC='\c' ;;
   *)	MN='-n' BC= ;;
esac
#DEBUG=y ; export DEBUG
case "$DEBUG" in
y)
	MYROOT="/usr/staff1/XXX" ; UULIB=$MYROOT
	UUHOME=$MYROOT/spool.uucppublic ; UUSPOOL=$MYROOT/spool.uucp
	NULL=dev.null ; ETC_PASSWD=$MYROOT/etc.passwd ; ETC_PTMP=$MYROOT/ptmp
	;;
*)
	UULIB=/usr/var/uucp ; UUHOME=/usr/spool/uucppublic 
	UUSPOOL=/usr/spool/uucp ; NULL=/dev/null ; ETC_PASSWD=/etc/passwd
	ETC_PTMP=/etc/ptmp
	;;
esac
DIR1=$UUSPOOL/sys/DEFAULT/D.$HOSTNAME
DIR2=${DIR1}X
PATH="$UULIB:/usr/ucb:/bin:/etc:/usr/bin:";export PATH
readonly UULIB UUHOME UUSPOOL DIR1 DIR2 PATH
MYNAME=$UULIB/addinsys
MKPASS=$UULIB/mkpass

## Fault routines
REPEATORDIE='
	echo $MN "
Interrupt !!!  "$BC
	while :
	do
echo $MN "Enter \"q\" to quit, or \"c\" to continue [q]: "$BC
	read buff
	case "$buff" in
	c)
		echo "
Current system aborted.  Continuing...
"
		exec $MYNAME $0
		# NOTREACHED #
		;;
	""|q)
		exit 5
		;;
	esac
	done
'
IGNORE=':'
ONINTR=$REPEATORDIE
trap 'eval "$ONINTR"' $STDSIGS

DEF_XLEVEL=1  # default execution level

XLEVEL_HELP="
The execution access level can range from 0 to 9. Where 0 gives no
access to the remote system and 9 gives the most access. In order for a
remote system to be able to execute a particular command on your
system, the remote system must have an access level equal to or greater
than the corresponding protection level in the L.cmds file."

: ${STRING=ne}  ${SEENMSG=""} ${a=a} ${ADDEDSYSTEM=""}
export STRING SEENMSG a ADDEDSYSTEM   # export it in case we're re-invoked 
while :
do
	echo $MN \
"Enter the name of $a system allowed to establish incoming uucp 
connections, press RETURN if no$STRING: "$BC
	read INSYSTEM
	case "$INSYSTEM" in
	"")	break
		;;
	esac
	case "$STRING" in
	ne)	STRING=" more" a="another" ;;
	esac

	[ -f $UULIB/USERFILE ] && 
	    sed -n 's/.*,\([^ 	]*\).*/\1/p' $UULIB/USERFILE |
	        grep -sw "$INSYSTEM" &&
	{
		echo "
\"$INSYSTEM\" has already been added for incoming uucp connections."
		echo ""
		continue
	}

DUPLOGIN_MSG="
You may use the same login name for several systems, but you must 
remember the password that was given when original login was created.  
To keep this login name, answer \"y\" to the next question, or answer \"n\" 
if you want to pick another login name.
"

	SKIPLOGIN=	# If login name already exists don't update /etc/passwd 
	DEF_LOGIN=`echo U$INSYSTEM | dd bs=8 count=1 2>$NULL`
	while :
	do
		echo $MN "
Enter a login name for system \"$INSYSTEM\" [${DEF_LOGIN}]: "$BC
		read LOGIN
		case "$LOGIN" in
		"")	LOGIN=$DEF_LOGIN
		esac
		gt_eight=`expr "$LOGIN" : "........\(.*\)"`
		case "$gt_eight" in
		"")	:
			;;
		*)	echo "
The login name may not be greater than eight characters, try again."
			continue
			;;
		esac
		grep -ws "^$LOGIN" $UULIB/USERFILE &&
		{
			echo $MN "
$LOGIN is already in $UULIB/USERFILE."$BC
			case "$SEENMSG" in
			y)	: ;;
			*)	echo "$DUPLOGIN_MSG"
				SEENMSG=y ;;
			esac
			echo $MN "Is this ok (y/n) [y]: "$BC
			read BUFF
			case "$BUFF" in
			""|y)	SKIPLOGIN=y ;;
			*)	continue ;;
			esac
		}
		break  # got a name
	done
	


	case "$SKIPLOGIN" in
	y)	:
		;;
	*)
	echo $MN "
Enter a short comment for the passwd file: "$BC

	read COMMENT  # Null comment is ok
	COMMENT=`echo $COMMENT | tr ':' ' '`  # dispose of :'s in comment
		;;
	esac

	while :
	do
		case "$SKIPLOGIN" in
		y)	break ;;
		esac
		echo $MN "
Enter a password for system \"$INSYSTEM\" for the password file: "$BC
		read PASSWD
		case "$PASSWD" in
		"")	
			echo "
Null password not acceptable, try again."
			continue
		esac
		break
	done
	ENCRYPTED=`echo $PASSWD | $MKPASS`

	# Next set up the USERFILE
	while :
	do
		echo $MN "
Enter the execution access level for system \"$INSYSTEM\", \"?\" for help
(0-9) [$DEF_XLEVEL]: "$BC

		read XLEVEL
		case "$XLEVEL" in
		"")	XLEVEL=$DEF_XLEVEL
			;;
		[0-9])	;;
		"?")	
			echo "$XLEVEL_HELP"
			while :
			do
				echo $MN "
Do you wish to see the L.cmds file (y/n) [y]? "$BC
				read JUNK
				case $JUNK in
				""|[yY]*)	
					echo
					more $UULIB/L.cmds
					;;
				[nN]*)	;;
				*)	echo ""
					continue
					;;
				esac
				break
			done
			continue
			;;
			
		*)	echo "
	The execution access level must be between 0 and 9."
			continue
			;;
		esac
		break
	done

	# does s/he want callback?
	while :
	do
		echo $MN "
If you choose the call back option you will always pay the phone 
bill for connections.  Do you want the call back option for system 
\"$INSYSTEM\" (y/n) [n]: "$BC

		read JUNK
		case "$JUNK" in
		[yY]*)	CALLBACK=c
			CALLBACKMSG='y'
			;;
		""|[nN]*)	
			CALLBACK=
			CALLBACKMSG='n'
			;;
		*)	echo ""
			continue
			;;
		esac
		break
	done

	echo $MN "
Enter the directory path for system \"$INSYSTEM\" 
[$UUHOME]: "$BC
	read DIRPATH

	case $DIRPATH in
	"")	DIRPATH="$UUHOME"
		;;
	esac

	## Verify this entry
	case "$SKIPLOGIN" in
	y)
		VERIFYMSG="
Name of system: $INSYSTEM
Login name for system: $LOGIN
Execution level: $XLEVEL
Callback option (y/n): $CALLBACKMSG
Directory path: $DIRPATH
"
		;;
	*)	VERIFYMSG="
Name of system: $INSYSTEM
Login name for system: $LOGIN
Comment for password file: $COMMENT
Password: $PASSWD
Execution level: $XLEVEL
Callback option (y/n): $CALLBACKMSG
Directory path: $DIRPATH
"
		;;
	esac
        echo "
The following is a summary of your responses for system \"$INSYSTEM\". "
	echo "$VERIFYMSG"
	while :
	do
		DOIT=
echo $MN "Enter \"y\" if all the information is correct, \"n\" if you wish to
re-enter the information, or \"?\" to re-display the table. [y]: "$BC
		read BUFF
		case "$BUFF" in
		""|"y") DOIT="y"
			break
			;;
		"n")	echo "
System skipped
"
			break
			;;
		"?")	echo "$VERIFYMSG"
			continue
			;;
		*)	echo " "
			continue
			;;
		esac
	done
	case "$DOIT" in
	y)	:
		;;
	*)	continue
	esac


	## Do the work
	## RACE CONDITION ALERT !
	## This should obey /etc/ptmp locking - but the user may ^Z which
	## could leave /etc/passwd locked (/bin/sh can't trap SIGTSTP),
	## so we resign to just checking that passwd isn't currently locked.
	trap '' $STDSIGS
	ADDEDSYSTEM=y
	case "$SKIPLOGIN" in
	y)	:
		;;
	*)	
		tries=0  
		while [ $tries -lt 4 ]
		do
			locked=
			[ ! -f "$ETC_PTMP" ] && break
			locked=true
			sleep 5 ; tries=`expr $tries + 1`
		done
		case "$locked" in "true")
			echo ""
			echo "Password file is locked for over 20 seconds.  Aborting program."
			exit 3
		esac
		if grep -s "^+:" $ETC_PASSWD
		then 
		ed - $ETC_PASSWD <<EOF 1>>$NULL 2>&1
/^\(+:*\)/
i
$LOGIN:$ENCRYPTED:4:2:$COMMENT:$UUHOME:$UULIB/uucico
.
w
q
EOF
		else echo "$LOGIN:$ENCRYPTED:4:2:$COMMENT:$UUHOME:$UULIB/uucico" >> $ETC_PASSWD
fi

		;;
	esac

	echo "$LOGIN,$INSYSTEM X$XLEVEL $CALLBACK	$DIRPATH" >> $UULIB/USERFILE

	# If $INSYSTEM is not in the L.sys file already, for outgoing
	#  connections, then add it in.

	[ -f $UULIB/L.sys ] && grep -ws "^$INSYSTEM" $UULIB/L.sys ||
		echo "$INSYSTEM incoming" >> $UULIB/L.sys
	trap 'eval "$ONINTR"' $STDSIGS
	echo "
System $INSYSTEM added for incoming connections
"

done	# while : # incomming systems

# Make sure sendmail knows about the new site.
case "$ADDEDSYSTEM" in "y")
	case "$DEBUG" in "y") echo "- sendmail -bz" ;; esac
	/usr/lib/sendmail -bz ;;
esac

chown uucp $UULIB/USERFILE $UULIB/L.sys  2> $NULL
chmod 400 $UULIB/USERFILE $UULIB/L.sys   2> $NULL

exit 0
