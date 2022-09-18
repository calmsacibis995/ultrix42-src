
#ifndef lint
static char *sccsid = "@(#)netx.c	4.2	ULTRIX	7/15/88";
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
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/
/************************************************************************
 *			Modification History
 *
 * 7/15/88 -- prs
 *      Added char string DR_NETX to allow proper use with -o option.
 *
 ************************************************************************/

/*
 *
 *	NETX.C -- This file contains the tcp/ip network exerciser modules
 *
 */
#include <sys/param.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/fs.h>
#include <netinet/in.h>
#include <sys/file.h>
#include <sys/time.h>
#include <sys/errlog.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <netdb.h>
#include "diag.h"

#define MODULE		"netx"			/* name of module */
#define MINUTE		60			/* minute in 1 sec counts */
#define SECOND		1 			/* one second */
#define MAXERRCT	10			/* maximum number of bad char */
#define LOCHOSTLEN	255			/* maximum number of bad char */

char *help[] =
{
	"\n\n(netx) - ULTRIX-32 Tcp/ip Net Exerciser\n",
	"\n",
	"usage:\n",
	"\tnetx [-h] [-t#] [-p#] nodename\n",
	"\n",
	"-h\t(optional) Print this help message\n",
	"-t#\t(optional) Run time in minutes (default: run forever until cntl-c)\n",
	"-p#\t(optional) Port number to use in internet domain (port# < 32768)\n",
	"nodename - remote or local system host name running the server\n",
	"\n",
	"The netx exerciser runs inconjunction with miscd server and the echo\n",
	"service which has to be enabled in the /etc/inetd.conf file.\n",
	"No # in front of the echo service.\n",
	"\n",
	"examples:\n",
	"\tnetx -t60 photon &\n",
	"\t{ Exercise node photon for 60 min. in the background }\n",
	"\tnetx keel\n",
	"\t{ Exercise node keel forever }\n",
	"\n",
	"",
};

struct net_stat nstat;				/* netx status struct */
struct net_stat *nstatp;

char errbuf[1024];				/* error message buffer */
char *errptr;					/* pointer to err buf */

char *lochost;					/* local host name ptr */
char lochostn[LOCHOSTLEN];			/* local host name */

u_char wrtbuf[2048];				/* write message buffer */
u_char rdbuf[2048];				/* read data buffer */

int rb, wb;					/* number of chars from 
						   read/write*/
int timedelta;					/* run time variables */
long stoptime;

int s;						/* socket descriptor */

char DR_NETX[] = "#LOG_NETX_1";			/* Logfile name */

main(argc, argv)
	int argc;
	char **argv;
{
	void netx_clean();
	struct sockaddr_in sin;
	struct sockaddr_in from;
	struct hostent *hp;
	struct servent *sp;
	int i, port = 0;
	char *ptr;

	signal(SIGTERM,netx_clean);
        signal(SIGINT,netx_clean); 

	if (argc == 1) {
		printf("usage: netx arg, type \"netx -h\" for help\n");
		exit(0);
	}

	while (--argc > 0) {
	    if (**++argv == '-') {
		switch (*++*argv) {

		case 't':
			timedelta = atoi(++*argv);
			break;
		case 'p':
			port = atoi(++*argv);
			if (port > 32768) {
				printf("netx: port %d invalid, type \"netx -h\" for help\n",port);
				exit(0);
			}
			break;
		case 'h':
			for (i = 0; *help[i]; i++)
				printf("%s",help[i]);
			exit(0);
		default:
			printf("netx: Invalid arg %s, type \"netx -h\" for help\n",*argv);
			exit(0);
		}
	    }
	    else {
		ptr = nstat.node;
		while (**argv)
			*ptr++ = *(*argv)++;
	    }
	}

	if (timedelta)
		stoptime = (timedelta * 60) + time(0);

	if (nstat.node[0] == 0) {
		fprintf(stderr,"%s: No node chosen for test! type \"netx -h\" for help\n",MODULE);
		exit(0);
	}
	lochost = lochostn;
	if ((gethostname(lochost,LOCHOSTLEN)) < 0) {
		fprintf(stderr,"%s: Gethostname failed - %s\n",MODULE,
			sys_errlist[errno]);
		exit(0);
	}

	if (report(DR_OPEN,MODULE,DR_NETX)) {
		fprintf(stderr,"%s: Can not start report generator, test aborted\n",MODULE);
		exit(1);
	}
	if (!port) {
		sp = getservbyname("echo","tcp");
	/*	printf("port sp %d\n",ntohs(sp->s_port)); */
		if (sp == 0) {
			fprintf(stderr,"%s: Unknown service echo/tcp, check /etc/services file for entry\n",MODULE);
			exit(1);
		}
	}
	hp = gethostbyname(nstat.node);
	if ( hp == 0) {
		fprintf(stderr,"%s: Unknown host %s, check /etc/hosts file for entry\n",MODULE,nstat.node);
		exit(1);
	}
	bzero((char *)&sin,sizeof(sin));
	bcopy(hp->h_addr,(char *)&sin.sin_addr,hp->h_length);
	sin.sin_family = hp->h_addrtype;
	sin.sin_port = port ? htons(port) : sp->s_port;
/*	printf("port sin %d\n",sin.sin_port); */
	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s < 0) {
		sprintf(errbuf,"socket error: %s",sys_errlist[errno]);
		report(DR_WRITE,MODULE,errbuf);
		netx_clean();
	}
	if (connect(s,(char *)&sin,sizeof(sin)) < 0) {
		sprintf(errbuf,"connect error: %s",sys_errlist[errno]);
		report(DR_WRITE,MODULE,errbuf);
		netx_clean();
	}
	doit(s);

}

doit(sd)
	int sd;
{
	int j, pktsize;
	u_char *wbp, *rbp;
	int errct;
	int errct2 = 0;
	int errct3 = 0;

	sprintf(errbuf,"Started netx exerciser - testing: %s to %s\n",
		lochost,nstat.node);
	(void)logerr(ELMSGT_DIAG,errbuf);
	report(DR_WRITE,MODULE,errbuf);

	randx = time(0);
	for (;;) {
	    	pktsize = time(0) & 0x1ff;
	    	pktsize = rng(pktsize) & 0x3;
	    	pktsize += 1;
	    	pktsize *= DEV_BSIZE;

		pattern(DG_RANDM, pktsize, wrtbuf);
		if ((wb = write(sd, wrtbuf, pktsize)) < 0) {
			sprintf(errbuf,"Write failure: %s\n	requested bytes = %d; received bytes = %d",sys_errlist[errno],pktsize,wb);
			report(DR_WRITE,MODULE,errbuf);
			nstat.n_fail++;
			if (errno != EIO)
				netx_clean();
			if (++errct2 > MAXERRCT)
				break;
			continue;
		}
		nstat.n_wrts++;
		nstat.n_wcc += wb;
		rbp = rdbuf;
		do {
		    if ((rb = read(sd, rbp, pktsize)) < 0) {
			sprintf(errbuf,"Read failure: %s\n	requested bytes = %d; received bytes = %d",sys_errlist[errno],pktsize,rb);
			report(DR_WRITE,MODULE,errbuf);
			nstat.n_fail++;
			if (errno != EIO)
				netx_clean();
			++errct3;
		    }
		    if (rb > 0) {
		        rbp += rb;
		        pktsize -= rb;
			nstat.n_rds++;
			nstat.n_rcc += rb;
		    }
		} while (pktsize && !errct3) ;
		if (errct3)
			if (errct3 > MAXERRCT)
				break;
			else
				continue;

		wbp = wrtbuf;
		rbp = rdbuf;
		sprintf(errbuf,"Data error in looped packet\n");
		errptr = errbuf;
		bumpptr(errptr);
		errct = 0;
		for (j = 0; j < pktsize; j++) {
		    if (*rbp++ != *wbp++) {
			if (++errct > MAXERRCT) {
			    sprintf(errptr,"[error printout limit exceeded]\n");
			    break;
			}
			sprintf(errptr,"BYTE = %d	GOOD = %x	BAD = %x\n",j+1,*(wbp - 1),*(rbp - 1));
			bumpptr(errptr);
		    }
		}
	/*	printf("compared wrtbuf & rdbuf\n"); */

		/* if data error then report it */
		if (errct) {
			nstat.n_fail++;
			report(DR_WRITE,MODULE,errbuf);
		}
		else
			nstat.n_pass++;

		if (tstop())
			break;
	}
	netx_clean();
	
}

/* check stop time */
int tstop()
{
	return(stoptime && stoptime < time(0) ? 1 : 0);
}

void netx_clean()
{
	shutdown(s,2);
	sprintf(errbuf,"Stopped netx exerciser - tested: %s to %s",
		lochost,nstat.node);
	(void)logerr(ELMSGT_DIAG,errbuf);
	report(DR_WRITE,MODULE,errbuf);
	netx_report();
	report(DR_CLOSE,0,DR_NETX);
	exit(0);
}

int netx_report()
{
	nstatp = &nstat;

	errptr = errbuf;
	sprintf(errptr,"Statistics: ");
	bumpptr(errptr);
	sprintf(errptr,"\n\nHost    -    Node     Writes     Kbytes ");
	bumpptr(errptr);
	sprintf(errptr,"     Reads     Kbytes      Passed Failed\n");
	bumpptr(errptr);
	sprintf(errptr,"%-8s %8s %10d %10.1f %10d %10.1f",lochost,
		nstatp->node, nstatp->n_wrts,nstatp->n_wcc/1000,
		nstatp->n_rds,nstatp->n_rcc/1000);
	bumpptr(errptr);
	sprintf(errptr,"  %10d %6d\n",nstatp->n_pass,nstatp->n_fail);
	bumpptr(errptr);
	
	sprintf(errptr,"\n");
	report(DR_WRITE,MODULE,errbuf);

}
