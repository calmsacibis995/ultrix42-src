:
# @(#)unlockpw.sh	4.1 (ULTRIX) 7/2/90
#
#									
# 			Copyright (c) 1984 by				
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
# Purpose:	Unlock the passwd and group files
# Usage:	unlockpw
# Environment:	Bourne shell script
# Date:		3/15/84
# Author:	afd
# 
# Remarks:
#    Removes the lock file that the adduser script looks for.
#

# Trap ^c signal

trap "" 2
LOCK_FILE="/etc/ptmp"

# Make sure that we are "root"

if test ! -w /etc/passwd
then
    echo "Please su to root first."
    exit 1
fi

# Remove the lock file

rm -f $LOCK_FILE
exit 0
