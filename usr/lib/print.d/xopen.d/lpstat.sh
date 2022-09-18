#!/usr/bin/ksh
#
# @(#)lpstat.sh	4.2      ULTRIX 	10/16/90
#
# murdoch 21/11/89
#
#************************************************************************
#*									*
#*			Copyright (c) 1989, 1990 by			*
#*		Digital Equipment Corporation, Maynard, MA		*
#*			All rights reserved.				*
#*									*
#*   This software is furnished under a license and may be used and	*
#*   copied  only  in accordance with the terms of such license and	*
#*   with the  inclusion  of  the  above  copyright  notice.   This	*
#*   software  or  any  other copies thereof may not be provided or	*
#*   otherwise made available to any other person.  No title to and	*
#*   ownership of the software is hereby transferred.			*
#*									*
#*   The information in this software is subject to change  without	*
#*   notice  and should not be construed as a commitment by Digital	*
#*   Equipment Corporation.						*
#*									*
#*   Digital assumes no responsibility for the use  or  reliability	*
#*   of its software on equipment which is not supplied by Digital.	*
#*									*
#************************************************************************/

# Modification History:
#
# 01-oct-90 - Adrian Thoms
#	Changed to /usr/bin/ksh so as to avoid numbered parameters being
#	stomped by shell functions
#	Improved -u option so that it runs in same order time, regardless
#	of how many users we are looking for.
#	Cleaned up use of test

LPC_PGM="/etc/lpc"
printers=`$LPC_PGM status| grep :|cut -f1 -d:`

sched () 
{	
	if (ps ax | grep -s lpd$)
	then
		echo "lp daemon present"
	else
		echo "no lp daemon present"
	fi
}
			
defl ()
{
	echo "system default destination: " `grep '|lp|' /etc/printcap | cut -f1 -d"|"`
}

queues () {
	for printer
	do
		echo "$printer:"
		lpq -P$printer|awk "
BEGIN		{ EMPTY = 0; }
/no entries/	{ EMPTY = 1 }
		{
		 	if ( EMPTY == 0 ) {
				print \$0;
			}
		}
END		{ print \"\"; }
"
	done
}

users () {
	typeset INITIALISATION=
	integer n

	for printer in $printers
	do
		INITIALISATION=	
		(( n = 0 ))
		for user ; do
			(( n += 1 ))
			INITIALISATION="${INITIALISATION}
			user[$n] = \"$user\";
"
		done
		lpq -P$printer | awk "
BEGIN		{ 
${INITIALISATION}
		}
		{
			for ( i = 1; i <= $n; i++ ) {
				if ( \$2 == user[i] ) {
					printf \"%s:\\t%s\\n\", \"$printer\", \$0;
				}
			}
		}
"
	done
}


#
# The main body of lpstat ...
#

if [[ $# -eq 0 ]]
then 
	users $USER
fi

progname=${0#/}
progname=${progname##*/}

while [[ $# -ne 0 ]]; do
	ARG="$1"
	case "$ARG" in
		 -a)
			for printer in $printers
			do
				echo "$printer :" `$LPC_PGM stat $printer|fgrep queuing`
		 	done
			shift
			;;
		 -a*)
			ARGS2="${ARG#-a}"
			ARGS2=$(echo $ARGS2 | tr ',' ' ')
			for printer in $ARGS2
			do
				if ($LPC_PGM stat $printer | grep -s queuing) 
				then
					echo $printer':' `$LPC_PGM stat $printer | grep queuing`
				else
					$LPC_PGM stat $printer
				fi
			done
			shift
			;;

		 -c)
			echo "${progname}: Printer class? Not implemented"
			shift
			;;

		 -o)
			queues $printers
			shift
			;;

		 -o*)
			ARGS2="${ARG#-o}"
			ARGS2=$(echo $ARGS2 | tr ',' ' ')
			queues $ARGS2
			shift
			;;
		
		-d)
			defl
			shift
			;;

		-r)
			sched
			shift
			;;

		-s)
			sched
			defl
			shift
			;;

		-t)
			sched
			defl
			queues $printers
			$LPC_PGM status
			shift
			;;			

		 -u)
			queues $printers
			shift
			;;

		 -u*)
			ARGS2="${ARG#-u}"
			ARGS2=$(echo $ARGS2 | tr ',' ' ')
			users $ARGS2
			shift
			;;
								
		 -p)
			$LPC_PGM status
			shift
			;;

		 -p*)			
			ARGS2="${ARG#-p}"
			ARGS2=$(echo $ARGS2 | tr ',' ' ')
			$LPC_PGM status $ARGS2
			shift
			;;

		 -v)
			echo "${progname}: See /etcprintcap file for printer information"
			shift
			;;
		
		 *)
			echo "invalid option: $1"
			echo "usage:  ${progname} [-a[list]] [-d] [-o[list]] [-p[list]] [-r] [-s] [-t] [-u[list]] "
			shift
			;;

	esac
done
#end of lpstat
































