#!/bin/sh5
#
#  @(#)adduser.sh	4.3 (ULTRIX) 12/20/90
#
# 			Copyright (c) 1984, 1989, 1990 by
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
# Purpose:	To add new users to the group and passwd file
# Usage:	adduser
# 
# Remarks:
#	Interactive adduser script to ease the process of adding accounts.
#

# The maximum uid should really be read from /usr/include/limits.h 

PATH=/etc/sec:/usr/etc/sec:/bin:/usr/bin:/usr/ucb
export PATH
UID_MAX=32000
umask 022

if test ! -w /etc/passwd
then
    echo "Please su to root first."
    exit 1
fi

# Trap signals

trap '/etc/unlockpw ; exit 1' 1 2

# Lock the passwd and group files

/etc/lockpw
exstat="$?"
if test $exstat -ne 0
then
    exit $exstat
else
    trap '/etc/unlockpw ; exit 0' 0
fi

#
# See if this system is a Yellow Pages client.
#
tail -1 /etc/passwd | grep "^[+-]:" > /dev/null
yp_used="$?"
tail -1 /etc/group | grep "^[+-]:" > /dev/null
if [ $? -eq 0 ] || [ $yp_used -eq 0 ]
then
	/etc/unlockpw
	echo "
This system makes use of the Yellow Pages service for
user account administration.  Please add the new user
by following the steps given in the Overview of Yellow
Pages chapter of the Network Management Guide."
	exit 5
fi

# Get new user's login name

while true
  do
    echo ''
    echo 'Enter login name for new user (initials, first or last name): ' \\c
    if read USER
      then
	case "${USER}"
	  in
	    *:*)
		echo 'Error, login name cannot contain colons.'
		;;
	    '')
		;;
	    ?????????*)
		echo 'Error, login name cannot be longer than 8 characters.'
		;;
	    *)
		break
		;;
	esac
    else
	exit 0
    fi
done

# See if user already exists in passwd file, if so exit.
if grep -s "^${USER}:" /etc/passwd
  then
    /etc/unlockpw
    echo "User ${USER} already in /etc/passwd file."
    exit 5
fi

# Get the user ID for the new user.  Sort the passwd file on uid,
#   get the largest uid, validity check it as a valid number,
#   then add one to it to get the new uid.

UID=`sort -nt: +2 -3 /etc/passwd | tail -1 | cut -d: -f3 | sed -n '/[0-9][0-9]*/p'`

# Check for valid ${UID}

if test ! "${UID}"
  then
    echo ''
    echo "The password file (/etc/passwd) may be corrupt!"
    echo "Exiting ${0} script.  New entry not created!"
    /etc/unlockpw
    exit 1
fi

if test "${UID}" -gt ${UID_MAX}
  then
    echo ''
    echo "A uid greater that the maximum allowed," ${UID_MAX} "was found."
    echo "Exiting ${0} script.  New entry not created!"
    /etc/unlockpw
    exit 1
fi

UID=`expr "${UID}" + 1`
# Verfify UID
while true
  do
    echo "Enter uid for new user [${UID}]: " \\c
    if read X
      then
	case "${X}"
	  in
	    '')
		break
		;;
	    *)
		N=`expr "${X}" : '[^0-9]'`
		if [ ${N} -gt 0 ]
		  then
		    echo "Bad value for UID, must be an integer"
		else
		    if [ ${X} -gt ${UID_MAX} ]
		      then
			echo "UID must be less than ${UID_MAX}"
		    else
			if grep -s "^[^:]*:[^:]*:${X}:" /etc/passwd
			  then
			    echo "There is another account with uid ${X}, is this ok [yes]? " \\c
			    if read Y
			      then
				case "${Y}"
				  in
				    [yY]*|'')
					;;
				    *)
					continue
					;;
				esac
			    else
				exit 1
			    fi
			fi
			UID=${X}
			break
		    fi
		fi
		;;
	esac
    else
	exit 1
    fi
done

# Get new user's real name

while true
  do
    echo 'Enter full name for new user: '\\c
    if read NAME
      then
	case "${NAME}"
	  in
	    *:*)
		echo 'Error, name may not contain colons.'
		;;
	    '')
		;;
	    *)
		break
		;;
	esac
    else
	exit 1
    fi
done

# Get the login group for the new user.

while true
do
while true
  do
    echo 'What login group should this user go into [users]: ' \\c
    if read LOGGROUP
      then
	case "${LOGGROUP}"
	  in
	    *:*)
		echo 'Error, group name may not contain colons.'
		;;
	    '')
		LOGGROUP=users
		break
		;;
	    *)
		break
		;;
	esac
    else
	exit 1
    fi
done

#   Get the group ID for the new user

    GID=`grep "^${LOGGROUP}:" /etc/group | cut -d: -f3`
    if test ! "${GID}"
      then
	echo
	echo "Unknown group: ${LOGGROUP}. Known groups are:"
	echo
	cut -d: -f1 < /etc/group | pr -t -l1 -4
	while true
	  do
	    echo
	    echo "Do you want to add group ${LOGGROUP} to the /etc/group file [yes]? " \\c
	    if read ADDGROUP
	      then
		case "${ADDGROUP}"
		  in
		    [yY]*|'')
			echo ''
			echo 'Adding new group to /etc/group file...'
			/usr/etc/addgroup "${LOGGROUP}"
# Addgroup script returns the gid number.
			GID=${?}
			break 2
			;;
		    [nN]*)
			break
			;;
		    *)
			echo 'Error, invalid response, you must answer yes or no.'
			;;
		esac
	    else
		exit 1
	    fi
	done
    else
	break
    fi
done

LOGGID=${GID}

# Get other groups if this user is to be part of any others

while true
do
while true
  do
    echo "
Enter another group that '${USER}' should be a member of"
    echo "(<RETURN> only if none): " \\c
    if read GROUP
      then
	case "${GROUP}"
	  in
	    *:*)
		echo 'Error, group name may not contain colons.'
		;;
	    '')
		break 2
		;;
	    *)
		break
		;;
	esac
    else
	exit 1
    fi
done

#   Get the group ID for the new user

    GID=`grep "^${GROUP}:" /etc/group | cut -d: -f3`
    if test ! "${GID}"
      then
	echo
	echo "Unknown group: ${GROUP}. Known groups are:"
	echo
	cut -d: -f1 < /etc/group | pr -t -l1 -4
	while true
	  do
	    echo
	    echo "Do you want to add group ${GROUP} to the /etc/group file [yes]? " \\c
	    if read ADDGROUP
	      then
		case "${ADDGROUP}"
		  in
		    [yY]*|'')
			echo ''
			echo 'Adding new group to /etc/group file...'
			/usr/etc/addgroup "${GROUP}"
# Addgroup script returns the gid number.
			GID=${?}
			continue 2
			;;
		    [nN]*)
			break
			;;
		    *)
			echo 'Error, invalid response, you must answer yes or no.'
			;;
		esac
	    else
		exit 1
	    fi
	done
    fi

# Add the user to each group as it is specified 

	grep "^${GROUP}:" /etc/group | cut -d: -f4 | grep -s "${USER}"
	if test $? -eq 0
	then 
	    echo ""
	    echo "  User ${USER} is already a member of group ${GROUP}."
	else
	    rm -f /tmp/group > /dev/null
	    cp /etc/group /tmp/group
	    ed /tmp/group > /dev/null << EOF
	    /^${GROUP}:/s/\$/,${USER}/
	    g/:,/s//:/
	    w
	    q
EOF
	    mv /tmp/group /etc/group
	fi

done

while true
  do
    echo "Enter parent directory for ${USER} [/usr/users]: " \\c
    if read PARENT
      then
	case "${PARENT}" in
	    '') PARENT=/usr/users
		;;
	esac
    else
	exit 1
    fi
    if test \! -d "${PARENT}"
      then
	while true
	  do
	    echo "${PARENT} not found, do you want to create it [yes]? " \\c
	    if read ADDDIR
	      then
		case "${ADDDIR}"
		  in
		    [yY]*|'')
			if mkdir "${PARENT}"
			  then
			    chmod 755 "${PARENT}"
			    chgrp "${LOGGROUP}" "${PARENT}"
			    break 2
			else
			    echo "Unable to create ${PARENT}."
			    break
			fi
			;;
		    [nN]*)
			break
			;;
		    *)
			echo 'Error, you must answer either yes or no.'
			;;
		esac
	    else
		exit 1
	    fi
	done
    else
	break
    fi
done

# Get the users login shell.

while true
  do
    echo 'The shells are:'
    echo
    pr -4 -t /etc/shells
    echo
    echo 'Enter the users login shell name [/bin/csh]: ' \\c
    if read LSHELL
      then
	case "${LSHELL}" in
	    '') LSHELL=/bin/csh
		;;
	    */*)
		;;
	    *) X=`grep "/${LSHELL}$" /etc/shells`
	   	if [ -n "${X}" ]
		  then
		    LSHELL="${X}"
		fi
		;;
	esac
	if [ "${LSHELL}" = /bin/sh ]
	  then
	    LSHELL=''
	fi
    else
	exit 1
    fi
    break
done

echo "
Adding new user ..."

# Make sure parent directories for everything exist.

if test \! -d /usr/spool
then
    mkdir /usr/spool
fi
if test \! -d /usr/spool/mail
then
    mkdir /usr/spool/mail
fi

# Add the user to the password file.

echo "${USER}:Nologin:${UID}:${LOGGID}:${NAME}:${PARENT}/${USER}:${LSHELL}" >> /etc/passwd
if [ -f /usr/etc/mkpasswd -a -f /etc/passwd.pag ]
  then
    echo 'Rebuilding the passwd data base...'
    ( cd /etc ; /usr/etc/mkpasswd -u passwd )
fi

# Add the user to the auth file

if test -f /etc/auth.pag -a -f /usr/etc/sec/setauth
  then

DEFPASSMAXLIFE=60
DEFPASSMINLIFE=0
    echo "Enter maximum password lifetime in days [${DEFPASSMAXLIFE}]: " \\c
    read MAXPASSLIFE
    if test ! "${MAXPASSLIFE}"
      then
	MAXPASSLIFE=${DEFPASSMAXLIFE}
    fi
    MAXPASSLIFE=`expr "${MAXPASSLIFE}" \* 24 \* 60 \* 60`
#
    echo "Enter minimum password lifetime in days [${DEFPASSMINLIFE}]: " \\c
    read MINPASSLIFE
    if test ! "${MINPASSLIFE}"
      then
	MINPASSLIFE=${DEFPASSMINLIFE}
    fi
    MINPASSLIFE=`expr "${MINPASSLIFE}" \* 24 \* 60 \* 60`
#
    echo 'Will machine generated passwords be required [no]? ' \\c
    if read X
      then
	case "${X}"
	  in
	    [Yy]*)
		AUTHMASK=03
		;;
	    *)
		AUTHMASK=07
		;;
	esac
    fi
    echo "${UID}:Nologin:1:${MINPASSLIFE}:${MAXPASSLIFE}:${AUTHMASK}:0:${UID}:00:00:00" \
        | /usr/etc/sec/setauth
    echo 'Do you wish to edit the auth file entry for this user [no]? ' \\c
    if read X
      then
	case "${X}"
	  in
	    [yY]*)
		/usr/etc/sec/edauth "${USER}"
		;;
	esac
    fi
fi

# Create home and bin directories, and set-up files

echo 'Creating home directory...'
cd "${PARENT}"
if [ "${?}" -ne 0 ]
  then
    echo "Unable to cd to parent directory ${PARENT}"
    exit 1
fi
for I in "${USER}" "${USER}/bin"
  do
    if [ -d "${I}" ]
      then
	echo "${PARENT}/${I} already exists."
    else
	mkdir "${I}"
	chmod 0751 "${I}"
	/etc/chown "${USER}" "${I}"
	chgrp "${LOGGROUP}" "${I}"
    fi
done
for I in .profile .login .cshrc
  do
    if [ -s "${USER}/${I}" ]
      then
	echo "${PARENT}/${USER}/${I} already exists."
    else
	cp /usr/skel/${I} "${USER}/${I}"
	chmod 0751 "${USER}/${I}"
	/etc/chown "${USER}" "${USER}/${I}"
	chgrp "${LOGGROUP}" "${USER}/${I}"
    fi
done

# Unlock the password and group files

/etc/unlockpw

# Set a password

echo "Until the password is set for ${USER} they will not be able to login."

while true
  do
    if passwd "${USER}"
      then
	break
    fi
done
exit 0
