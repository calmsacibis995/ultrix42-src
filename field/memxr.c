
#ifndef lint
static char *sccsid = "@(#)memxr.c	4.1	ULTRIX	7/2/90";
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
/*
 *
 *	MEMXR.C -- This routine perform the memory exercises.  
 *
 */

#include <stdio.h>
#include <sys/param.h>
#include "diag.h"


#define MODULE		"memxr"
#define TESTLOOP	3		/* number of test iterations */
#define ZZZZZZ		60		/* number of sleep seconds */
#define MAXERRCT	10

/*
 *	Global data
 */
int ptrn[] = {DG_RANDM,DG_5555,DG_AAAA};

char errbuf[1024];
char *errptr;				/* pointer to error buffer */

char tstbuf[DM_CLICK];

char module[14];
int success = 0;
int errct1 = 0;				/* count of number overall errors */

/* run time variables */
int timedelta;
long stoptime;

main (argc,argv)
int argc;
char *argv[];
{
register i,j,k;
register char *bufptr;
register char *buf;			/* pointer to allocated memory segment*/
unsigned long memsize;			/* size of memory segment */
int testnum;				/* process test number */
int numcl;				/* number of clicks in mem segment */
char *tst, *rtn;
int errct;				/* count of number of bytes in err */
long itime;
char *malloc();
void clean();

	/* insure that upon receiving a kill signal, cleanup can occur */
	signal(SIGTERM,clean);
	signal(SIGINT,clean);

	/* retrieve test number */
	sprintf(module,"%s%s",MODULE,argv[2]);
	testnum = atoi(argv[2]);

	if (testnum < 1 || testnum > DM_MAXPROC)
	{
		sprintf(errbuf,"Invalid test number - %d",testnum);
		report(DR_WRITE,module,errbuf);
		clean();
	}

	/* retrieve minimum memory block size */
	memsize = atoi(argv[1]);
	if (memsize < DM_MINMEM)
	{
		sprintf(errbuf,"(test %d) Memory block size = %d; must be at least %d",testnum,memsize,DM_MINMEM);
		report(DR_WRITE,module,errbuf);
		clean();
	}

	/* retrieve log file descriptor */
	logfd = atoi(argv[3]);

	timedelta = atoi(argv[4]);
	if (timedelta)
		stoptime = time(0) + (timedelta * 60);

	/*
	 *	In this loop, the following steps are followed:
	 *		1.  Memory is allocated
	 *		2.  Seed random number generator
	 *		3.  random data is written/read/verified (1) times
	 *		    also  a 55 pattern and AA pattern is done
	 *		4.  Memory is deallocated
	 *		5.  routine is put to sleep for (x) seconds
	 *		6.  goto 1.
	 */
	errct1 = 0;
	forever {
		buf = malloc(memsize);
		if (buf == 0) {
			sprintf(errbuf,"Exerciser Process Stopped; out of memory or swap space!");
			report(DR_WRITE,module,errbuf);
			clean();
		}
			
		itime = time(0) & 0x1ffff;
		numcl = (memsize + (DM_CLICK - 1)) / DM_CLICK;

		/*
		 *  In this loop, a random pattern is generated, the memory
		 *  is filled, the pattern checked and any discrepacies are
		 *  reported.
		 */
		for (i = 0; i < TESTLOOP; i++) {

			/* write out random pattern */
			randx = itime;
			bufptr = buf;
			for (j = 0; j < numcl; j++) {
				pattern(ptrn[i],DM_CLICK,bufptr);
				bufptr += DM_CLICK;
			}

			/* set up error message header */
			sprintf(errbuf,"Data error in memory:\n");
			errptr = errbuf;
			bumpptr(errptr);
			errct = 0;

			/* read and verify random pattern */
			randx = itime;
			bufptr = buf;
			for (j = 0; j < numcl; j++) {
				pattern(ptrn[i],DM_CLICK,tstbuf);
				tst = tstbuf;
				rtn = bufptr;
				for (k = 0; k < DM_CLICK; k++)
					if (*rtn++ != *tst++) {
						if (++errct > MAXERRCT) {
							sprintf(errptr,"[error printout limit exceeded]\n");
							break;
						}
						sprintf(errptr,"VIRTUAL BYTE = %x	GOOD = %x	BAD = %x\n",bufptr + k,*(tst-1),*(rtn-1));
						bumpptr(errptr);
					}

				if (errct > MAXERRCT)
					break;
				bufptr += DM_CLICK;
			}

			/* if data error then report it */
			if (errct)
			{
				report(DR_WRITE,module,errbuf);
				errct1++;
				break;
			}
			if (tstop())
				break;
			itime = randx & 0x1ffff;
		}
		free(buf);
		if (errct1 >= MAXERRCT)
		{
			sprintf(errbuf,"Too many errors\n");
			report(DR_WRITE,module,errbuf);
			clean();
		}
		success++;
		if (tstop())
			clean();
		sleep(ZZZZZZ);
	}

}

int tstop()
{
	return(stoptime && stoptime < time(0) ? 1 : 0);
}

/**/
/*
 *
 *	CLEAN -- This routine will print the diagnostics when the process has
 *		 been killed
 *
 */

void clean()
{

	sprintf(errbuf,"Exerciser Process Stopped;  %d successful passes  %d failures\n",success,errct1);
	report(DR_WRITE,module,errbuf);
	exit(0);
}
