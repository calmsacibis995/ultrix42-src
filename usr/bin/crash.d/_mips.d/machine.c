#ifndef lint
static char *sccsid = "@(#)machine.c	4.2	(ULTRIX)	7/17/90";
#endif

/************************************************************************
 *									*
 *			Copyright (c) 1988 by				*
 *		Digital Equipment Corporation, Maynard, MA		*
 *			All rights reserved.				*
 *									*
 *   This software is furnished under a license and may be used and	*
 *   copied  only  in accordance with the terms of such license and	*
 *   with the  inclusion  of  the  above  copyright  notice.   This	*
 *   software  or  any  other copies thereof may not be provided or	*
 *   otherwise made available to any other person.  No title to and	*
 *   ownership of the software is hereby transferred.			*
 *									*
 *   This software is  derived  from  software  received  from  the	*
 *   University    of   California,   Berkeley,   and   from   Bell	*
 *   Laboratories.  Use, duplication, or disclosure is  subject  to	*
 *   restrictions  under  license  agreements  with  University  of	*
 *   California and with AT&T.						*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/
#include "../crash.h"
static long *buf,*p, *end;
static int size;
dis(start, ssize)
	char *start, *ssize;
{
	unsigned int loc;
	long get_bytes();
	int done=0;
	int offset = 0;
	char *offp;
	struct Symbol *sp, *nmsrch();


	if(offp = rindex(start, '+')) {
		*offp = '\0';
		if(isdigit(*++offp)) {
			sscanf(offp, "%x", &offset);
		}
	}

	if((sp = nmsrch(start)) == NULL) {
		if(isdigit(start[0])) {
			loc = scan_vaddr(start)+offset;
		} else {
			printf("symbol '%s' not found\n", start);
			return;
		}
	} else 
		loc = sp->s_value + offset;

	size=atoi(ssize);
	buf = p = (long *) malloc(size);
	readmem(buf, loc, size);
	while (done < size)
		done += disassembler(loc, 0, 0, 0, get_bytes, 0);
}
long
get_bytes() {
		return(*p++);
}
