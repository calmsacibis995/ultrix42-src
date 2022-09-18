/*	@(#)getdomainname.c	1.1	(ULTRIX)	3/26/86 */

#include "SYS.h"

SYSCALL(getdomainname)
	ret		# len = getdomainname(buf, buflen)
