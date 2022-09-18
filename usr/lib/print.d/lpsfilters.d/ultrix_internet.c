#ifndef lint
static char sccsid[] = "@(#)ultrix_internet.c	4.1      LPS_ULT_IP 	11/15/90";
#endif
/*
 * ultrix_internet.c
 *
 * Written by:  Mike Augeri
 * Date:  12-APR-1990
 *
 * History:
 *   13-APR-1990  mva  Change module name from net_ultrix_x.c to ultrix_decnet.c
 *   11-JUL-1990  mva  Remove include of paprelay.h.
 *
 * This module contains all the Internet and ULTRIX operating system dependent
 * functions required for the lpcomm.c module.
 *
 */
#include <sys/file.h>
#include <sys/types.h>		/* system data types */
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <stdio.h>		/* standard i/o definitions */
#include "lps.h"
 
/*
 *		open_channel
 *
 * This routine is a copy of the original lpcomm AssignDirect function.
 *
 * This routine is called to establish a connection to the printserver.
 * 
 * Inputs:		nodename -- string nodename of printerserver
 * Outputs:		none
 * Returns:		file descriptor (fd) of channel opened on printserver
 * Actions:		allocates the channel, opens a connection to
 *			printserver, and binds that channel to the connection.
 *
 */
open_channel(nodename)
char *nodename;
{
    int  AFD;			/* FD to connect to printer server */
    int connected;		/* true when we have made a connection */
    int failureCount;		/* number of times we have failed to connect */
    int haddr;
    struct hostent *hp;
    struct servent *sp;
    struct sockaddr_in sin;
    char hostString[256];
    char serviceName[128];

		/* get the host address and port number */

    strcpy(serviceName,"print-srv");
    sscanf(nodename,"%[^/]/%[^/]",hostString, serviceName);

    haddr = inet_addr(hostString);
    if (haddr != -1) {
	sin.sin_addr.s_addr = haddr;
    } else {    
	hp = gethostbyname(hostString);
	if (hp == (struct hostent *) NULL) {
	    fprintf(stderr,"%s	%s: Unknown host %s\n",
		progName, LPStime(), hostString);
	    exit(1);
	}
	sin.sin_addr.s_addr = (haddr = *(int *) hp->h_addr);
    }
    sp = getservbyname(serviceName, "tcp");
    if (sp == (struct servent *) NULL) {
        fprintf(stderr,"%s	%s: Unknown service %s\n", 
	    progName, LPStime(), serviceName);
	exit(1);
    }

    sin.sin_family = AF_INET;
    sin.sin_port = sp->s_port;

    connected = FALSE;
    failureCount = 0;

    while (!connected) {

	    	/* get a socket */
	AFD = socket(AF_INET, SOCK_STREAM, 0);
	if (AFD < 0) {
	    sprintf(errbuf,"%s	%s: cannot allocate socket",progName, LPStime());
	    perror(errbuf);
	    exit(1);
	}
#ifdef DEBUG
	if (debug > 1) fprintf(stderr,"%s	%s: connecting to socket %d on  %s (%s)\n",
	    progName, LPStime(), ntohs(sp->s_port), inet_ntoa(sin.sin_addr),hostString);
			       
#endif DEBUG
    
/* keep trying forever to connect to server. Back off retry interval  */
	if (connect(AFD, (caddr_t) &sin, sizeof(sin)) < 0) {
	    sprintf(errbuf,"%s	%s: unable to connect to '%s'",
	        progName,LPStime(), hostString);
	    perror(errbuf);
	    close(AFD);
	    if (failureCount > 0) {
	        sleep(failureCount < 8 ? (1<<failureCount) : 360);
	    }
	    failureCount++;
	    continue;
	}
	connected = TRUE;
    }
#ifdef DEBUG
	if (debug > 1) fprintf(stderr,"%s	%s: connected.\n",
	    progName, LPStime());
			       
#endif DEBUG
    return(AFD);
}
