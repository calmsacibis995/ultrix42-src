#ifndef lint
static char *sccsid = "@(#)pfconfig.c	4.3	(ULTRIX)	2/26/91";
#endif lint

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
 * pfconfig.c
 *
 * Configure system-wide packet filter settings
 *
 * Usage:
 *	pfconfig [-/+p[romisc]] [-b[acklog] nnn] [-/+c[opyall]] [-a[ll]]
 *			[devicename ...]
 *
 * HISTORY:
 *	22 May 1990	Jeffrey Mogul	DECWRL
 *		Added -/+c[opyall]
 *
 *	24 July 1989	Jeffrey Mogul	DECWRL
 *		- Created.
 */

#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <net/if.h>
#include <net/pfilt.h>
#include <stdio.h>

int promisc = -1;
int copyall = -1;
int backlog = 0;
int printit = 1;
int doall = 0;
char *default_ifname = "pf0";

main(argc, argv)
int argc;
char **argv;
{
	while (argc > 1) {
	    if (argv[1][0] == '-') {
		switch (argv[1][1]) {
		    case 'p':
			promisc = 0;
			printit = 0;
			break;

		    case 'c':
			copyall = 0;
			printit = 0;
			break;

		    case 'a':
			doall++;
			break;

		    case 'b':
			if (argc < 3) {
			    fprintf(stderr,
			    	"%s: %s must be followed by a number\n",
					argv[0], argv[1]);
			    exit(1);
			}
			backlog = atoi(argv[2]);
			argc--;
			argv++;
			printit = 0;
			break;

		    default:
			fprintf(stderr, "%s: %s not a valid option\n",
				argv[0], argv[1]);
			exit(1);
		}
	    }
	    else if (argv[1][0] == '+') {
		switch (argv[1][1]) {
		    case 'c':
			copyall = 1;
			printit = 0;
			break;

		    case 'p':
			promisc = 1;
			printit = 0;
			break;

		    default:
			fprintf(stderr, "%s: %s not a valid option\n",
				argv[0], argv[1]);
			exit(1);
		}
	    }
	    else
		DoInterface(argv[1]);
	    argc--;
	    argv++;
	}

	if (doall)
	    DoAll();
}

DoInterface(ifname)
char *ifname;
{
	int fid;
	struct ifreq ifr;

	fid = GetPFfid(ifname);	
	if (fid < 0 ) {
	    perror(ifname);
	    exit(1);
	}
	strcpy(ifr.ifr_name, ifname);
	if (ioctl(fid, EIOCSETIF, &ifr) < 0) {
	    perror(ifname);
	    exit(1);
	}

	DoFid(fid);

	close(fid);
}
	
DoAll()
{
	int fid;
	struct ifreq ifr;
	int unit;
	char tryname[64];


	for (unit = 0; unit <= 9; unit++) {
	    sprintf(tryname, "%s%d", ENGENPREFIX, unit);
	    fid = GetPFfid(tryname);
	    if (fid < 0)
		continue;
	    sprintf(ifr.ifr_name, "%s%d", ENGENPREFIX, unit);
	    if (ioctl(fid, EIOCSETIF, &ifr) < 0) {
		continue;
	    }
	    DoFid(fid);
	}

	close(fid);
}

DoFid(fid)
int fid;
{
	if (promisc >= 0) {
	    if (ioctl(fid, EIOCALLOWPROMISC, &promisc) < 0) {
		perror("EIOCALLOWPROMISC");
		exit(1);
	    }
	}

	if (copyall >= 0) {
	    if (ioctl(fid, EIOCALLOWCOPYALL, &copyall) < 0) {
		perror("EIOCALLOWCOPYALL");
		exit(1);
	    }
	}

	if (backlog > 0) {
	    if (ioctl(fid, EIOCMAXBACKLOG, &backlog) < 0) {
		perror("EIOCMAXBACKLOG");
		exit(1);
	    }
	}

	if (printit) {
	    struct ifreq ifr;
	    int cur_copyall = -1;
	    int cur_promisc = -1;
	    int cur_backlog = -1;

	    if (ioctl(fid, EIOCIFNAME, &ifr) < 0) {
		perror("EIOCIFNAME");
		exit(1);
	    }
	    if (ioctl(fid, EIOCALLOWPROMISC, &cur_promisc) < 0) {
		perror("EIOCALLOWPROMISC");
		exit(1);
	    }
	    if (ioctl(fid, EIOCALLOWCOPYALL, &cur_copyall) < 0) {
		perror("EIOCALLOWCOPYALL");
		exit(1);
	    }
	    if (ioctl(fid, EIOCMAXBACKLOG, &cur_backlog) < 0) {
		perror("EIOCMAXBACKLOG");
		exit(1);
	    }
	    printf("%s: maximum backlog is %d;", ifr.ifr_name, cur_backlog);
	    if (cur_promisc)
		printf(" auto-promiscuous mode is enabled\n");
	    else
		printf(" auto-promiscuous mode is disabled\n");
	    if (cur_copyall)
		printf("\tauto-copyall mode is enabled\n");
	    else
		printf("\tauto-copyall mode is disabled\n");
	}
}

GetPFfid(ifname)
char *ifname;
{
	int fid;

	if ((fid = pfopen(ifname, 0)) < 0) {
	    return(-1);
	}
	return(fid);
}
