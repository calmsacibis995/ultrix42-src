#ifndef lint
static char *sccsid = "@(#)elcsd.c	4.2	(ULTRIX)	9/4/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1986,1990 by			*
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
/************************************************************************
 *
 *  File name:
 *	elcsd.c 
 *
 *  Source file description:
 *	This file contains the errlog copy/server code which reads
 *	error data from the kernel via /dev/errlog or from a inet
 *	socket and writes this information to a file on disk. Also
 *	accepts windowing reqeusts on local data and sends this
 *	data to the requesting process. The elcsd reads a config 
 *	file which has the information on setting up the state of
 *	elcsd. Elcsd also accepts control information via a unix
 *	domain socket.
 *
 *  Functions:
 *	main,fwndproc,movelptr,reaper,fillinhdr,sndrem,ackmsg,sndmsg,
 *	wndcnt,wndwshtdwn,wndw,chkwt,msgopen,msgclose,cntlmsg,cntlmsgrply,
 *	elcsterm,elcsclean,elcsinit,setinetskt,setlocal,setremote,setoft,
 *	gofts,goftp,chkfcpy,rchkfcpy,closef,chkrht,setlocpath,wrtdata,
 *	statfile,fsfull,rdcfg,fct,getdmpfil,error,setmarker
 *
 *  Usage:
 *	Invoked by the eli command or /etc/elcsd [-s] [-n] &
 *
 *  Modification history:
 *
 *  23-Aug-90 -- stuarth
 *      Removed redundant setting of rhdr_sid in fillinhdr().
 *      rhdr_sid gets set by ealloc().  And, for vax9000, some
 *      error message types require that rhdr_sid be set to the
 *      sid of the console (SPU).  fillinhdr() was overwriting
 *      the rhdr_sid after vax9000 code was setting it to the SPU's sid.
 *
 *  10-Nov-89 -- cranston
 *      Error log packets from remote systems also now sent to
 *      window-processes. This was done by taking 13 lines of code from the
 *      'rdmask & elfdmask' routine (starting at line #323) and duplicating
 *      them in the 'rdmask & dsimask' routine (starting at line #363).
 * 
 *  26-Sep-88 -- pmk
 *	removed elbuf define - use the one in errlog.h, 
 *	buf now sizeof(struct elbuf),
 *	removed used used variables.
 *
 *  06-Sep-88 -- afd
 *	ported to PMAX (all it needed was an errlog.h that it could swallow).
 * 
 *  8-Jun-88 -- map
 *	Changed signal handlers to void.
 *
 *  8-Mar-88 -- arce
 *	changed elver to move a short ELVER which is now defined in
 *	errlog.h and not in elcsd.h
 *
 * 14-May-97 -- bglover
 *	change fsfull to return positive value; cast fd_bfreen to int in 
 *	fsfull() to check for negative value; clean up sprintfs
 *
 * 12-May-87 -- jaa
 *	changed fsfull() to use new statfs library routine instead of getmnt().
 *	no longer have to setmarker(), etc.
 *
 * 04-Dec-86 -- pmk
 *	Elcsd now forks it's self.
 *
 * 11-Sep-86 -- bglover
 *	Successful startup msgs are no longer printed on the console in 
 *	single user mode (installation request); add routine setmarker()
 *	to save pointer to mount table entry in eloft - used in call to
 *	getmnt in fsfull() - no longer get entire mount table - caused
 *	delays when file systems remotely mounted (nfs)
 *
 * 11-Sep-86 -- pmk
 *	Changed the write to pipe and the read from pipe to correct problem
 *	with windowing the errlog data.
 *
 * 30-Jun-86 -- bjg
 *	Limit number of mesgs printed when fs out of space; missed_cnt
 *
 * 09-Jun-86 -- bjg
 *	Added ioctl call to get system startup time; change fillinhdr rtn
 *	to insert system startup time.
 *
 * 29-Apr-86 -- pmk
 *	Added 4 routines movelptr,wndwshtdwn,cntlmsgrply & fsfull.
 *	Changed the error routine to log to console in single user mode.
 *
 * 27-Mar-86 -- bjg
 *	Added rchkfcpy routine to remotely log file created in single
 *	user mode; changed getdmpfil rtn to look in multi user or
 *	single user logging locations for elbuffer; added new error 
 *	messages (#64 & #65 in elcsd.h)
 *
 * 19-Mar-86 -- pmk
 *	Added code to send startup msg. if logging remotes,
 *	on remote logging send kernel elbuffer to remote and
 *	if prilogflg log local.
 *
 * 05-Mar-86 -- pmk 
 *	Changed open logfile to w+ and added code
 *	to handle new elbuffer data in getdmpfil().
 *
 * 19-Feb-86 -- pmk  Added getdmpfil routine
 *
 * 01-Dec-85 -- pete keilty	elcsd.c created
 *
 ************************************************************************/

#include <sys/param.h>
#include <sys/mount.h>
#include <sys/types.h>
#include <sys/fs.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <sys/lock.h>
#include <netinet/in.h>
#include <errno.h>
#include <syslog.h>
#include <signal.h>
#include <stdio.h>
#include <netdb.h>
#include <sys/errlog.h>
#include <elcsd.h>

void elcsterm();
void elcsclean();
void elcsinit();
void reaper();

extern int errno, sys_nerr;
extern char *sys_errlist[];

#define MISSED_CNT 3

char buf[sizeof(struct elbuf)]; 
char wbuf[8192]; 

char hostname[MAX_NLEN];
struct elparam pidsid;
long savetime;

FILE *logfp = NULL;
int mode = 0;
int port = 0;
int termflg = 0;
int owndflg = 0;
int pathflg = 0;
int wprocflg = 0;
int nologflg = 0;
int sglusrflg = 0;
int logremflg = 0;
int remlogflg = 0;
int prilogflg = 0;
int oftindx = OFTINDX;
int missed_cnt = 0;

int wpid = 0;
int pfd[2];
int elfd;
int sc, ssi, dsi;
int scmask, ssimask, dsimask; 
int toslen, scfromlen;
int dsifromlen, ssifromlen;
struct sockaddr_un scfrom;
struct sockaddr_in tos;
struct sockaddr_in ssifrom;
struct sockaddr_in dsifrom;

/*
 *  Function: main(argc,argv)
 *
 *  Description: Main does initial set up, than calls elcsinit()
 *	which initializes the state of elcsd. Then selects
 *	on the file descriptors waiting for data. When something
 *	arrives call the routine to service it and go back to
 *	select.
 *
 *  Arguments:  int argc
 *		char **argv
 *
 *  Return value: none
 *
 *  Side effects: none
 */
main(argc,argv)
	int argc;
	char **argv;
{
	int ns, ret;
	int rdmask, elfdmask, osigmask;
	int wcc, cc, opid;
	void movelptr();
	char *bufp;

	
	struct timeval timeout;
	timeout.tv_sec = 2;
	timeout.tv_usec = 0;

	(void) signal(SIGINT, SIG_IGN);
	(void) signal(SIGQUIT, SIG_IGN);
	(void) signal(SIGTERM, elcsterm);

	while (--argc > 0 && **++argv == '-') {

		switch (*++*argv) {
		case 's':
			sglusrflg++;
			mode = 1;
			break;
		case 'n':
			nologflg++;
			mode = 2;
			break;
		default:
			fprintf(stderr,"elcsd: Invalid arg %s, valid args are -s or -n\n",*argv);
			exit(0);
		}
	}

	switch (fork()) {
	    case 0: break;
	    case -1: fprintf(stderr,"elcsd: failed to fork %s\n",sys_errlist[errno]);
	    default: exit(0);
	}

	elfd = open("/dev/errlog",O_RDONLY);
	if (elfd < 0) {
	    error(ALERT,elcsmsg[1],"/dev/errlog");
	    exit(0);
	}

	if (ioctl(elfd, (int)ELGETPID, (char *)&opid) < 0) {
	    error(ALERT,elcsmsg[2],(char *)0);
	    exit(0);
	}

	if (opid) {
	    (void)kill(opid, SIGTERM);
	    sleep(5);
	    errno = 0;
	}

	if (!sglusrflg) {
	    logfp = fopen(logpath,"w+");
	    if (logfp == NULL) {
	        error(ALERT,elcsmsg[1],logpath);
	        exit(0);
	    }
	}

	/* startup msgs */
	if (!sglusrflg) {
	    error(NONALERT,elcsmsg[3],elcsmode[mode]);
	}

	pidsid.pid = getpid();
	if (ioctl(elfd, (int)ELSETPID, (char *)&pidsid) < 0) {
	    error(ALERT,elcsmsg[4],(char *)0);
	    elcsclean();
	}

	if (ioctl(elfd, (int)ELGETTIME, (char *)&savetime) < 0) {
	    error(ALERT,elcsmsg[70],(char *)0);
	}

	if (gethostname(hostname,sizeof(hostname)) < 0) {
	    error(ALERT,elcsmsg[5],(char *)0);
	    elcsclean();
	}
	if (*hostname == '\0') {
	    error(ALERT,elcsmsg[23],(char *)0);
	    elcsclean();
	}

	elfdmask = 1<<elfd;

	elcsinit();
	(void) signal(SIGHUP, elcsinit);
	(void) signal(SIGCHLD, reaper);

	for (;;) {
	    rdmask = elfdmask | dsimask | ssimask | scmask;
	    if (select(16, &rdmask,(int *)0,(int *)0,termflg ? &timeout : 0) < 0) {
		if (errno == EINTR) {
		    errno = 0;
		    continue;
		}
		error(NONALERT,elcsmsg[6],(char *)0);
		continue;
	    }
	    osigmask = sigblock(sigmask(SIGTERM)|sigmask(SIGHUP)|sigmask(SIGINT));
	    if (rdmask & elfdmask) {
	        cc = read(elfd,buf,sizeof(buf));
		if (cc > 0) {
		    fillinhdr(cc,buf);
		    if (wprocflg > 0) {
			bufp = buf;
			wcc = cc;
			while (wcc > 0) {
			    ret = write(pfd[1],bufp,(wcc > 4096) ? 4096 : wcc);
			    if (ret < 0) {
			        error(NONALERT,elcsmsg[7],(char *)0);
				break;
			    }
			    wcc -= ret;
			    bufp += ret;
			}
		    }
		    if (remlogflg > 0) {
			sndrem(cc,buf);
		    }
		    else if (nologflg == 0) {
			ret = wrtdata(LOCLOG,buf,cc);
			if (ret < 0) {
			    error(ALERT,elcsmsg[8],(char *)0);
			    elcsclean();
			}
		    }
		    movelptr(cc);
		}
	    }
	    if (rdmask & dsimask) {
		cc = recvfrom(dsi,buf,sizeof(buf),0,(char *)&dsifrom,&dsifromlen);
		if ((cc == 4) && ((int)buf[0] == ELSHUTDOWN)
			  && (remlogflg > 0)) {
		    remlogflg = 0;
    		    error(ALERT,elcsmsg[10],(char *)0);
		    setlocal();
		}
		else if ((cc == 4) && ((int)buf[0] == ELSTARTUP) 
			  && (remlogflg == 0)) {
		    error(ALERT,elcsmsg[63],(char *)0);
		    elcsinit();
		}
		else if (cc > 4 && logremflg > 0) {
		    if (wprocflg > 0) {
			bufp = buf;
			wcc = cc;
			while (wcc > 0) {
			    ret = write(pfd[1],bufp,(wcc > 4096) ? 4096 : wcc);
			    if (ret < 0) {
			        error(NONALERT,elcsmsg[7],(char *)0);
				break;
			    }
			    wcc -= ret;
			    bufp += ret;
			}
		    }
		    ret = wrtdata(LOGREM,buf,cc);
		}
	    }
	    if (rdmask & ssimask) {
		if (logremflg > 0) {
		    ns = accept(ssi, &ssifrom, &ssifromlen);
		    if (ns > 0) {
		        ackmsg(ns);
		        (void)close(ns);
		    }
		    else
			error(NONALERT,elcsmsg[11],(char *)0);
		}
	    }
	    if (rdmask & scmask) {
		cc = recvfrom(sc,buf,sizeof(buf),0,(char *)&scfrom,&scfromlen);
		if (cc > 0)
		    cntlmsg(cc,buf);
	    }
	    if (termflg > 0) {
		if (logremflg > 0)
		   sndmsg(ELSHUTDOWN);
		elcsclean();
	    }
	    (void)sigsetmask(osigmask);

	}
}

/*
 *  Function: fwndproc()
 *
 *  Description: Fork window process
 *
 *  Arguments: none
 *
 *  Return value: none
 *
 *  Side effects: none
 */
fwndproc()
{
	(void)pipe(pfd);

	wpid = fork();
	if (wpid == 0) {
		wndcnt();
		exit(1);
	}
	if (wpid < 0)
	    error(ALERT,elcsmsg[12],(char *)0);
	else
	    wprocflg++;
	return(wpid);
}

/*
 *  Function: movelptr(cc)
 *
 *  Description: move kernel errlog output pointer
 *
 *  Arguments: cc	character count
 *
 *  Return value: none
 *
 *  Side effects: none
 */
void movelptr(cc)
int cc;
{
	int moverrflg = 0;

	while (ioctl(elfd, (int)ELMOVPTR, (char *)&cc) < 0) {
	    moverrflg++;
	    error(ALERT,elcsmsg[9],(char *)0);
	    switch (moverrflg) {
		case 1:
		    if (ioctl(elfd, (int)ELSETPID, (char *)&pidsid) < 0) {
	    		error(ALERT,elcsmsg[4],(char *)0);
	    		elcsclean();
		    }
	    	    error(ALERT,elcsmsg[68],(char *)0);
		    break;
		case 2:
	    	    if (ioctl(elfd, (int)ELREINIT, (char *)0) < 0) {
	        	error(ALERT,elcsmsg[24],(char *)0);
	    		elcsclean();
		    }
	    	    error(ALERT,elcsmsg[58],(char *)0);
		    return;
		default:
	            error(ALERT,elcsmsg[66],(char *)0);
	    	    elcsclean();
	    }
	}
}

/*
 *  Function: reaper()
 *
 *  Description: Clean up after window process exits.
 *
 *  Arguments: none
 *
 *  Return value: none
 *
 *  Side effects: none
 */
void
reaper()
{
	union wait status;

	while (wait3(&status,WNOHANG,(struct rusage *)0) > 0) {
	    wprocflg = 0;
	    wpid = 0;
	    (void)close(pfd[0]);
	    (void)close(pfd[1]);
	}
}

/*
 *  Function: fillinhdr(cc,buffer)
 *
 *  Description: Fill in trailer code, and errlog version in
 *	each record header.  Also, fill in time if needed.
 *
 *  Arguments:  int cc		character count
 *		char *buffer	pointer to data
 *
 *  Return value: none
 *
 *  Side effects: none
 */
fillinhdr(cc,buffer)
int cc;
char *buffer;
{
	char *bufp;
	struct el_rhdr *elrhdp;

	bufp = buffer;
	while (cc > 0) {
	    elrhdp = (struct el_rhdr *)bufp;
	    if (elrhdp->rhdr_time < 3600) {
		elrhdp->rhdr_time = savetime;
	    }
	    elrhdp->rhdr_elver = ELVER;
	    (void)strncpy(elrhdp->rhdr_hname,hostname,12);
	    if (elrhdp->rhdr_reclen > cc) {
		elrhdp->rhdr_reclen = cc;
		elrhdp->rhdr_valid = 0;
	    }
	    bufp += elrhdp->rhdr_reclen - 4;
	    (void)strncpy(bufp,trailer,4);
	    bufp += 4;
	    cc -= elrhdp->rhdr_reclen;
	}
}

/*
 *  Function: sndrem(cc,buffer)
 *
 *  Description: Send data to remote system and log priority
 *	errors local.
 *
 *  Arguments:  int cc		character count
 *		char *buffer	pointer to data
 *
 *  Return value: none
 *
 *  Side effects: none
 */
sndrem(cc,buffer)
int cc;
char *buffer;
{
	int ret;
	char *bufp;
	struct el_rhdr *elrhp;

	bufp = buffer;
	while (cc > 0) {
	    elrhp = (struct el_rhdr *)bufp;
	    ret = sendto(dsi,bufp,(int)elrhp->rhdr_reclen,0,(char *)&tos,toslen);
	    if (ret < 0)
		error(NONALERT,elcsmsg[13],(char *)0);
	    if (prilogflg > 0 && elrhp->rhdr_pri < EL_PRILOW) {
		if (pathflg == 0)
		    pathflg++;
	    	ret = wrtdata(LOCLOG,bufp,(int)elrhp->rhdr_reclen);
	    	if (ret < 0)
	            error(ALERT,elcsmsg[14],(char *)0);
	    }
	    cc -= elrhp->rhdr_reclen;
	    bufp += elrhp->rhdr_reclen;
	}
}

/*
 *  Function: ackmsg(ns)
 *
 *  Description: Receive start remote log msg and acknowledge
 *
 *  Arguments: int ns		socket descriptor
 *
 *  Return value: none
 *
 *  Side effects: none
 */
ackmsg(ns)
int ns;
{
	int cc;
	int msg;
	struct hostent *hp;
	struct elrht *rhtp;

	cc = recv(ns,(char *)&msg,4,0);
	if (cc == 4 && msg == ELSTRTRLOG) {
	    hp = gethostbyaddr((char *)&ssifrom.sin_addr,4,AF_INET);
	    if (hp != 0) {
		rhtp = chkrht(hp->h_name);
		if (rhtp != 0) {
		    msg = ELGOAHEAD;
		    cc = send(ns,(char *)&msg,4,0);
		    if (cc < 0) 
	    		error(NONALERT,elcsmsg[15],(char *)0);
		}
		else {
		    msg = ELNOGO;
		    cc = send(ns,(char *)&msg,4,0);
		    if (cc < 0) 
	    		error(NONALERT,elcsmsg[15],(char *)0);
		}
	    }
	    else if (errno == 0)
	    	error(NONALERT,elcsmsg[60],(char *)0);
	    else
	    	error(NONALERT,elcsmsg[16],(char *)0);
	}
	else if (cc < 0)
	    error(NONALERT,elcsmsg[17],(char *)0);
}

/*
 *  Function: sndmsg(msg)
 *
 *  Description: Send startup/shutdown message to remotes systems
 *
 *  Arguments: none
 *
 *  Return value: none
 *
 *  Side effects: none
 */
sndmsg(msg)
int msg;
{
	int ret;
	struct hostent *hp;
	struct elrht *rhtp;

	for (rhtp = elrht; rhtp < &elrht[RHTSIZE]; rhtp++) {
	    if (*rhtp->hname == '\0')
		continue;

            hp = gethostbyname(rhtp->hname);
	    if (hp == 0)
	        error(NONALERT,elcsmsg[18],rhtp->hname);
	    else {
	        bzero((char *)&tos,sizeof(tos));
	        bcopy(hp->h_addr,(char *)&tos.sin_addr,hp->h_length);
	        tos.sin_family = hp->h_addrtype;
	        tos.sin_port = port;
	        toslen = sizeof(tos);
		ret = sendto(dsi,(char *)&msg,4,0,(char *)&tos,toslen);
		if (ret < 0)
	            error(NONALERT,elcsmsg[13],rhtp->hname);
	    }
	}
}

/*
 *  Function: wndcnt()
 *
 *  Description: Window control routine receives data from elcs process
 *	through a pipe and processes the msg or data.
 *
 *  Arguments: none
 *
 *  Return value: none
 *
 *  Side effects: none
 */
wndcnt()
{
	int cc, rdmask;
	int  pmask, osigmask;
	char *bufp;
	struct wndpkt *wpp;
	struct timeval timeout;
	int savcc = 0;

	timeout.tv_sec = 0;
	timeout.tv_usec = 0;

	(void) signal(SIGTERM, elcsterm);
	(void)sigsetmask(~(sigmask(SIGTERM)));

	pmask = 1 << pfd[0];
	for (;;) {
	    rdmask = pmask;
	    if (select(16, &rdmask,(int *)0,(int *)0,termflg ? &timeout : 0) < 0) {
		if (errno == EINTR) {
		    errno = 0;
		    continue;
		}
		error(NONALERT,elcsmsg[6],"window elcsd daemon");
		continue;
	    }
	    osigmask = sigblock(sigmask(SIGTERM));
	    if (rdmask & pmask) {
	        bufp = wbuf;
		bufp += savcc;
	        cc = read(pfd[0],bufp,(savcc == 0) ? 4096 : (4096 - savcc));
	        if (cc < 0) {
	            error(NONALERT,elcsmsg[20],(char *)0);
		    continue;
	        }
		cc += savcc;
		savcc = 0;
	        bufp = wbuf;
	        while (cc > 0) {
	            wpp = (struct wndpkt *)bufp;
	            if ((wpp->msgtyp <= ELCLOSE_WNDW) && (wpp->msgtyp > 0)) {
		        if (wpp->msgtyp == ELOPEN_WNDW) {
		            msgopen(bufp);
		        }
		        else {
		            msgclose(bufp);
		        }
		        cc -= sizeof(struct wndmsg);
		        bufp += sizeof(struct wndmsg);
	            }
	            else if (wpp->msgtyp >= EL_MISCSIZE) {
			if (wpp->msgtyp > cc) {
			    savcc = cc;
			    bcopy(bufp,wbuf,cc);
			    cc = 0;
			}
			else {
		            wndw(wpp->msgtyp,bufp);
		            cc -= wpp->msgtyp;
		            bufp += wpp->msgtyp;
			}
		    }
		    else {
			cc = 0;
		    }
	        }
	        if (owndflg == 0)
	            exit(0);
	    }
	    if (termflg > 0) {
		wndwshtdwn();
	        exit(0);
	    }
	    (void)sigsetmask(osigmask);
	}
}

/*
 *  Function: wndwshtdwn()
 *
 *  Description: Send shutdown message to all windowing processes
 *
 *  Arguments: none 
 *
 *  Return value: none
 *
 *  Side effects: none
 */
wndwshtdwn()
{
	int scc, msg;
	struct wndwt *wtp;

	msg = ELSHTDWN_WNDW;
	for (wtp = wndwt; wtp < &wndwt[WTSIZE]; wtp++) {
	    if (wtp->wsalen == 0)
		continue;
	    scc = sendto(sc,(char *)&msg,sizeof(msg),0,(char *)&wtp->wsa,wtp->wsalen);
	    if (scc < 0)
		error(NONALERT,elcsmsg[21],"window elcsd daemon");
	}
}

/*
 *  Function: wndw(cc,bufp)
 *
 *  Description: Send data to requesting process if it matches
 *	requested data.
 *
 *  Arguments:  int cc		character count
 *		char *bufp	pointer to data
 *
 *  Return value: none
 *
 *  Side effects: none
 */
wndw(cc,bufp)
int cc;
char *bufp;
{
	int scc;
	struct wndwt *wtp;

	for (wtp = wndwt; wtp < &wndwt[WTSIZE]; wtp++) {
	    if (wtp->wsalen == 0)
		continue;
	    if (chkwt(wtp,bufp) > 0) {
		scc = sendto(sc,bufp,cc,0,(char *)&wtp->wsa,wtp->wsalen);
		if (scc < 0)
		    error(NONALERT,elcsmsg[21],"window elcsd daemon");
	    }
	}
}

/*
 *  Function: chkwt(wtp,bufp)
 *
 *  Description: Check data against requested data in window table slot.
 *
 *  Arguments:  struct wndwt *wtp	pointer to window table entry
 *		char *bufp		pointer to data
 *
 *  Return value: ret = 0 no match
 *		  ret = 1 match
 *
 *  Side effects: none
 */
int chkwt(wtp,bufp)
struct wndwt *wtp;
char *bufp;
{
	int ret = 0;
	struct el_rec *recp;
	struct el_sub_id *subp;

	recp = (struct el_rec *)bufp;
	subp = &recp->elsubid;

	if (subp->subid_class == wtp->class || wtp->class == ELWNDALL) {
	    if (subp->subid_type == wtp->type || wtp->type == ELWNDALL) {
		if (subp->subid_ctldevtyp == wtp->ctldevtyp ||
				wtp->ctldevtyp == ELWNDALL) {
	    	    if (subp->subid_num == wtp->num || wtp->num == ELWNDALL) {
	    		if (subp->subid_unitnum == wtp->unitnum ||
					wtp->unitnum == ELWNDALL)
		    	    ret++;
	    	    }
	        }
	    }
	}
	return(ret);
}

/*
 *  Function: msgopen(bufp)
 *
 *  Description: Open entry in window table and send message back to 
 *	process requesting windowing of data.
 *
 *  Arguments: char *bufp	pointer to data
 *
 *  Return value: none
 *
 *  Side effects: none
 */
msgopen(bufp)
char *bufp;
{
	int i = 0;
	int scc;
	struct wndwt *wtp;
	struct wndpkt *wp;
	struct wndmsg *wmp;

	wp = (struct wndpkt *)bufp;
	wmp = (struct wndmsg *)bufp;
	for (wtp = wndwt; wtp < &wndwt[WTSIZE]; wtp++,i++) {
	    if (wtp->wsalen == 0) {
	        scc = sendto(sc,(char *)&i,sizeof(i),0,(char *)&wmp->sfa,wmp->sfalen);
	        if (scc < 0)
		    error(NONALERT,elcsmsg[21],"window elcsd daemon");
		else {
	            wtp->wsa = wmp->sfa;
	            wtp->wsalen = wmp->sfalen;
	            wtp->class = wp->class;
	            wtp->type = wp->type;
	            wtp->ctldevtyp = wp->ctldevtyp;
	            wtp->num = wp->num;
	            wtp->unitnum = wp->unitnum;
	            owndflg++;
	            break;
		}
	    }
	}
	if (i >= WTSIZE) {
	    i = -1;
	    scc = sendto(sc,(char *)&i,sizeof(i),0,(char *)&wmp->sfa,wmp->sfalen);
	    if (scc < 0)
		error(NONALERT,elcsmsg[21],"window elcsd daemon");
	}
}

/*
 *  Function: msgclose(bufp)
 *
 *  Description: Close out entry in window table.
 *
 *  Arguments: char *bufp	pointer to data
 *
 *  Return value:  none
 *
 *  Side effects:  none
 */
msgclose(bufp)
char *bufp;
{
	struct wndwt *wtp;
	struct wndpkt *wp;

	wp = (struct wndpkt *)bufp;
	wtp = &wndwt[wp->class];
	wtp->wsalen = 0;
	wtp->class = 0;
	wtp->type = 0;
	wtp->ctldevtyp = 0;
	wtp->num = 0;
	wtp->unitnum = 0;
        owndflg--;
}

/*
 *  Function: cntlmsg(cc,buf)
 *
 *  Description: Process control messages form eli or window
 *
 *  Arguments:  int cc		character count
 *		char *buf	pointer to data
 *
 *  Return value: none
 *
 *  Side effects:  none
 */
cntlmsg(cc,buf)
int cc;
char *buf;
{
	int ret;
	struct wndpkt *wpp;

	wpp = (struct wndpkt *)buf;
	switch (wpp->msgtyp) {
	    case ELCLOSE_WNDW:
	    case ELOPEN_WNDW:
	        if (wprocflg == 0)
	            if (fwndproc() < 0)
			break;
	        ret = write(pfd[1],buf,cc);
	        if (ret < 0)
	            error(NONALERT,elcsmsg[7],(char *)0);
	        break;
	    case ELDISABLE:
	        elcsterm();
	        cntlmsgrply(wpp);
	        break;
	    case ELINITIALIZE:
	        if (ioctl(elfd, (int)ELREINIT, (char *)0) < 0)
	            error(ALERT,elcsmsg[24],(char *)0);
	        else {
	            error(NONALERT,elcsmsg[58],(char *)0);
	            cntlmsgrply(wpp);
	        }
	        break;
	    case ELRECONFIG:
	        elcsinit();
	        error(NONALERT,elcsmsg[59],(char *)0);
	        cntlmsgrply(wpp);
	        break;
	    case ELLOCK:
	        plock(PROCLOCK);
	        cntlmsgrply(wpp);
	        break;
	    default:
	        break;
	}
}

/*
 *  Function: cntlmsgrply(wpp)
 *
 *  Description: Control message reply to eli
 *
 *  Arguments:  struct wndpkt *wpp	pointer to window packet
 *
 *  Return value: none
 *
 *  Side effects:  none
 */
cntlmsgrply(wpp)
struct wndpkt *wpp;
{
	struct sockaddr_un scun;
	int scunlen;
	int scc;

	scun.sun_family = AF_UNIX;
	bcopy(elisckt,(char *)scun.sun_path,sizeof(elisckt));
	scunlen = sizeof(scun);

	scc = sendto(sc,(char *)&wpp->msgtyp,sizeof(wpp->msgtyp),0,(char *)&scun,scunlen);
	if (scc < 0)
	    error(NONALERT,elcsmsg[21],(char *)0);
}

/*
 *  Function: elcsterm()
 *
 *  Description: On terminate signal set flag which will allow
 *	select for last read of any data than exit.
 *
 *  Arguments: none
 *
 *  Return value: none
 *
 *  Side effects: none
 */
void elcsterm()
{
	termflg++;
}

/*
 *  Function: elcsclean()
 *
 *  Description: Shutdown elcsd clear pid in kernel, if windowing
 *	kill window process and	unlink elcsd control socket.
 *
 *  Arguments: none
 *
 *  Return value: none
 *
 *  Side effects: none
 */
void elcsclean()
{
	if (ioctl(elfd, (int)ELCLRPID, (char *)0) < 0)
	    error(NONALERT,elcsmsg[25],(char *)0);
	error(NONALERT,elcsmsg[26],elcsmode[mode]);
	if (wpid > 0)
	    (void)kill(wpid, SIGTERM);
	if (sc > 0)
	    (void)close(sc);
	(void)unlink(elcsckt); 
	exit(0);
}

/*
 *  Function: elcsinit()
 *
 *  Description: Initialize elcsd for logging errors. Calls rdcfg
 *	to read the elcsd config file, sets up sockets and log
 *	files. Also determines whether to log local or remote,
 *	and whether to log remote systems.
 *
 *  Arguments: none
 *
 *  Return value: none
 *
 *  Side effects: none
 */
void
elcsinit()
{
	struct eloft *p;
	struct elrht *rhtp;
	struct sockaddr_un scun;

	for (eloftp = eloft; eloftp < &eloft[OFTSIZE]; eloftp++) {
	    if (eloftp->fdes >= 0 && *eloftp->fname != '\0')
		closef(eloftp);
	    eloftp->fdes = -1;
	    eloftp->marker = -1;
	}
	for (rhtp = elrht; rhtp < &elrht[RHTSIZE]; rhtp++)
	    if (*rhtp->hname != '\0') {
		*rhtp->hname = '\0';
		rhtp->where = '\0';
	    }

	remlogflg = 0;
	logremflg = 0;
	prilogflg = 0;

	if (sc <= 0) {                           /* set up control socket */
	    sc = socket(AF_UNIX, SOCK_DGRAM, 0);
	    if (sc > 0) {
	        scun.sun_family = AF_UNIX;
		if (unlink(elcsckt) < 0)
		    errno = 0;
		bcopy(elcsckt,(char *)scun.sun_path,sizeof(elcsckt));
		if (bind(sc, (char *)&scun, sizeof(scun)) < 0) {
	    	    error(ALERT,elcsmsg[27],(char *)0);
		    (void)close(sc);
		    sc = 0;
		    scmask = 0;
		}
		else {
		    scmask = 1<<sc;
		    scfromlen = sizeof(scun);
		}
	    }
	    else 
	       error(ALERT,elcsmsg[28],(char *)0);
	}
	if (nologflg == 0) {                    /* log errors if not set */
	    rdcfg();		                /* read config file */
	    if (sglusrflg) {               	/* set up single user mode */
		pathflg = 3;
		p = &eloft[0];
		if (setlocpath(p) < 0) {
		    elcsclean();
		}
	    }
	    else {                
		setinetskt();             /* set up multi-user sockets */
	        if ((elcfg.status & REMLOG) == 0)
		    setlocal();           /* multi-user log errors locally */
	        else           
		    setremote();          /* set up remote logging of errors */
	    }
	    getdmpfil();
	}
}

/*
 *  Function: setinetskt()
 *
 *  Description: Set up inet datagram & stream sockets for logging
 *	remotes or remote logging to a host.
 *
 *  Arguments: none
 *
 *  Return value: none
 *
 *  Side effects: none
 */
setinetskt()
{
	int ret; 
	struct sockaddr_in dsin;
	struct sockaddr_in ssin;
	struct hostent *hp;
	struct servent *sp;

     	sp = getservbyname("elcsd","udp");
        if (sp == 0)
            error(ALERT,elcsmsg[29],(char *)0);
	else
	    port = sp->s_port;
        
	hp = gethostbyname(hostname);
	if (hp == 0) {
            error(ALERT,elcsmsg[30],hostname);
        }
	if (dsi <= 0 && hp != 0 && port != 0) {
	    dsi = socket(AF_INET, SOCK_DGRAM, 0);
	    if (dsi > 0) {
	        bzero((char *)&dsin,sizeof(dsin));
	        bcopy(hp->h_addr,(char *)&dsin.sin_addr,hp->h_length);
	        dsin.sin_family = AF_INET;
	        dsin.sin_port = port;
	        if (bind(dsi, (char *)&dsin, sizeof(dsin)) < 0) {
    	            error(ALERT,elcsmsg[31],(char *)0);
		    (void)close(dsi);
		    dsi = 0;
		    dsimask = 0;
	        }
		else {
	            dsifromlen = sizeof(dsin);
	            dsimask = 1<<dsi;
		}
	    }
	    else
    	        error(ALERT,elcsmsg[32],(char *)0);
	}
	if (ssi <= 0 && hp != 0 && port != 0) {
	    ssi = socket(AF_INET, SOCK_STREAM, 0);
	    if (ssi > 0) {
		bzero((char *)&ssin,sizeof(ssin));
		bcopy(hp->h_addr,(char *)&ssin.sin_addr,hp->h_length);
		ssin.sin_family = AF_INET;
		ssin.sin_port = port;
		if ((ret = bind(ssi,(char *)&ssin,sizeof(ssin))) == 0) {
		    if ((ret = listen(ssi, 5)) == 0) {
		        ssifromlen = sizeof(ssin);
		        ssimask = 1<<ssi;
		    }
		    else 
	                error(ALERT,elcsmsg[33],(char *)0);
		}
		else 
    		    error(ALERT,elcsmsg[34],(char *)0);
		if (ret < 0) {
		    (void)close(ssi);
		    ssi = 0;
		    ssimask = 0;
	        }
	    }
	    else
    	        error(ALERT,elcsmsg[35],(char *)0);
        }
}

/*
 *  Function: setlocal()
 *
 *  Description: Set up local errlog file in open file table,
 *	call chkcpy to copy single user or back file to the main
 *	errlog file, also check for remote logging of other systems.
 *
 *  Arguments: none
 *
 *  Return value: none
 *
 *  Side effects: none
 */
setlocal()
{
	int ret; 
	struct eloft *p;

	p = &eloft[0];
	pathflg = 1;
	ret = setlocpath(p);
	if (ret < 0) {
	    error(ALERT,elcsmsg[36],(char *)0);
	    elcsclean();
	}
	if (pathflg == 2)
	    error(ALERT,elcsmsg[37],p->fname);
	if (pathflg == 3)
	    error(ALERT,elcsmsg[38],p->fname);

	getdmpfil();

	if (*elcfg.supath != '\0') {
	    ret = chkfcpy(p,elcfg.supath);
	    if (ret < 0) {
	        error(ALERT,elcsmsg[39],(char *)0);
	    }
	}
	if (*elcfg.bupath != '\0') {
	    ret = chkfcpy(p,elcfg.bupath);
	    if (ret < 0) {
	        error(ALERT,elcsmsg[40],(char *)0);
	    }
	}
	if ((elcfg.status & LOGREM) == LOGREM)
	    if (dsi > 0 && ssi > 0) {
	        logremflg++;
	        error(NONALERT,elcsmsg[19],(char *)0);
		sndmsg(ELSTARTUP);
	    }
	    else
	        error(ALERT,elcsmsg[41],(char *)0);
}

/*
 *  Function: setremote()
 *
 *  Description: Set up remote logging to a host named in elcsd.conf.
 *
 *  Arguments: none
 *
 *  Return value: none
 *
 *  Side effects: none
 */
setremote()
{
	int ret; 
	int s, msg; 
	struct hostent *hp;

        hp = gethostbyname(elcfg.rhostn);
        if ( hp == 0)
            error(ALERT,elcsmsg[30],elcfg.rhostn);

	if (port != 0 && hp != 0) {
            bzero((char *)&tos,sizeof(tos));
	    bcopy(hp->h_addr,(char *)&tos.sin_addr,hp->h_length);
	    tos.sin_family = hp->h_addrtype;
	    tos.sin_port = port;
	    toslen = sizeof(tos);

	    s = socket(AF_INET, SOCK_STREAM, 0);
	    if (s < 0)
	        error(ALERT,elcsmsg[35],(char *)0);
	    else {
	        if ((ret = connect(s,(char *)&tos,toslen)) == 0) {
	            msg = ELSTRTRLOG;
	            ret = send(s,(char *)&msg,4,0);
	            if (ret > 0) {
		        ret = recv(s,(char *)&msg,4,0);
		        if (ret > 0) {
		            if (msg == ELGOAHEAD) {
	                        remlogflg++;
				error(NONALERT,elcsmsg[44],elcfg.rhostn);
			        if ((elcfg.status & PRILOG) == PRILOG)
		                    prilogflg++;
		            }
			    else {
				error(ALERT,elcsmsg[65],(char *)0);
				ret = -1;
			    }
		        }
			else
		            error(ALERT,elcsmsg[17],(char *)0);
		    }
		    else
		        error(ALERT,elcsmsg[15],(char *)0);
		}
		else
		    error(ALERT,elcsmsg[45],(char *)0);
	    }
	    if (s < 0 || ret <= 0) {
	        error(ALERT,elcsmsg[46],(char *)0);
		setlocal();
	    }
	    else {
		if (*elcfg.supath != '\0') {
		    ret = rchkfcpy(&eloft[0],elcfg.supath);
		    if (ret < 0) {
		        error(ALERT,elcsmsg[39],(char *)0);
		    }
		}
	    }
	    if (s > 0)
	        (void)close(s);
	}
	if (port == 0 || hp == 0) {
	    error(ALERT,elcsmsg[46],(char *)0);
	    setlocal();
	}
}

/*
 *  Function: setoft(fsync,oftp,path,hname)
 *
 *  Description: Open errlog file and set up entry in open file table
 *
 *  Arguments:  int fsync		if 1 do sync write
 *		struct eloft *oftp	pointer to open file table slot
 *		char *path		pointer to directory
 *		char *hname		pointer name of file
 *
 *  Return value: fd > 0 success
 *		  fd < 0 failed
 *
 *  Side effects: none
 */
int setoft(fsync,oftp,path,hname)
int fsync;
struct eloft *oftp;
char *path;
char *hname;
{
	int fd;
	char fname[MAX_PATH];
	char *s;

	s = path;
	if (*s == '/' && *++s == '\0')
	    (void)sprintf(fname,"%s%s",el,hname);
	else
	    (void)sprintf(fname,"%s%s%s",path,el,hname);
	
	if (fsync) {
	    fd = open(fname,O_CREAT|O_RDWR|O_FSYNC|O_APPEND,MODE);
	    if (fd < 0) {
	        error(NONALERT,elcsmsg[47],fname);
	    }
	}
	else {
	    fd = open(fname,O_CREAT|O_RDWR|O_APPEND,MODE);
	    if (fd < 0) {
	        error(NONALERT,elcsmsg[47],fname);
	    }
	}
	if (fd >= 0) {
	    oftp->fdes = fd;
	    (void)strcpy(oftp->hname, hname);
	    (void)strcpy(oftp->fname, fname);

	}
	return(fd);
}

/*
 *  Function: gofts()
 *
 *  Description: Get slot in open file table
 *
 *  Arguments: none
 *
 *  Return value: eloftp   	pointer to slot
 *
 *  Side effects:  none
 */
struct eloft *gofts()
{
	eloftp = &eloft[oftindx];
	if (eloftp->fdes != -1)
	    closef(eloftp);
	if (++oftindx == OFTSIZE)
	    oftindx = OFTINDX;
	return(eloftp);
}

/*
 *  Function: goftp(ptr)
 *
 *  Description: Return pointer to slot in open file table if argument
 *	(hostname) found.
 *
 *  Arguments: char *ptr	pointer to hostname
 *
 *  Return value: eloftp = 0  not found
 *		  eloftp > 0  found hostname 
 *
 *  Side effects: none
 */
struct eloft *goftp(ptr)
char *ptr;
{
	int i;
	eloftp = eloft;

	for (i = 0; i < OFTSIZE; i++, eloftp++) {
	    if (*eloftp->hname != '\0')
	        if (strncmp(ptr,eloftp->hname,12) == 0)
	            break;
	}
	if (i >= OFTSIZE)
	    eloftp = NULL;
	return(eloftp);
}

/*
 *  Function: chkfcpy(pt,path)
 *
 *  Description: Copy single user or backup errlog file to main errlog file
 *
 *  Arguments:  struct eloft *pt	pointer to open file table entry
 *		char *path		pointer to directory
 *
 *  Return value: ret > 0 success
 *		  ret < 0 failed
 *
 *  Side effects: none
 */
int chkfcpy(pt,path)
struct eloft *pt;
char *path;
{
	struct eloft *pf;
	int ret, cc;

	pf = gofts();
	ret = setoft(NOFSYNC,pf,path,hostname);
	if (ret >= 0) { 
	    if (strcmp(pf->fname,pt->fname) != 0) {
		while ((cc = read(pf->fdes, buf, BSIZE)) != 0) {
		    if (cc < 0) {
			error(NONALERT,elcsmsg[48],pf->fname);
			ret = cc;
			break;
		    }
	    	    ret = write(pt->fdes, buf, cc);
	    	    if (ret < 0) {
			error(NONALERT,elcsmsg[49],pt->fname);
			break;
		    }
		}
		if (ret > 0)
		    (void)unlink(pf->fname);
	        closef(pf);
	    }
	}
	return(ret);
}

/*
 *  Function: rchkfcpy(pt,path)
 *
 *  Description: Copy single user file to remote errlog file
 *
 *  Arguments:  struct eloft *pt	pointer to open file table entry
 *		char *path		pointer to directory
 *
 *  Return value: ret > 0 success
 *		  ret < 0 failed
 *
 *  Side effects: none
 */
int rchkfcpy(pt,path)
struct eloft *pt;
char *path;
{
	struct eloft *pf;
	int ret, cc;
	short recsz;
	short *szptr;

	pf = gofts();
	ret = setoft(NOFSYNC,pf,path,hostname);
	if (ret >= 0) { 
	    if (strcmp(pf->fname,pt->fname) != 0) {
	        while ((cc = read(pf->fdes, buf, sizeof(recsz))) != 0) {
	            if (cc < 0) {
		        error(NONALERT,elcsmsg[48],pf->fname);
		        ret = cc;
		        break;
	            }
		    szptr = (short *) buf;
		    recsz = *szptr;
		    if (recsz > EL_MAXRECSIZE) {
			error(NONALERT,elcsmsg[64],pf->fname);
		        ret =  cc; 	
		        break;
		    }
		    cc = read(pf->fdes, &buf[sizeof(recsz)], recsz-sizeof(recsz));
		    if (cc < 0) {
			error(NONALERT,elcsmsg[48],pf->fname);
			ret = cc;
			break;
		    }
		    /* send out datagram */
		    ret = sendto(dsi,buf,cc+sizeof(recsz),0,(char *)&tos,toslen);
		    if (ret < 0) {
			error(NONALERT,elcsmsg[13],(char *)0);
			ret = cc;
			break;
		    }
		}
		if (ret > 0)
		    (void)unlink(pf->fname);
	        closef(pf);
	    }
	}
	return(ret);
}

/*
 *  Function: closef(p)
 *
 *  Description: Close file descriptor and zero open file table entry
 *
 *  Arguments: struct eloft *p		pointer to open file table entry
 *
 *  Return value: none
 *
 *  Side effects: none
 */
int closef(p)
struct eloft *p;
{

	(void)close(p->fdes);
	*p->hname = 0;
	*p->fname = 0;
	p->fdes = -1;
}

/*
 *  Function: chkrht(hname)
 *
 *  Description: Check hostname is in remote host table
 *
 *  Arguments: char *hname	pointer to hostname to check
 *
 *  Return value: rhtp = 0 hostname not in table
 *		  rhtp > 0 pointer to slot
 *
 *  Side effects: none
 */
struct elrht *chkrht(hname)
char *hname;
{
	int i;
	struct elrht *rhtp;

	rhtp = elrht;
	for (i = 0; i < RHTSIZE; i++, rhtp++) {
	    if (*rhtp->hname != '\0')
	        if (strncmp(hname,rhtp->hname,12) == 0)
	            break;
	}
	if (i >= RHTSIZE)
	    rhtp = NULL;
	return(rhtp);
}

/*
 *  Function: setlocpath(p)
 *
 *  Description: Check if path name in config table and set up local errlog file
 *
 *  Arguments: struct eloft *p		pointer to open file table slot
 *
 *  Return value: ret < 0 failed
 *		  ret > 0 success
 *
 *  Side effects: none
 */
int setlocpath(p)
struct eloft *p;
{
	int ret = -1;

        if (pathflg == 1) {
            if (*elcfg.elpath != '\0') {
                ret = setoft(FSYNC,p,elcfg.elpath,hostname);
                if (ret < 0) {
	            error(ALERT,elcsmsg[50],(char *)0);
	            pathflg++;
		}
		else {
		    if (!sglusrflg) {
	                error(NONALERT,elcsmsg[55],p->fname);
		    }
		}
	    }
	    else {
	        error(ALERT,elcsmsg[22],"main file");
	        pathflg++;
	    }
        }
        if (pathflg == 2) {
            if (*elcfg.bupath != '\0') {
	        ret = setoft(FSYNC,p,elcfg.bupath,hostname);
	        if (ret < 0) {
    	            error(ALERT,elcsmsg[51],(char *)0);
	            pathflg++;
		}
		else {
		    if (!sglusrflg) {
	                error(NONALERT,elcsmsg[55],p->fname);
		    }
		}
	    }
	    else {
	        error(ALERT,elcsmsg[22],"backup file");
	        pathflg++;
	    }
        }
        if (pathflg == 3) {
            if (*elcfg.supath != '\0') {
                ret = setoft(FSYNC,p,elcfg.supath,hostname);
	        if (ret < 0)
    	            error(ALERT,elcsmsg[52],(char *)0);
		else {
		    if (!sglusrflg) {
	                error(NONALERT,elcsmsg[55],p->fname);
		    }
		}
	    }
	    else 
	        error(ALERT,elcsmsg[22],"single user file");
	}
	return(ret);
}

/*
 *  Function: wrtdata(rem,bufp,cc)
 *
 *  Description: Write data out to errlog files both local and remote files
 *
 *  Arguments:  int rem		1 local or 2 remote data
 *		char *bufp	pointer to data
 *		int cc		character count
 *
 *  Return value: ret < 0 failed
 *		  ret > 0 success
 *
 *  Side effects: none
 */
int wrtdata(rem,bufp,cc)
int rem;
char *bufp;
int cc;
{
	int first = 0;
	int ret = 0;
	int sf;
	struct eloft *p;
	struct el_rhdr *rhdp;
	struct elrht *rhtp;

	rhdp = (struct el_rhdr *)bufp;

	/* write local errors to errlog file */
	if (rem == 1) {
	    first = 0;
	    p = &eloft[0];
	    if (p->fdes <= 0)
	        ret = setlocpath(p);
	    else {
	        sf = statfile(p);
	        if (sf == 0)
	            ret = setlocpath(p);
	    }
	    if (ret >= 0 && sf >= 0) {
	        do {
	            if (first == 0) {
	                if (pathflg != 0) {
		            first++;
		            continue;
	                }
	            }
	            else {
	                error(ALERT,elcsmsg[53],p->fname);
	                closef(p);
	            }
	            pathflg++;
		    ret = setlocpath(p);
		    if (ret >= 0) {
		        first++;
		        continue;
		    }
		    break;
	        } while ((ret = write(p->fdes, bufp, cc)) < 0);
	    }
	}
	/* write remote errors to errlog files */
	else {
	    rhtp = chkrht(rhdp->rhdr_hname);
	    if (rhtp != 0) {
		if (rhtp->where == 'R') {
	            p = goftp(remotes);
		    if (p > 0) {
		        sf = statfile(p);
		    }
	            if (p == 0 || sf == 0) {
	                p = gofts();
	                ret = setoft(NOFSYNC,p,elcfg.rhpath,remotes);
	                if (ret < 0)
		            error(ALERT,elcsmsg[54],remotes);
	            }
		}
		else {
	            p = goftp(rhdp->rhdr_hname);
		    if (p > 0) {
		        sf = statfile(p);
		    }
	            if (p == 0 || sf == 0) {
	                p = gofts();
	                ret = setoft(NOFSYNC,p,elcfg.rhpath,rhdp->rhdr_hname);
	                if (ret < 0)
		            error(ALERT,elcsmsg[54],rhdp->rhdr_hname);
	            }
		}
	        if (ret >= 0 && sf >= 0) {
	            ret = write(p->fdes, bufp, cc);
	            if (ret < 0)
	                error(ALERT,elcsmsg[49],p->fname);
	        }
	    }
	    else
		ret = 1;
	}
	return(sf != -1 ? ret : 1);
}

/*
 *  Function: statfile(p)
 *
 *  Description: Stat errlog file for link count and file size
 *
 *  Arguments: none
 *
 *  Return value: ret == 1  write to disk
 *		  ret == 0  file is not linked reopen, write to disk
 *		  ret == -1 exceded file limit size or file system almost full
 *			    don't write to disk
 *
 *  Side effects: none
 */
int statfile(p)
struct eloft *p;
{
	int ret = 1;
	int num;
	struct stat statbuf;
	struct stat *stp = &statbuf;
	char line[128];

	(void)fstat((int)p->fdes, stp);
	if (stp->st_nlink == 0) {
	    closef(p);
	    ret = 0;
	}
	if (elcfg.limfsize > 0) {
	    if (elcfg.limfsize < stp->st_size) {
	        ret = -1;
		if (missed_cnt < MISSED_CNT) {  /* not too  many to syslog */
	            error(ALERT,elcsmsg[43],(char *)0);
		}
	    }
	    else if ((num = elcfg.limfsize - stp->st_size) < 3000) {
	        (void)sprintf(line,"%s %d",elcsmsg[42],num);
	            error(ALERT,line,(char *)0);
	    }
	}
	else if ((ret = fsfull(p->fname)) < 0) {
	    if (missed_cnt < MISSED_CNT) {  /* don't log too many to syslog */
	        error(ALERT,elcsmsg[67],eloft[0].fname);
	    }
	}
	if (ret == -1) { 	/* couldn't write */
		missed_cnt++;
	}
	else {
		missed_cnt = 0;
	};
	return(ret);
}

/*
 *  Function: fsfull(path)
 *
 *  Description: Check whether file system is almost full, return -1 if it is.
 *
 *  Arguments:	path		ptr to path name
 *
 *  Return value: ret > 0	write on file system
 *		  ret < 0	Don't write on file system
 *
 *  Side effects: none
 */
int fsfull(path)
char *path;
{
	struct fs_data fdbuf;

	if(statfs(path, &fdbuf) == 0)
		return(-1);
	if ( ((int)fdbuf.fd_bfreen < 0) || 
	(FSPERCENT * fdbuf.fd_btot  / 100 > fdbuf.fd_bfreen) ) 
		return(-1);	
	return(1);
}

/*
 *  Function: rdcfg()
 *
 *  Description: Read in the elcsd config file and set up the
 *	config table and remote host table.
 *
 *  Arguments: none
 *
 *  Return value: none
 *
 *  Side effects: none
 */
int rdcfg()
{
	int i, j;
	FILE *cfp;
	char *cp, *cp2;
	char line[256];
	int dataflg = 0;

	cfp = fopen(cfgfile,"r");
	if (cfp == NULL) {
	    error(ALERT,elcsmsg[56],(char *)0);
	    elcsclean();
	}
	i = 0;
	while (fgets(line,sizeof(line),cfp) != NULL) {
	    cp = line;
	    if (*cp == '#' && dataflg == 0)
		continue;
	    if (*cp == '}')
	        break;
	    if (*cp == '{') {
		dataflg++;
		continue;
	    }
	    if (dataflg > 0) {
	        while (*cp == ' ' || *cp == '\t')
		    cp++;
	        if (*cp == '#') {
	            if (i < 1) {
		        error(ALERT,elcsmsg[57],(char *)0);
			(void)fclose(cfp);
	                elcsclean();
		    }
	            else {
		        line[0] = '\0';
		        fct(i,line);
		        i++;
		        continue;
		    }
	        }
	        cp2 = cp;
	        while (*cp2 != ' ' && *cp2 != '\t' &&
		       *cp2 != '\n' && *cp2 != '#') 
		    cp2++;
	        *cp2 = '\0';
	        fct(i,cp);
	        i++;
	    }
	}
	if (dataflg == 0 || i != 7) {
            error(ALERT,elcsmsg[61],(char *)0);
	    (void)fclose(cfp);
            elcsclean();
	}
	for (i = 0; i < RHTSIZE && fgets(line,sizeof(line),cfp) != NULL; i++) {
	    cp = line;
	    if (*cp == '#')
		continue;
	    while (*cp == ' ' || *cp == '\t')
		cp++;
	    cp2 = cp;
	    for (j = 0; j < 2; j++) {
	        while (*cp2 != ':' && *cp2 != '\t' && *cp2 != '\n' && 
		       *cp2 != ' ' && *cp2 != '#') 
		    cp2++;
	        *cp2 = '\0';
		if (j == 0) 
	            (void)strcpy(elrht[i].hname,cp);
		if (j == 1) 
	            elrht[i].where = *cp;
		cp = ++cp2;
	    }
	}
	(void)fclose(cfp);
}

/*
 *  Function: fct(i,cp)
 *
 *  Description: Set up config table called by rdcfg()
 *
 *  Arguments: 	int i		index
 *		char *cp	pointer to string data
 *
 *  Return value: none
 *
 *  Side effects: none
 */
fct(i,cp)
int i;
char *cp;
{
	int num;

	switch (i) {
	case 0:
	    num = atoi(cp);
	    elcfg.status = num;
	    break;
	case 1:
	    num = atoi(cp);
	    num *= 512;
	    elcfg.limfsize = num;
	    break;
	case 2:
	    (void)strcpy(elcfg.elpath,cp);
	    break;
	case 3:
	    (void)strcpy(elcfg.bupath,cp);
	    break;
	case 4:
	    (void)strcpy(elcfg.supath,cp);
	    break;
	case 5:
	    (void)strcpy(elcfg.rhpath,cp);
	    break;
	case 6:
	    (void)strcpy(elcfg.rhostn,cp);
	    break;
	}
}

/*
 *  Function: getdmpfil()
 *
 *  Description: Get the errlog dump file "elbuffer" which
 *	has in it the errlog data from the kernel buffer 
 *	at the time of a crash.
 *
 *  Arguments: none
 *
 *  Return value: none
 *
 *  Side effects: none
 */
getdmpfil()
{
	int fd, cc;
	int amt;
	char *pos;
	struct elbuf *elbufp;
	char *pin, *pout, *ple;

	char svelbuf[MAX_PATH + 10]; 		/* store elbuffer pathname */

	sprintf(svelbuf, "%s%s", elcfg.elpath, "/elbuffer");
	fd = open(svelbuf,O_RDONLY,0);
	if (fd < 0) {
	    if (errno != ENOENT) {
	        error(ALERT,elcsmsg[62],svelbuf);
		errno = 0;
		return;
	    }
	    else {	 /* try single user directory */
		sprintf(svelbuf, "%s%s", elcfg.supath, "/elbuffer");
		fd = open(svelbuf, O_RDONLY, 0);
		if (fd < 0) {
		    if (errno != ENOENT) {
			error(ALERT,elcsmsg[62],svelbuf);
		    }		
		    errno = 0;
		    return;		
		}
	    }
	}
	while ((cc = read(fd,(char *)&pos,sizeof(pos))) != 0) {
	    if (cc < 0) {
	        error(ALERT,elcsmsg[62],svelbuf);
		break;
	    }
	    cc = read(fd,buf,sizeof(buf));
	    if (cc < 0) {
	        error(ALERT,elcsmsg[62],svelbuf);
		break;
	    }
	    else if ((cc == 0) || (cc < sizeof(buf))) {
	        break;
	    }
	    elbufp = (struct elbuf *)buf;
	    pin = &buf[elbufp->in - pos];
	    pout = &buf[elbufp->out - pos];
	    ple = &buf[elbufp->le - pos];
	    if (pout == ple && pin < pout) {
		pout = elbufp->kqueue;
	    }
	    if (pin < pout) {			/* read data from out to le */
		amt = ple - pout;
		if ((amt >= EL_MISCSIZE) && (amt < EL_BSIZE) 
			&& (pout > &buf[11])
			&& ((pout + amt) < &buf[EL_BSIZE + 12])) {
		    fillinhdr(amt,pout);
		    if (remlogflg > 0)
			sndrem(amt,pout);
		    else
		        (void)wrtdata(LOCLOG,pout,amt);
		}
		amt = pin - elbufp->kqueue;   /* read data from kqueue to in */
		if ((amt >= EL_MISCSIZE) && (amt < EL_BSIZE)) {
		    fillinhdr(amt,elbufp->kqueue);
		    if (remlogflg > 0)
			sndrem(amt,elbufp->kqueue);
		    else
		        (void)wrtdata(LOCLOG,elbufp->kqueue,amt);
		}
	    }
	    else if (pin > pout) {
		amt = pin - pout; 		/* read data from out to in */
		if ((amt >= EL_MISCSIZE) && (amt < EL_BSIZE) 
			&& (pout > &buf[11])
			&& ((pout + amt) < &buf[EL_BSIZE + 12])) {
		    fillinhdr(amt,pout);
		    if (remlogflg > 0)
			sndrem(amt,pout);
		    else
		        (void)wrtdata(LOCLOG,pout,amt);
		}
	    }
	    pin = &buf[elbufp->sin - pos];
	    if (pin > elbufp->squeue) {
		amt = pin - elbufp->squeue;  /* read data from squeue to sin */
		if ((amt >= EL_MISCSIZE) && (amt < EL_SSIZE)) {
		    fillinhdr(amt,elbufp->squeue);
		    if (remlogflg > 0)
			sndrem(amt,elbufp->squeue);
		    else
		        (void)wrtdata(LOCLOG,elbufp->squeue,amt);
		}
	    }
	    break;
	}
	(void)close(fd);
	(void)unlink(svelbuf);
	return;
}

/*
 *  Function: error(alert,cp1,cp2)
 *
 *  Description: This function takes the data pointed to by 
 *	the two char pointer args and outputs the msgs to
 *	elcsdlog file. Also if alert is set notify system
 *	manager(root) and log in syslog file.
 *
 *  Arguments:  int alert - 0 nonalert or 1 alert system manager(syslog)
 *      	char *cp1	pointer to string data
 *      	char *cp2	pointer to string data
 *
 *  Return value: none
 *
 *  Side effects: none
 */
error(alert,cp1,cp2)
int alert;
char *cp1;
char *cp2;
{
	long now;
	int saverrno;
	char errline[256];
	char line[256];

	saverrno = errno;
	(void)time((time_t *)&now);

        if (saverrno == 0) {
	    sprintf(errline,"%.24s  elcsd:%s %s;",
	            ctime((time_t *)&now),cp1,cp2);
	}
	else if (saverrno > sys_nerr || saverrno < 0) {
	    sprintf(errline,"%.24s  elcsd:%s %s; errno = %d",
	  	    ctime((time_t *)&now),cp1,cp2,saverrno);
	}
	else {
	    sprintf(errline,"%.24s  elcsd:%s %s; %s",
		    ctime((time_t *)&now),cp1,cp2,sys_errlist[saverrno]);
	}

	if (!sglusrflg && logfp != NULL) {
	    if (fsfull(logpath) > 0) {
		    fprintf(logfp,"%s\n",errline);
	            (void)fflush(logfp);
	    }
	    else {
		if (missed_cnt < MISSED_CNT) {  /* not too many to syslog */
	            (void)sprintf(line,"%.24s  elcsd:%s %s",ctime((time_t *)&now),elcsmsg[67],logpath);
		    fprintf(logfp,"%s\n",line);
	            (void)fflush(logfp);
	            (void)sprintf(line,"elcsd:%s %s; ",elcsmsg[67],logpath);
	            syslog(LOG_SALERT, line);
	            alert = 1;
		}
	    }
	    if (alert) {
		if (missed_cnt < MISSED_CNT ) {  /* don't log too many */
		    errno = saverrno;
	            (void)sprintf(line,"elcsd:%s %s; ",cp1,cp2);
	            syslog(LOG_SALERT, line);
		}
	    }
	}
	else {
	        logfp = fopen("/dev/console","w");
	        if (logfp != NULL) {
	            fprintf(logfp,"%s\n",errline);
		    (void)fflush(logfp);
	            (void)fclose(logfp);
	        }
	}
	errno = 0;
}
