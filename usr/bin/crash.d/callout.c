#ifndef lint
static char *sccsid = "@(#)callout.c	4.1	(ULTRIX)	7/17/90";
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

#include	"crash.h"
#include	<sys/callout.h>
#include	<sys/map.h>

prcallout()
{
	register  struct  Symbol  *sp;
	struct    callout  callout;
	struct    Symbol *search();
	unsigned  int nxt;	/* core offset of next callout entry */

	nxt = tab[CALLOUT_T].first;
	while (nxt) {
		if(readmem((char *)&callout, (int)nxt, sizeof callout) !=
		    sizeof callout) {
			error("read error on callout table");
			return;
		}

		if(callout.c_func == NULL) return;
		sp = search((unsigned)callout.c_func);
		if(sp == NULL) {
			printf("unknown function 0x%x\n", callout.c_func);
		} else {
			if (sp->s_name[0] == '_')
				printf("%-15.15s  ",&(sp->s_name[1]));
				else printf("%-16.16s  ",sp->s_name);
			printf("%8.8x", callout.c_arg);
			printf(" %5u\n",callout.c_time);

		}
		nxt = (unsigned int)callout.c_next;
	}
}

/*
prmap(s)
	register  char  *s;
{
	struct	map	mbuf[NMAP];
	register  int  i;
	unsigned  free = 0, seg = 0;
	register  struct  Symbol  *sp;

	printf("%s\n", s);
	if((sp = nmsrch(s)) == NULL) {
		printf("symbol not found\n");
		return;
	}
	printf("address  size\n");
	lseek(mem, SYM_VALUE(sp), 0);
	for(;;) {
		if(read(mem, mbuf, sizeof mbuf) != sizeof mbuf) {
			error("read error on map structure");
			return;
		}
		for(i = 0; i < NMAP; i++) {
			if(mbuf[i].m_size == 0) {
				printf("%u segments, %u units\n", seg, free);
				return;
			}
			printf(FMT, mbuf[i].m_addr);
			printf(" %5u\n", mbuf[i].m_size);
			free += mbuf[i].m_size;
			seg++;
		}
	}
}
*/
