#ifndef lint
static char *sccsid = "@(#)startcpu.c	4.1	ULTRIX	7/17/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1984 by				*
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
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/smp_lock.h>
#include <sys/cpudata.h>

extern int errno;
/* 	Modification History
 *
 *	Apr-17-89	gmm
 *	       Changes to conform to the way startcpu() system call handles
 *	       errors. First submission to the sccs pool
 */

main(argc,argv)
int argc;
char *argv[];
{

	register int i;
	int cpunum = -1;
	int ret_val;

	if(--argc > 0) {
		if(argc--)
			cpunum = atoi(argv[1]);
	}

	if (cpunum == -1) {
		for (i=0; i< MAXCPU; i++) {
			if (ret_val=startcpu(i)) {
			  	/*perror("startcpu");
				exit(errno); Since we are attempting to start
				  all POSSIBLE cpus, some of them may fail. We
				  do not want to exit in this case */
			}else
				printf("cpu %d started\n",i);
		}
	} else {
		if (cpunum>=MAXCPU || cpunum<0) {
			printf("bad cpu number %d\n",cpunum);
			exit(1);
		}
		if (ret_val=startcpu(cpunum)){
		  	perror("startcpu");
			exit(errno);
		}else
			printf("cpu %d started\n",cpunum);
	}
	exit(0);
}








