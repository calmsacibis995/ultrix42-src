#ifndef lint
static char *sccsid = "@(#)subr_xxx.c	4.2 (ULTRIX) 11/9/90";
#endif

/************************************************************************
 *									*
 *			Copyright (c) 1986, 1987 by			*
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

#include "../machine/pte.h"

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/conf.h"
#include "../h/gnode.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/buf.h"
#include "../h/proc.h"
#include "../h/vm.h"
#include "../h/cmap.h"
#include "../h/uio.h"

/*
 * Routine placed in illegal entries in the bdevsw and cdevsw tables.
 */
nodev()
{

	return (ENODEV);
}

/*
 * Null routine; placed in insignificant entries
 * in the bdevsw and cdevsw tables.
 */
nulldev()
{

	return (0);
}

/*
 * Null system calls. Not invalid, just not configured.
 */
errsys()
{
	u.u_error = EINVAL;
}

nullsys()
{
}

imin(a, b)
{

	return (a < b ? a : b);
}

imax(a, b)
{

	return (a > b ? a : b);
}

unsigned
min(a, b)
	u_int a, b;
{

	return (a < b ? a : b);
}

unsigned
max(a, b)
	u_int a, b;
{

	return (a > b ? a : b);
}

#ifndef vax
#ifndef mips
ffs(mask)
	register long mask;
{
	register int i;

	for(i = 1; i < NSIG; i++) {
		if (mask & 1)
			return (i);
		mask >>= 1;
	}
	return (0);
}

bcmp(s1, s2, len)
	register char *s1, *s2;
	register int len;
{

	while (len--)
		if (*s1++ != *s2++)
			return (1);
	return (0);
}

strlen(s1)
	register char *s1;
{
	register int len;

	for (len = 0; *s1++ != '\0'; len++)
		/* void */;
	return (len);
}
#endif !mips

strncmp(s1, s2, n)
register char *s1, *s2;
register int n;
{

	while (--n >= 0 && *s1 == *s2++)
		if (*s1++ == '\0')
			return(0);
	return(n<0 ? 0 : *s1 - *--s2);
}

char *
strcpy(dst, src)
register char *dst, *src;
{
	while (*dst++ = *src++)
		continue;
	return (dst - 1);
}

strcmp(s1, s2)
register char *s1, *s2;
{

	while (*s1 == *s2++)
		if (*s1++=='\0')
			return(0);
	return(*s1 - *--s2);
}

#ifdef mips
skpc(c, len, cp)
	register char c;
	register u_short len;
	register char *cp;
{
	if (len == 0)
		return (0);
	while (*cp++ == c && --len)
		;
	return (len);
}
#endif mips

/*
 * Copy s2 to s1, truncating to copy n bytes
 * return ptr to null in s1 or s1 + n
 */
char *
strncpy(s1, s2, n)
	register char *s1, *s2;
{
	register int i;

	for (i = 0; i < n; i++) {
		if ((*s1++ = *s2++) == '\0') {
			return (s1 - 1);
		}
	}
	return (s1);
}

char *
index(strp, c)
register char *strp;
register char c;
{
	for (; *strp; strp++)
		if (*strp == c)
			return(strp);
	return(NULL);
}
#endif !vax

/*
 * This atoi came with minor mods from libc.
 * It's intended use is for those
 * few cases where the conversion of a string to
 * an integer in the kernel is required like
 * generic boot code. Please do not abuse it.
 */
atoi(s1)
register char *s1;
{
        register int n;
        register int f;

        n = 0;
        f = 0;
        for(;;s1++) {
		switch(*s1) {
                case ' ':
                case '\t':
                        continue;
                case '-':
                        f++;
                case '+':
                        s1++;
                }
                break;
	}
	while(*s1 >= '0' && *s1 <= '9')
                n = n*10 + *s1++ - '0';
        return(f ? -n : n);
}
