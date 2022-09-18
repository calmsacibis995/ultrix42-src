#ifndef lint
static char *sccsid = "@(#)strip.c	4.1	ULTRIX	7/17/90";
#endif
/************************************************************************
 *									*
 *			Copyright (c) 1989 by				*
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
 * Modification History:
 *
 * 10/16/89 - D. Long
 *	Don't strip if relocation entries present as per XPG-3.
 */

#include <a.out.h>
#include <signal.h>
#include <stdio.h>
#include <sys/file.h>

struct	exec head;
int	status;
int	pagesize;

main(argc, argv)
	char *argv[];
{
	register i;

	pagesize = getpagesize();
	signal(SIGHUP, SIG_IGN);
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	for (i = 1; i < argc; i++) {
		strip(argv[i]);
		if (status > 1)
			break;
	}
	exit(status);
}

strip(name)
	char *name;
{
	register f;
	long size;

	f = open(name, O_RDWR);
	if (f < 0) {
		fprintf(stderr, "strip: "); perror(name);
		status = 1;
		goto out;
	}
	if (read(f, (char *)&head, sizeof (head)) < 0 || N_BADMAG(head)) {
		printf("strip: %s not in a.out format\n", name);
		status = 1;
		goto out;
	}
	if ((head.a_syms == 0) && (head.a_trsize == 0) && (head.a_drsize ==0)) {
		printf("strip: %s already stripped\n", name);
		goto out;
	}
	if (head.a_syms && (head.a_trsize || head.a_drsize)) {
		printf("strip: %s contains relocation entries, not stripped\n", name);
		status = 1;
		goto out;
	}
	size = (long)head.a_text + head.a_data;
	head.a_syms = head.a_trsize = head.a_drsize = 0;
	if (head.a_magic == ZMAGIC)
		size += pagesize - sizeof (head);
	if (ftruncate(f, size + sizeof (head)) < 0) {
		(void) fprintf(stderr, "strip: "); perror(name);
		status = 1;
		goto out;
	}
	(void) lseek(f, (long)0, L_SET);
	(void) write(f, (char *)&head, sizeof (head));
out:
	close(f);
}
