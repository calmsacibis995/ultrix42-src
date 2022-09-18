#ifndef lint
static char *sccsid = "@(#)tcp_conn.c	4.1     ULTRIX  7/2/90";
#endif

#include "lp.h"

/*
 *		T C P _ c o n n
 *
 * Create a TCP connection to the remote printer server.
 *
 * Inputs:		char *rhost;
 * Outputs:		none
 * Returns:		FD open on network printer
 *
 * The string "rhost" is expected to be something like "decwrl", or
 * "decwrl.dec.com", or "decwrl.dec.com/servicename". If the / is present,
 * then the string following it is the name of the service, to be looked
 * up in /etc/services. If no name is provided (the / is absent), then
 * the service "printserver" is assumed.
 *
 */

/* SCCS history beginning
 * ***************************************************************
 *                -- Revision History --
 * ***************************************************************
 * 
 * 4.1  13/09/88 -- gray
 * date and time created 88/09/13 15:39:38 by gray
 * 
 * ***************************************************************
 * 
 * 5.1  06/10/88 -- hodges
 * vNe
 * 
 * ***************************************************************
 * 
 * 5.2  12/10/88 -- root
 * FixingNE
 * 
 * ***************************************************************
 * 
 * 5.3  13/10/88 -- root
 * FixingNE
 * 
 * ***************************************************************
 *
 * 5.4  16/10/88 -- thoms
 * Provided static fatal routine to use log file so that
 * error messages are available.
 *
 * SCCS history end
 */


static void fatal(fmt, arg1, arg2)
char *fmt;
int arg1, arg2;
{
	log(fmt, arg1, arg2);
	exit(1);
}

tcp_conn(rhost)
char *rhost;
{
    struct hostent *hp;
    struct servent *sp;
    struct sockaddr_in sin;
    int s, retries, aval;
    int err;
    char hostString[512];
    char serviceName[512];

    extern int sys_nerr;
    extern char *sys_errlist[];

    /*
     * Get the host address and port number to connect to.
     */
    strcpy(serviceName,"printserver");
    sscanf(rhost,"%[^/]/%[^/]", hostString, serviceName);
    bzero((char *)&sin, sizeof(sin));

    aval = inet_addr(hostString);
    if (aval != -1) {
	sin.sin_addr.s_addr = aval;
    } else {
	hp = gethostbyname(hostString);
	if (hp == (struct hostent *) NULL)
	    fatal("unknown host %s", hostString);
	sin.sin_addr.s_addr = *(int *) hp->h_addr;
    }
    sp = getservbyname(serviceName, "tcp");
    if (sp == (struct servent *) NULL)
        fatal("Service '%s/tcp' not defined. Fix /etc/services.",serviceName);
    sin.sin_family = AF_INET;
    sin.sin_port = sp->s_port;

    if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        if ((errno > 0) && (errno <= sys_nerr))
            fatal("Can't get socket: %s",sys_errlist[errno]);
        else    fatal("Can't get socket, errno=%d",errno);
    }
    retries = 0;
    while (1) {
        if (connect(s, (caddr_t)&sin, sizeof(sin), 0) < 0) {
            err = errno;
            close(s);
	    retries = (retries > 5) ? 6 : retries+1;
	    status("Trying to connect to %s",hostString);
            if (err == ECONNREFUSED) sleep(1<<retries);
        } else
	    return(s);
    }
}
