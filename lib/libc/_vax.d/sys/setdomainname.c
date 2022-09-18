/*	@(#)setdomainname.c	1.1	(ULTRIX)	3/26/86 */

#include "SYS.h"

SYSCALL(setdomainname)
	ret		# setdomainname(buf, buflen)
