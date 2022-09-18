#!/bin/sh5
#  @(#)addmodems.sh	4.1	ULTRIX	7/2/90
#
# addmodems - configure new modems for uucp and tip
#
#	copyright at end of file
#
STDSIGS="1 2 3 15"
trap '' $STDSIGS # Ignore until we setup fault handler
case "`echo -n`" in  # handle sh or sh5 echo's
"-n")	MN= BC='\c' ;;
   *)	MN='-n' BC= ;;
esac
#DEBUG=y; export DEBUG
case "$DEBUG" in
y)
	MYROOT="/usr/staff1/XXX" ; UULIB=$MYROOT 
	ACUCAP=$MYROOT/acucap ; FILECMD=$MYROOT/file ; TMP=$MYROOT/tmp 
	DEV=$MYROOT/dev ; TTYS=$MYROOT/ttys ; NULL=dev.null 
	REMOTE=$MYROOT/remote
	;;
*)
	UULIB=/usr/var/uucp ; ACUCAP=/etc/acucap ; FILECMD=/usr/bin/file
	TMP=/tmp ; DEV=/dev ; TTYS=/etc/ttys ; NULL=/dev/null 
	REMOTE=/etc/remote
	;;
esac

PATH=:/etc:/usr/ucb:/bin:/usr/bin:$UULIB:
MODEMDB=$UULIB/modemdb
MDDBTMP=$UULIB/modemdb.tmp
TTYDB=$UULIB/ttydb
MYNAME=$UULIB/addmodems
LDEVICES=$UULIB/L-devices
DIALUP="dialup"
AorD="ACU"   #if direct then this changes to DIR

DIALERS=`sed -n '/^[a-zA-Z]/s/|.*//p' < $ACUCAP`  
DIALERS="$DIALERS direct"  # direct connect lines 

## Pager macro  
PAGER='eval echo $MN "-- Press RETURN to continue --"$BC; read JUNK'

## Fault routines
REPEATORDIE='
	echo $MN "
Interrupt !!!  "$BC
	while :
	do
echo $MN "Enter \"q\" to quit, or \"c\" to start over [q]: "$BC
	read buff
	case "$buff" in
	"" | q*)
		rm -f $MDDBTMP
		exit 5
		;;
	c)
		echo "
Starting over...
"
		rm -f $MDDBTMP
		exec $MYNAME $*   
		#NOTREACHED#
		;;
	esac
	done
'
IGNORE=':'

# Setup fault handler
ONINTR=$REPEATORDIE
trap 'eval "$ONINTR"' $STDSIGS

## Allowable modem speeds.  If there is a modem we don't know about then we
## allow any of the speeds under DEFAULTSPEED.  The first entry is the default
## speed for the given modem type.
#### SPEEDS ############# MODEM NAME ######
DF02SPEED="300"		# df02
DF03SPEED="1200 300"	# df03
DF112SPEED="1200 300"	# df112*
DF124SPEED="2400 1200"	# df124*
DF224SPEED="2400 1200"	# df224*
HAYESSPEED="2400 1200 300"	# hayes
VENTELSPEED="1200 300"	# ventel
#DEFAULTSPEED="1200 300 2400"  # unknown modem type
DEFAULTSPEED="9600 4800 2400 1200 300 150 19200"  # unknown and direct lines

cd $UULIB

## Build database of available terminal special files
##   This next part is dependent on the new devioctl code and the supporting
##   user level changes to the file command.  If it ain't there ...
#
#  the following sed bandaid was necessary due to a last minute change
#  in the file command.  such is life...
#
$FILECMD $DEV/tty* | sed 's/terminal/junk &/' |
  awk '
  NF >= 9 {
	if ($8 != "terminal") continue
	if ($5 == "LAT") continue # skip LAT devices
	if ($5 ~ /^VCB/) continue # skip QDSS devices
	term_name = substr($1,6,5)
	if (term_name == "tty:") continue  # skip /dev/tty
	if (term_name ~ /^ttyd.$/) continue # skip existing dialers
	cntrl_type = $5
	split($6,junk,","); split(junk[1],junk2,"#");cntrl_num = junk2[2]
	split($9,junk,"#"); cntrl_line = junk[2]
	if (NF >= 10 && $10 == "modem_control")
		modemctrl="yes"
	else
		modemctrl="no"

	printf "%-6s  %-10s  %-2s  %-2s   %-7s\n", term_name, cntrl_type, cntrl_num, cntrl_line, modemctrl
  }' > $TTYDB

[ ! -s $TTYDB ] &&   # Check that file exists and is of non-zero length
{
	echo "There are no available tty lines on your system."
	exit 1
} 
set ""`wc -l $TTYDB`
NTTYLINES="$1"  # Number of ttylines available

## find valid ttyd? names we can use
cd $DEV
echo "ttyd0 ttyd1 ttyd2 ttyd3 ttyd4 ttyd5 ttyd6 ttyd7 ttyd8 ttyd9 ttyda ttydb ttydc ttydd ttyde ttydf ttydg ttydh ttydi ttydj ttydk ttydl ttydm ttydn ttydo ttydp ttydq ttydr ttyds ttydt ttydu ttydv ttydw ttydx ttydy ttydz" |
	tr ' ' '\012' > $TMP/getmodems.tmp1$$
ls ttyd? > $TMP/getmodems.tmp2$$ 2>/dev/null
TTYDNAMES=`comm -23 $TMP/getmodems.tmp1$$ $TMP/getmodems.tmp2$$`
rm -f $TMP/getmodems.tmp1$$ $TMP/getmodems.tmp2$$
cd $UULIB
## DEBUG echo "TTYDNAMES --> "$TTYDNAMES

[ ! -s $MODEMDB ] &&   # Make sure the modemdb exists
{
	echo \
"# Modem Datebase - built by uucpsetup
#
# tty   cntrl	 cntrl  line   modem    modem	modem	 out/in	  old tty
# name  name	 number number control  type	speed	 shared	  name
#=====  ======   =====  ====== =======  ======  =====    ======   =======" \
	>> $MODEMDB
	chown uucp $MODEMDB 2>$NULL
	chgrp daemon $MODEMDB 2>/$NULL
}

rm -f $MDDBTMP	
[ -f $MDDBTMP ] &&
{
	echo "Can't remove temporary modem database."
	exit 1
}
touch $MDDBTMP ||
{
	echo "Can't create temporary modem database."
	exit 1
}

# Set 19 as the maximum number of modems anyone can setup at once
[ "$NTTYLINES" -gt 19 ] && NTTYLINES=19  

## Get number of modems to be configured
while :
do
	echo $MN "How many modems are you adding to this system [1]? "$BC
	read NMODEMS
	case "$NMODEMS" in
	[0-9]|[1][0-9]) # < 20 is reasonable
		[ "$NMODEMS" -gt "$NTTYLINES" ] && 
		{
			echo " "
			echo "You only have $NTTYLINES tty lines available."
			echo " "
			continue
		}
		break
		;;
	"") 
		NMODEMS=1
		break
	   	;;
	*) 
		echo " "
		echo "Please enter a number between 0 and $NTTYLINES."
		echo " "
		continue
		;;
	esac
done
echo " "

## Build a list of available modems
MODEMHELPTABLE=`
echo "
The following is a table of modem types you may choose from.  Please
pick a name from the column entitled \"MODEM TYPE\".  

MODEM TYPE	DESCRIPTION
----------	-----------"
sed -n '/^[a-zA-Z]/s/\([^|:]*\).*|\([^|:]*\):.*/\1		\2/p' < $ACUCAP
echo "direct		Direct connect tty line (no modem)"
echo " "
`
MODEMHELP='eval echo "$MODEMHELPTABLE"'

## Build a list of tty lines and their physical locations to aid the
## user in deciding where to place the modems.
TTYHELPTABLE=`
echo '
The following is a table of tty lines and their physical locations.  Pick
a tty line from the column entitled "TTY LINE".  If the word "--More--"
is displayed at the bottom of the screen you may press the SPACEBAR to display
the next page of information or type "q" if you want to stop viewing the 
table and wish to continue answering the questions.'
echo ""
echo "TTY LINE  CNTRL NAME  CNTRL NUMBER  LINE NUMBER  TYPE OF CONNECTION ALLOWED"
awk '{
	if ((NR-1) % 4 == 0)
		print "--------  ----------  ------------  -----------  --------------------------"
	if ($5 == "yes") 
		connect="shared line support"
	else
		connect="incoming or outgoing only"
	printf "%-8s  %-10s  %-12s  %-12s %-12s\n",$1,$2,$3,$4,connect
}
' $TTYDB
echo " "
`  

# in case user is used to typing SIGINTR to more(1) we temporarily ignore faults
TTYHELP='eval ONINTR="$IGNORE"; 
	 echo "$TTYHELPTABLE" | more; 
	 ONINTR="$REPEATORDIE"'

## Start collecting info on the modems
i=1 
while [ $i -le "$NMODEMS" ]
do
	## Get modem type
	done=no
	while :
	do
		echo $MN \
		 'Enter the MODEM TYPE of modem number '$i', "?" for help: '$BC
		read MDTYPE
		case "$MDTYPE" in
		"")	continue
			;;
		"?")	$MODEMHELP
			continue
		      	;;
		esac
		# check that modem is a known type
		echo "$DIALERS" | grep -ws "$MDTYPE" &&
		{
			# Matched
			break
			:
		}||{
			# No match, try again
			echo " "
			echo "That modem type is not known."
			$MODEMHELP
			continue
		}
	done

	## Find out where to put modem
	echo ""
	while :
	do
		echo $MN \
	   'Enter the TTY LINE to attach modem number '$i' to, "?" for help: '$BC
		read MDLINE
		case "$MDLINE" in
		"")	continue
			;;
		"?")	$TTYHELP
			continue
			;;
		esac
		# First make sure the line wasen't already specified.
		# The tty line name is remembered in the 9'th field.
		BUFF=`awk ' $9 == "'$MDLINE'" {print ; exit}' $MDDBTMP`
		case "$BUFF" in
		"") :   # ok
		    	;;
		*) 	echo "
That line has already been used, please try another.
"
			continue
			;;
		esac

		# extract tty record from tty database
		TTYRECORD=` awk '$1 == "'$MDLINE'" {print ; exit}' $TTYDB`
		case "$TTYRECORD" in
		"")	echo "
There is no tty line by that name, please pick another.
"
			continue
			;;
		esac
		set ""$TTYRECORD
		case "$#" in	# sanity check
		5)	:  # ok
			;;
		*)	echo "There is something wrong with the tty database for that line."
			echo "Please pick another."
			continue
			;;
		esac
		## break out stuff from TTYRECORD
		MDCTRLTYPE=$2 MDCTRLNUM=$3 MDLINENUM=$4 MDMODEMCTRL=$5

		# Remove line from TTYHELP
		TTYHELPTABLE=`echo "$TTYHELPTABLE" | grep -wv "$MDLINE"`
		break
	done

	## What speed will modem run at
	echo ""
	case "$MDTYPE" in
	df02)	SPEEDS="$DF02SPEED" ;; 	df03)	SPEEDS="$DF03SPEED" ;;
	df112*) SPEEDS="$DF112SPEED" ;; df124*) SPEEDS="$DF124SPEED" ;;
	df224*)	SPEEDS="$DF224SPEED" ;; hayes)  SPEEDS="$HAYESSPEED" ;;
	hayes-p) SPEEDS="HAYESSPEED" ;;	ventel)	SPEEDS="$VENTELSPEED" ;; 
	*)	SPEEDS="$DEFAULTSPEED" ;;
	esac
	set ""$SPEEDS
	while :
	do
		echo $MN "Enter the SPEED (baud rate) modem $i runs at, "'"?" for help.'" [$1]: "$BC
		read MDSPEED
		case "$MDSPEED" in
		"?")	echo " "
			echo "Valid SPEEDS are: $SPEEDS"
			echo " "
			continue
			;;
		"")	MDSPEED=$1
			;;
		esac
		echo "$SPEEDS" | grep -ws $MDSPEED &&
		{
			break	# was a valid speed
			:
		}||{
			echo "That is not a valid speed for MODEM TYPE \"$MDTYPE\""
			echo " "
			echo "A valid speed is one of the following: $SPEEDS"
			echo " "
			continue
		}
	done

	## What direction is the modem used in: shared, outgoing, incoming
	echo ""
	while :
	do
		case "$MDMODEMCTRL" in
		yes)	
			echo $MN \
'Setup modem to make outgoing calls, accept incoming calls, or both.
Specify "out" for outgoing, "in" for incoming, or "shared" for both.

Enter "out", "in", or "shared" [shared]: '$BC
			RESPONSES="out in shared"
			;;
		no)
			echo $MN \
'Setup modem to make outgoing calls or accept incoming calls.
Specify "out" for outgoing, "in" for incoming.

Enter "out" or "in". [out]: '$BC
			RESPONSES="out in"
			;;
		*)	echo "Invalid modem control entry from database - error"
			exit 1
			;;
		esac
		read MDDIRECTION
		case "$MDDIRECTION" in
		"")	case "$MDMODEMCTRL" in
			yes)	
				MDDIRECTION="shared"
				;;
			no) 
				MDDIRECTION="out"
			esac
		;;
		esac
		
		echo "$RESPONSES" | grep -ws "$MDDIRECTION" &&
		{
			break	# valid response
			:
		}||{
			echo "
Inappropriate response, please try again.
"
		}
	done

	## Use the first available ttyd? name for this modem
	##  - except for direct connect lines (which are not modems)
	case "$MDTYPE" in
	direct)	
		MDNEWNAME=$MDLINE  # stays the same
		;;
	*)
		set ""$TTYDNAMES
		MDNEWNAME=$1	   # becomes ttyd?
		shift
		TTYDNAMES="$*"
		;;
	esac
	
	## All done with this modem, write out entry to temp modem database
	A=$MDNEWNAME B=$MDCTRLTYPE C=$MDCTRLNUM D=$MDLINENUM E=$MDMODEMCTRL
	F=$MDTYPE G=$MDSPEED H=$MDDIRECTION I=$MDLINE

	echo $A $B $C $D $E $F $G $H $I |
	  awk  '{
		printf "%-6s  %-8s  %-2s      %-2s   %-4s    %-8s %-6s  %-8s  %-8s\n",\
		$1, $2, $3, $4, $5, $6, $7, $8, $9 
		}' >> $MDDBTMP

	## increment modem counter
	echo ""
	i=`expr $i + 1`
done

## Now verify that all information is correct, and if not we re-exec 
## ourself.
echo "
The following table lists the information you entered for the modem(s).
Look at it carefully and decide if the information is correct.  If it
is not you will be given a chance to re-enter the information.

MODEM INFORMATION:
"

RESPONSES=`   ## Build response table
echo "
MODEM    MODEM   IN/OUT  TTY     CNTRL  CNTRL   LINE
TYPE     SPEED   SHARED  LINE    NAME   NUMBER  NUMBER
------   -----   ------  -----   -----  ------  ------"
awk  '{
	printf "%-8s %-5s   %-6s  %-5s   %-5s    %-4s    %-4s\n",\
	$6, $7, $8, $9, $2, $3, $4
	}'  $MDDBTMP
echo "
`	## end response table

echo "$RESPONSES"

while :
do
	echo $MN "
Enter \"y\" if all the information is correct, \"n\" if you would like to
start over, \"q\" if you would like to abort adding modems, or \"?\" to 
re-display the table.

Enter \"y\", \"n\", \"q\", or \"?\" [y]: "$BC
	read buff
	case "$buff" in
	"" | y*)
		break
		;;
	n*)
		echo "
Starting over...
"
		exec $MYNAME $*
		## NOTREACHED
		;;
	"?")
		echo "$RESPONSES"
		;;
	q*)	
		rm -f $MDDBTMP
		exit 1
		;;
	esac
done

case "$MDTYPE" in 
direct) ;;
*)
## Parse /etc/remote.  If a dial entry is free we mark it as "UNUSED",
## otherwise we mark it with the device type currently being used. We
## are currently left with the restriction that all modems on a given
## rotary (one per leaf) must be of the same type.  If this weren't
## the case then all modems at a given speed could be added on for use by
## tip, no questions asked. 
## The remote file is parsed out as shell variable entries named:
##
##       TIP${SPEED}=value, where value is either "UNUSED" or the acu type
##
## E.g.  If the entry "dial1200" already had a df112 modem on it then the
##       shell variable TIP1200 would be set to "df112".
eval `awk '
	/^dial[0-9]*\|/ {
		speed=substr($0,5,index($0,"|")-5)
		if (speed != "300" && speed != "1200" \
		    && speed != "2400" && speed != "9600" \
		    && speed != "19200")
			next  # sanity check
		getline(s)  # next line has attributes we want (or better have)
		# parse attributes
		nattrs = split($0, attr, ":")
		unused="false"
		for (i=1; i<=nattrs; i++) {
			if (attr[i] ~ /^dv=/) {
				n = split(attr[i], dvs, "=")
				if (n != 2) {
					continue # bad entry - skip
				} else if (dvs[2] == "") {
					unused="true"
				} 
			} else if (attr[i] ~ /^at=/) {
				nats = split(attr[i], ats, "=")
				if (nats != 2) {
					continue # bad entry - skip
				} else if (ats[2] == "") {
					unused="true"
				} else {
					type=ats[2]
				}
			}
		}
		if (unused == "true") type="UNUSED"
		printf "TIP%s=\"%s\"\n", speed, type
				
	}
' $REMOTE `
## Build generic ed substitution string for adding tip entry
EDTIP=\
'H
/^dial$speed|/+1s/dv=[^:]*/&,\\/dev\\/$newname/p
s/dv=,/dv=/p
s/at=[^:]*/at=$type/p
s/cu=[^:]*://p
w
q
'
	;;
esac #if !direct
#for i in 300 1200 2400 4800 9600; do eval type=\$TIP$i echo "DIAL$i=$type" done

## Do the work
trap '' $STDSIGS

echo "
The following special device files in /dev have been changed as follows:"

exec <$MDDBTMP
while :
do
	read record
	case "$?" in
	0) : ;;
	*) 	break
		;;
	esac
	set - $record
	newname=$1 modemctrl=$5 type=$6 speed=$7 dir=$8 oldname=$9
	echo "$type" | grep -ws "direct" &&
	{ 	
		:
	}||{
		mv $DEV/$oldname $DEV/$newname
	}

	chmod 666 $DEV/$newname
	
	# ***ALERT*** this part will need fixing up in the future
	# We don't have getty entries defined for any other modem speeds.
	case "$speed" in
	19200)	GETTYENTRY="std.19200\"" ;;
	9600)	GETTYENTRY="std.9600\"" ;;
	2400)	GETTYENTRY="H2400\"" ;;
	1200)	GETTYENTRY="D1200\"" ;;
	300)	GETTYENTRY="D300\" "  ;;
	*)	echo "
WARNING: Can't setup a $speed baud modem yet, skipped...
"
		continue
		;;
	esac
	
	case "$dir" in
	shared)	STATE="on " SHARED="shared" MODEM="modem  " ;;
	out)	STATE="off" SHARED="      " MODEM="modem  " ;;
	"in")	STATE="on " SHARED="      " MODEM="modem  " ;;
	esac

	case "$type" in 
		"direct")
			modemctrl="no" 
			DIALUP="vt100" 
			AorD="DIR"
			;;
		*)	DIALUP="dialup"
			AorD="ACU"
			;;
	esac
	
	case "$modemctrl" in
	yes)	: ;;
	no) 	MODEM="nomodem" ;;
	esac

	TTYENTRY="$newname \"/etc/getty $GETTYENTRY $DIALUP $STATE $MODEM $SHARED # oldname=$oldname, $type"

	ed - $TTYS <<EOF  1>>$NULL 2>&1
H
g/^$oldname/d
/^#*console/
a
$TTYENTRY
.
w
q
EOF
	
	case "$dir" in "shared"|"out")
	    echo "g/$newname/d\nw\nq" | ed - $LDEVICES
	    echo "$AorD	$newname $newname $speed $type" >> $LDEVICES
	esac

case "$type" in
	"direct") ;;
	*)
	echo "
	/dev/$oldname has been renamed as /dev/$newname"
	
	# If modem can be used by tip then add it to /etc/remote
	case "$dir" in "shared"|"out")
		eval tiptype=\$TIP$speed
		case "$tiptype" in "UNUSED" | "$type") 
			eval EDSTRING=\""$EDTIP"\"    # We can use it
			#echo "$EDSTRING" | ed - $REMOTE 1>>$NULL 2>&1
			echo $EDSTRING
			echo "$EDSTRING" | ed - $REMOTE 
			echo "	and has been made available for use by tip"
			eval TIP$speed=$type  # set it in case it was UNUSED
			;;
		esac
		;;
	esac
	;;
esac
	
	echo " $record" >> $MODEMDB  # record it in modem database

done	# loop

chown uucp $LDEVICES 2>/dev/null
chmod 400 $LDEVICES 2>/dev/null

kill -HUP 1
rm -f $MDDBTMP
echo "
The list of installed modems on your system is available in the
file: /usr/var/uucp/modemdb.  You should now physically connect
the modems to your system if you haven't already.
"

exit 0
#									
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
#	EDIT HISTORY:
#
#	1-Jun-1986	XXX	- first version
#	16-Feb-1988	Lea Gottfredsen 
#				- /usr/lib/uucp -> /usr/var/uucp
#	1-23-1989	Lea Gottfredsen
#				- added support for direct connect lines
#
