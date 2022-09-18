#ifndef lint
static CHTYPE *sccsid = "@(#)sub.c	4.1 (ULTRIX) 7/17/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1986 by				*
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
/*
 *
 * Modification History
 * ~~~~~~~~~~~~~~~~~~~~
 * 
 * 001	David Lindner 26-Sep-89
 *	Added dynamic memory allocation code for ctow and wtoc
 *	routines. Defined MAXBUF for internal use in these routines,
 *	and externally declared malloc.
 *	
 *
 */

/*
 * DJL 001
 */
#include <strings.h>
#define MAXBUF	1024
#define alloc	malloc
extern CHTYPE	*alloc();

/*
 * DJL 001
 * Modified routine to use dynamic memory allocation.
 */

CHTYPE *
ctow(s)
register char *s;
{
	static CHTYPE wbuf[MAXBUF];
	static CHTYPE *d_wbuf;
	static unsigned d_wbufsize=0;
	register CHTYPE *wp;
	register unsigned actlen;

	actlen = (strlen(s) * sizeof(CHTYPE)) / sizeof(char);
	if (actlen > MAXBUF) {
		if (actlen > d_wbufsize) {
			if (d_wbufsize)
				free(d_wbuf);
			/* Determine buffer size to nearest kbyte */
			d_wbufsize = ((actlen/1024) + 1) * 1024;
			d_wbuf = alloc(d_wbufsize);
			}
		}
	else {
		if (d_wbufsize) {
			d_wbufsize = 0;
			free(d_wbuf);
			}
		d_wbuf = wbuf;
		}

	wp = d_wbuf;
	while (*wp++ = *s++)
		;

	return d_wbuf;
}


/*
 * DJL 001
 * Modified routine to use dynamic memory allocation.
 */

char *
wtoc(wp)
CHTYPE *wp;
{
	static char cbuf[MAXBUF];
	static char *d_cbuf;
	static unsigned d_cbufsize=0;
	register char *s;
	register unsigned actlen;

	actlen = length(wp);
	if (actlen > MAXBUF) {
		if (actlen > d_cbufsize) {
			if (d_cbufsize)
				free(d_cbuf);
			/* Determine buffer size to nearest kbyte */
			d_cbufsize = ((actlen/1024) + 1) * 1024;
			d_cbuf = (char *)alloc(d_cbufsize);
			}
		}
	else {
		if (d_cbufsize) {
			d_cbufsize = 0;
			free(d_cbuf);
			}
		d_cbuf = cbuf;
		}

	s = d_cbuf;
	while (*s++ = *wp++)
		;
	
	return d_cbuf;
}

readw(fd, w, l)
int fd;
register CHTYPE *w;
int l;
{	char buf[1024];
	register i;
	register char *cp = buf;
	int len;

	i = len = read(fd, cp, l);
	while (i-- > 0)
		*w++ = *cp++;
	return len;
}


char **
convargs(list, start)
register CHTYPE *list[];
register char **start;
{	register char **ptr;
	register int i;
	/*
	 * count number of arguments
	 */
	for (i = 0; list[i]; i++)
		;
	/*
	 * reserve pointer space  nor forgetting the NULL pointer
	 */
	ptr = &start[i + 1];
	/*
	 * copy and convert arguments
	 */
	for (i = 0; list[i]; i++)
	{
		*start++ = (char *) ptr;
		ptr += swtoc(ptr, list[i]) + 1;
	}
	*start = (char *)0;
	/* 
	 * return pointer to end of space
	 */
	return ptr;
}

swtoc(to, from)
register char *to;
register CHTYPE *from;
{	register int i;

	i = 0;
	while (*to++ = *from++)
		i++;
	return i;
}
