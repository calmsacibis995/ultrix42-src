#ifndef lint
static char *sccsid = "@(#)dcl_xar.c	4.1	ULTRIX	7/2/90";
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


/*
 * dcl_xar.c -- code to extract modules from an archive
 *
 * Description:
 *	This code is based on Berkeley 4.3 ar.c extract routine
 */

/* SCCS history beginning
 * ***************************************************************
 *                -- Revision History --
 * ***************************************************************
 * 
 * 1.1  30/03/88 -- thoms
 * date and time created 88/03/30 18:07:06 by thoms
 * 
 * ***************************************************************
 * 
 * 1.2  25/04/88 -- thoms
 * Minor fixes
 * 
 * 
 * ***************************************************************
 *
 * 1.3  15/07/88 -- thoms
 * Added copyright notice, modification history, improved comments
 *
 * SCCS history end
 */


#include <sys/param.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <stdio.h>
#include <ar.h>
#include "filter.h"
#include "connection.h"
#include "dcl.h"

static struct	stat	stbuf;
static struct	ar_hdr	arbuf;
static struct	lar_hdr {
	char	lar_name[16];
	long	lar_date;
	u_short	lar_uid;
	u_short	lar_gid;
	u_short	lar_mode;
	long	lar_size;
} larbuf;

char	buf[MAXBSIZE];
static char	*file;
static char	name[16];

#define	SKIP	1
#define	IODD	2
#define	OODD	4
#define	HEAD	8

/*
 * getdir -- get the directory entry info for an archive member
 */
static int
getdir(af)
int af;			/* archive file descriptor */
{
	register char *cp;
	register i;

	i = read(af, (char *)&arbuf, sizeof arbuf);
	if(i != sizeof arbuf) {
		return(1);
	}
	if (strncmp(arbuf.ar_fmag, ARFMAG, sizeof(arbuf.ar_fmag))) {
		fprintf(stderr, "dcl_xar: malformed archive (at %ld)\n", lseek(af, 0L, 1));
		return(1);
	}
	cp = arbuf.ar_name + sizeof(arbuf.ar_name);
	while (*--cp==' ')
		;
	*++cp = '\0';
	strncpy(name, arbuf.ar_name, sizeof(arbuf.ar_name));
	file = name;
	strncpy(larbuf.lar_name, name, sizeof(larbuf.lar_name));
	sscanf(arbuf.ar_date, "%ld", &larbuf.lar_date);
	sscanf(arbuf.ar_uid, "%hd", &larbuf.lar_uid);
	sscanf(arbuf.ar_gid, "%hd", &larbuf.lar_gid);
	sscanf(arbuf.ar_mode, "%ho", &larbuf.lar_mode);
	sscanf(arbuf.ar_size, "%ld", &larbuf.lar_size);
	return(0);
}

/*
 * trim -- chop an archive member file name down to size
 */
static char *
trim(s)
char *s;
{
	register char *p1, *p2;

	/* Strip trailing slashes */
	for(p1 = s; *p1; p1++)
		;
	while(p1 > s) {
		if(*--p1 != '/')
			break;
		*p1 = 0;
	}

	/* Find last component of path; do not zap the path */
	p2 = s;
	for(p1 = s; *p1; p1++)
		if(*p1 == '/')
			p2 = p1+1;

	/*
	 * Truncate name if too long, only if we are doing an 'add'
	 * type operation. We only allow 15 cause rest of ar
	 * isn't smart enough to deal with non-null terminated
	 * names.  Need an exit status convention...
	 * Need yet another new archive format...
	 */
#define truncate 1		/* Yes we do, always */
	if (truncate && strlen(p2) > sizeof(arbuf.ar_name) - 1) {
		*(p2 + sizeof(arbuf.ar_name) - 1) = '\0';
	}
	return(p2);
}


/*
 * match -- check whether trimmed name matches a member file
 */
static int
match(name)
char *name;
{
	if(strcmp(trim(name), file) == 0) {
		return(1);
	}
	return(0);
}

/*
 * copyfil -- copy or skip over next file according to flag argument
 *
 * size given in arbuf
 */
static int
copyfil(fi, fo, flag)
int fi, fo, flag;	
{
	register i, o;
	int pe;

	if(flag & HEAD) {
		for (i=sizeof(arbuf.ar_name)-1; i>=0; i--) {
			if (arbuf.ar_name[i]==' ')
				continue;
			else if (arbuf.ar_name[i]=='\0')
				arbuf.ar_name[i] = ' ';
			else
				break;
		}
		if (write(fo, (char *)&arbuf, sizeof arbuf) != sizeof arbuf) {
			perror("ar write error");
			return 1;
		}
	}
	pe = 0;
	while(larbuf.lar_size > 0) {
		i = o = MAXBSIZE;
		if(larbuf.lar_size < i) {
			i = o = larbuf.lar_size;
			if(i&1) {
				buf[i] = '\n';
				if(flag & IODD)
					i++;
				if(flag & OODD)
					o++;
			}
		}
		if(read(fi, buf, i) != i)
			pe++;
		if((flag & SKIP) == 0)
			if (write(fo, buf, o) != o) {
				perror("ar write error");
				return 1;
			}
		larbuf.lar_size -= MAXBSIZE;
	}
	if(pe) {
		fprintf(stderr, "dcl_xar: phase error on %s\n", file);
		return 1;
	}
	return 0;
}

/*
 * ar_rewind -- seek to beginning of archive
 */
static void
ar_rewind(ar_fd)
int ar_fd;
{
	lseek(ar_fd, (long)SARMAG, 0);
}

/*
 * ar_x -- extract the named module and write it to file
 */
int
ar_x(ar_fd, out_fd, module)
int ar_fd, out_fd;
char *module;
{
	int found=0;

	ar_rewind(ar_fd);

	while(!getdir(ar_fd)) {
		if(match(module)) {
			found++;
			if (copyfil(ar_fd, out_fd, IODD)) {
				return 0;
			}
			break;
		}
		if (copyfil(ar_fd, -1, IODD+SKIP)) {
			return 0;
		}
	}
	return found;
}

/*
 * ar_get -- return open file descriptor on verified archive file
 */
int
ar_get(arnam)
char *arnam;
{
	char mbuf[SARMAG];
	int af;

	af = open(arnam, 0);
	if(af < 0
	   || read(af, mbuf, SARMAG) != SARMAG
	   || strncmp(mbuf, ARMAG, SARMAG))
	{
		fprintf(stderr, "dcl_xar: %s not in archive format\n", arnam);
		return(af);
	}
	return(af);
}
