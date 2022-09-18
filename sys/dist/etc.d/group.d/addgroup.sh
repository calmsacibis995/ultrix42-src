#!/bin/sh5
#
# @(#)addgroup.sh	4.2 (ULTRIX) 8/7/90
#									
# 			Copyright (c) 1984, 1989 by				
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
# Purpose:	Add a new group number to /etc/group
# Usage:	addgroup
# 
# Remarks:
#    Returns the group number.  Uses negative return values for errors.
#

# Make sure its root.
#
PATH=/etc/sec:/usr/etc/sec:/bin:/usr/bin:/usr/ucb
export PATH
umask 027
if test ! -w /etc/group
then
    echo 'Please su to root first.'
    exit 1
fi
#
# See if this system is a Yellow Pages client.
#
if tail -1 /etc/group | grep -s '^[+-]:'
  then
    cat << EOF
This system makes use of the Yellow Pages service for
group administration.  Please add the new group by
following the steps given in the Overview of Yellow
Pages chapter of the Network Management Guide.
EOF
    exit 5
fi
#
if test "${#}" -lt 1
  then
#
# Lock the passwd and group files
#
    trap '/etc/unlockpw ; exit 1' 1 2
    /etc/lockpw
    exstat="${?}"
    if test $exstat != 0
      then
	exit $exstat
    else
	trap '/etc/unlockpw ; exit 0' 0
    fi
fi
#
# Get new group name
#
while true
  do
    if test "${#}" -lt 1
      then
	echo 'Enter group name for new group: ' \\c
	if read GROUP
	  then
	    true
	else
	    exit 1
	fi
    else
	GROUP="${1}"
    fi
    case "${GROUP}"
      in
	*:*)
	    echo 'Error, illegal characters in group name.'
	    ;;
	'')
	    echo 'Error, empty group name.'
	    ;;
	?????????*)
	    echo 'Error, group name too long.'
	    ;;
	*)
#
# See if group already exists in passwd file, if so exit.
#
	    if grep -s "^${GROUP}:" /etc/group
	      then
		echo "Error, group ${GROUP} already in /etc/group file."
	    else
		break
	    fi
	    ;;
    esac
    if test "${#}" -ge 1
      then
	exit 1
    fi
done
#
# Get the group number for the new user.  Sort the group file on gid,
# get the largest gid, validity check it as a valid number,
# then add 5 to it to get the new gid.
#
GID=`sed -n '/^[^:]*:[^:]*:[0-9][0-9]*:/p' < /etc/group | sort -nt: +2 -3 | tail -1 | cut -d: -f3`
#
# Check for valid $gid
#
if test ! "${GID}"
  then
    echo
    echo 'Error, the /etc/group file is corrupt!'
    echo "Exiting ${0} script.  New entry not created!"
    exit 1
fi
DEFGID=`expr "${GID}" + 5`
#
while true
  do
    echo
    echo "Enter group number for new group [${DEFGID}]: " \\c
    if read GNUM
      then
	case "${GNUM}"
	  in
	    '')
		GNUM=${DEFGID}
		break
		;;
	    *)
		if expr "${GNUM}" : '[^0-9]' > /dev/null
		  then
		    echo 'Error, group number must be all digits.'
		else
#
# See if group number already exists in group file, if so get another.
#
		    if grep -s "^[^:]*:[^:]*:${GNUM}:" /etc/group
		      then
			echo "Error, Group number ${GNUM} is already in /etc/group file."
		    else
			break
		    fi
		fi
		;;
	esac
    else
	exit 1
    fi
done
#
# Add the group to the /etc/group file
#
echo "${GROUP}:*:${GNUM}:" >> /etc/group
exit ${GNUM}
