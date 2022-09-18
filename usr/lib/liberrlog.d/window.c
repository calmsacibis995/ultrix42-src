#ifndef lint
static char	*sccsid = "@(#)window.c	1.5	(ULTRIX)	4/29/86";
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
*
* File Name: window.c
*
* Source file description: Provide a method for user processes to 
*	tap into the error logging stream before this data is written
*	to disk.  (Similar to VMS mailbox).  Currently 10 windows
*	are allowed (see elcsd code).  The user can window data
*	conviently through an option to uerf; uerf will then be
*	invoking these routines.
*
* Functions:
*	open_wndw
*	close_wndw
*	alrmrtn
*
* Compile: See Makefile
*
* Modification history: 
*	Apr 22 1986 - bjg
*		Add read_wndw rtn; change exits and wndwclean call in 
*			open_wndw rtn.
*	Apr 7 1986  - bjg
*		Change "window.h" to <elwindow.h>
*
*/

#include <sys/types.h>
#include <sys/file.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/un.h>
#include <errno.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>

#include <elcsd.h> 
#include <signal.h>
#include <elwindow.h>

#define SUCCESS 0
#define ERROR -1
char buf[4];
char hostname[32];
char wsckt[] = "elcswndwsckt";

int alrmflg = 0;
int elcsd_down = 0;

/*
*
*
* Function: open_wndw(optr, sptr)
*
* Function description: Open a window to the elcsd process, to tap into
*	the error log data stream.  
*
* Arguments:
*	optr	ptr to options structure (see elwindow.h); specifies specific
*			packets which the user process wants to window.
*	sptr	ptr to socket-related structure(see elwindow.h);this structure
*			will be filled in by this routine.
*
* Return value: 
*	s	socket descriptor, to be used by the calling process,
*			to read or recv window data off of.
*	ERROR	socket could not be established.
*
* Side effects: Note that the structure pointed to by sptr is filled
*		with an index (used by elcsd) and the tolen and to address
*		used for the socket.
*
*/

open_wndw(optr, sptr)
struct options *optr;
struct s_params *sptr;
{
	int s, cc, fromslen;
	int indx;
	struct sockaddr_un sun, froms;
	struct wndmsg wndinfo; /* defined in elcsd.h */
	int alrmrtn();
	long pid;
	char full_name[30];
	char tmp[10];
	int rval = ERROR;
	int rcc;

	if (gethostname(hostname,sizeof(hostname)) < 0) {
		perror("open_wndw: gethostname");
		return(ERROR);
	}
	/* set up window packet */

	wndinfo.wndpkt.msgtyp = ELOPEN_WNDW;
	wndinfo.wndpkt.class = optr->class;
	wndinfo.wndpkt.type = optr->type;
	wndinfo.wndpkt.ctldevtyp = optr->ctldevtyp;
	wndinfo.wndpkt.num = optr->num;
	wndinfo.wndpkt.unitnum = optr->unitnum;

	wndinfo.sfalen = sizeof(wndinfo.sfa);
	wndinfo.sfa.sun_family = AF_UNIX;

	pid = getpid();
	(void)itoa(pid, tmp);
	(void)strcpy(full_name, "/tmp/");
	(void)strcat(full_name, wsckt);
	(void)strcat(full_name, tmp);
	(void)bcopy(full_name, (char *)wndinfo.sfa.sun_path, 30);
	(void)strcpy(sptr->scktname,full_name);

	/* open socket to go to elcsd */
	s = socket(AF_UNIX, SOCK_DGRAM, 0);
	if (s < 0) {
		perror("open_wndw: socket");
		return(ERROR);
	}
	sun.sun_family = AF_UNIX;
	(void)bcopy(full_name, (char *)sun.sun_path, 30);
	if (bind(s, (char *)&sun, sizeof(sun)) < 0) {
		perror("open_wndw: bind");
		return(ERROR);
	}

	/* set up to_ address to elcsd */
	sptr->tos.sun_family = AF_UNIX;
	bcopy(elcsckt, (char *)sptr->tos.sun_path, sizeof(elcsckt));
	sptr->toslen = sizeof(sptr->tos);

	cc = sendto(s,(char *)&wndinfo,sizeof(wndinfo),0,(char *)&sptr->tos,sptr->toslen);

	if (cc < 0) { 
		perror("open_wndw: sendto");
		(void)unlink(full_name);
		return(ERROR);
	}

	/* wait for reply */
	(void)signal(SIGALRM, alrmrtn);
	(void)alarm(30);
	
	fromslen = sizeof(froms);
	rcc = recvfrom(s,buf,sizeof(buf),0,(char *)&froms,&fromslen);
	if (alrmflg == 1) {
		if (rcc < 0) {
			perror("open_wndw: Failure due to timeout on ack");
			wndclean(full_name);
			return(ERROR);
		}
	}

	if (rcc == 4) {
		indx = (int)buf[0]; /* NOTE: CHANGE TO INTEGER */
		if (indx == -1) {
		    rval = 0;
		}
		else {
		    sptr->indx = indx;
		    rval = s;
		}
	}
	else {
		rval = ERROR;
	}
	return(rval);
}
/*
*
*
* Function: read_wndw(s, buf, bufsz)
*
* Function description: read data from the window to the elcsd process.
*	
* Arguments:
*	s	socket descriptor, returned from open_wndw
*	buf	ptr to buffer to store data
*	bufsz	sizeof buffer
*	
* Return value: 
*		amount of data read from socket or a -1 if an 
*			error occurred, or null (0) if shutdown. 
*
* Side effects: note that the read_wndw routine checks for a shutdown mesg
*	from the elcsd process, and if received, will return NULL (-1) to the 
*	caller indicating "EOF"
*
*/
read_wndw(s, buf, bufsz)
int s;
char *buf;
int bufsz;
{
	int rcc;

	rcc = recvfrom(s,buf,bufsz,0,(char *)0, 0);
	if  ((rcc == 4) &&  ((int)*buf == ELSHTDWN_WNDW)) {
		elcsd_down = 1;
		rcc = 0;
	}
	return(rcc);
}
wndclean(full_name)
char *full_name;
{
	while (alrmflg == 0) {  /* wait for alarm */
	}
	(void) unlink(full_name);  /* unlink filename, but leave socket open*/
}

itoa(n, s)
char s[];
int n;
{
	int i, sign;
	if ((sign = n) < 0) 
		n = -n;
	i = 0;
	do {
		s[i++] = n % 10 + '0';
	} while ((n /= 10) > 0);
	if (sign < 0)
		s[i++] = '-';
	s[i] = '\0';
	reverse(s);
}
reverse(s)
char s[];
{
	int c, i, j;
	for (i=0, j=strlen(s)-1; i<j; i++, j--) {
		c = s[i];
		s[i] = s[j];
		s[j] = c;
	}
}

/*
*
*
* Function: alrmrtn
*
* Function description: Set flag indicating that timeout has occurred,
*		when awaiting for reply from elcsd
*
* Arguments: None
*
* Return value: None
*
* Side effects: None
*
*/

alrmrtn() 
{
	alrmflg = 1;
}

/*
*
*
* Function: close_wndw(sd, sptr)
*
* Function description: Transmit datagram to elcsd, indicating that
*	the previously opened window should now be closed.
*
* Arguments:
*	sd	socket descriptor, returned on previous open_wndw call
*	sptr	pointer to socket-related structure, filled in on previous
*			open_wndw call.
*
* Return value: ERROR or SUCCESS
*
* Side effects: None
*
*/

close_wndw(sd,sptr)
int sd;
struct s_params *sptr;
{
	struct wndpkt wpkt;
	int cc;
	int rval = SUCCESS;

	wpkt.msgtyp = ELCLOSE_WNDW;
	wpkt.class = sptr->indx;

	if (elcsd_down == 0) {
	    if (sd > 0) {
	    	cc = sendto(sd,(char *)&wpkt,sizeof(wpkt),0,(char *)&sptr->tos,sptr->toslen);
	    	if (cc < 0) {
			perror("close_wndw: Can't send message to elcsd");
			rval = ERROR;
	    	}

	    }
	    else {
			rval = ERROR;
 	    }
	}
	/* elcsd currently sends no reply */
	(void)close(sd);
	(void)unlink((char *)sptr->scktname);
	return(rval);
}

