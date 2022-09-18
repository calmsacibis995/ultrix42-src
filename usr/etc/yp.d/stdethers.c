#ifndef lint
static char *sccsid = "@(#)stdethers.c	4.2      ULTRIX  9/17/90";
#endif lint

/****************************************************************
 *                                                              *
 *  Licensed to Digital Equipment Corporation, Maynard, MA      *
 *              Copyright 1985 Sun Microsystems, Inc.           *
 *                      All rights reserved.                    *
 *                                                              *
 ****************************************************************/
/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/* 	08/23/90	terry	commented out ether_addr declaration */

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>

/* The following is now in if_ether.h.  terry 8/23/90 */
/*** SUN HAD THIS IN if_ether.h *****/
/*
 * Ethernet address - 6 octets
 *
 * struct ether_addr {
 *       u_char  ether_addr_octet[6];
 * };
 */

/*
 * Filter to convert addresses in /etc/ethers file to standard form
 */

main(argc, argv)
	int argc;
	char **argv;
{
	char buf[512];
	register char *line = buf;
	char hostname[256];
	register char *host = hostname;
	struct ether_addr e;
	register struct ether_addr *ep = &e;
	FILE *in;

	if (argc > 1) {
		in = fopen(argv[1], "r");
		if (in == NULL) {
			fprintf(stderr, "%s: can't open %s\n", argv[0], argv[1]);
			exit(1);
		}
	} else {
		in = stdin;
	}
	while (fscanf(in, "%[^\n] ", line) == 1) {
		if ((line[0] == '#') || (line[0] == 0))
			continue;
		if (ether_line(line, ep, host) == 0) {
			fprintf(stdout, "%s	%s\n", ether_ntoa(ep), host);
		} else {
			fprintf(stderr, "%s: ignoring line: %s\n", argv[0], line);
		}
	}
	exit(0);
}
