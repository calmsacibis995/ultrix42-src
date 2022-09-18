static char *sccsid = "@(#)lattelnet.c	4.1	(DECnet-ULTRIX)	7/2/90";

/*
 * Program lattelnet.c
 *
 * Copyright (C) 1988, 1989 by
 * Digital Equipment Corporation, Maynard, Mass.
 *
 * This software is furnished under a license and may be used and copied
 * only  in  accordance  with  the  terms  of such  license and with the
 * inclusion of the above copyright notice. This software or  any  other
 * copies thereof may not be provided or otherwise made available to any
 * other person. No title to and ownership of  the  software  is  hereby
 * transferred.
 *
 * The information in this software is subject to change without  notice
 * and  should  not be  construed  as  a commitment by Digital Equipment
 * Corporation.
 *
 * Digital assumes no responsibility for the use or  reliability  of its
 * software on equipment which is not supplied by Digital.
 *
 *
 * MODULE DESCRIPTION:
 *
 * LAT/telnet gateway
 * Common data storage
 *
 * Another fine product by
 * Networks & Communications Software Engineering
 *
 * IDENT HISTORY:
 *
 * 1.00 22-Apr-88
 *
 *	11-Oct-1989	Matt Thomas
 *	Add perror("/usr/ucb/telnet") for errors on execl.
 *	Add trailing to execl call list.
 *	Change '||' to '|' for allflags.
 *	Restore default handling for SIGHUP.
 */

/*
 *	l a t t e l n e t
 *
 * This is a sample program for the Ultrix LAT to TELNET gateway.  
 * It accepts connect request from LAT terminal and exec's telnet.
 * More extensive tty setting up may be needed (such as for 
 * the parameters defined in gettytap & termcap).  See getty(8).
 *
 * To compile: cc -o lattelnet lattelnet.c
 * 
 * See 'Guide to Ethernet Comm Server' for LAT service set up.
 *
 * To access TELNET service from LAT terminal:
 *	CONNECT telnet [NODE hostname [DEST telnet_hostname]]
 */

#include <sys/ltatty.h>
#include <sys/ioctl.h>
#include <sgtty.h>
#include <signal.h>
#include <ctype.h>
#include <sys/file.h>
#include <errno.h>
#include <ttyent.h>
#include <stdio.h>

struct	sgttyb tmode = {
	0, 0, CERASE, CKILL, 0
};

char	*rindex();
char	hostname[32];
char	dev[] = "/dev/";
char	ttyn[32];
char	*tty;
struct  ltattyi ltainfo;


main(argc, argv)
	char *argv[];
{
	long allflags;
	int flags;
	int latfd; 
	char *np;


	signal(SIGINT, SIG_IGN);
	signal(SIGHUP, SIG_DFL);
	gethostname(hostname, sizeof(hostname));
		
	strcpy(ttyn, dev);
	strncat(ttyn, argv[2], sizeof(ttyn)-sizeof(dev));
	chown(ttyn, 0, 0);
	tty = rindex(ttyn, '/');
	if (tty == NULL)
		tty = ttyn;
	else
		tty++;
	
	chmod(ttyn, 0622);
	flags = O_RDWR;
			
	/* 
	 * open LAT line 
	 */
	latfd = open(ttyn, flags);		

	/* 
	 * get DESTINATION field 
	 */ 
	ioctl(latfd, LIOCTTYI, &ltainfo);		

	dup(0);
	dup(1);

	/* 
	 * set tty flags & mode
         */
	allflags = LCRTERA|LCRTBS|LPRTERA;
	ioctl(0, TIOCLSET, &allflags);
	tmode.sg_flags = CRMOD|ECHO;
	ioctl(0, TIOCSETP, &tmode);

        if (ltainfo.lta_dest_port[0] != 0) {
        	printf("\nLAT to TELNET gateway on %s connecting to %s\n", 
			hostname,ltainfo.lta_dest_port);
                for (np = ltainfo.lta_dest_port; *np; np++) {
	        	if (isupper(*np))
		    		*np = tolower(*np);
		}
		execl("/usr/ucb/telnet","telnet",ltainfo.lta_dest_port,0);
	} else {
                printf("\nLAT to TELNET gateway on %s\n", hostname);
		execl("/usr/ucb/telnet","telnet",0);
	}
	perror("/usr/ucb/telnet");
}
