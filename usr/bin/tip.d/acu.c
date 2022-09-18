#ifndef lint
static char *sccsid = "@(#)acu.c	4.1	ULTRIX	7/17/90";
#endif

/************************************************************************
 *									*
 *			Copyright (c) 1985 by				*
 *		Digital Equipment Corporation, Maynard, MA		*
 *			All rights reserved.				*
 *									*
 *   This software is furnished under a license and may be used and	*
 *   copied  only  in accordance with the terms of such license and	*
 *   with the  inclusion  of  the  above  copyright  notice.   This	*
 *   software  or  any	other copies thereof may not be provided or	*
 *   otherwise made available to any other person.  No title to and	*
 *   ownership of the software is hereby transferred.			*
 *									*
 *   This software is  derived	from  software	received  from	the	*
 *   University    of	California,   Berkeley,   and	from   Bell	*
 *   Laboratories.  Use, duplication, or disclosure is	subject  to	*
 *   restrictions  under  license  agreements  with  University  of	*
 *   California and with AT&T.						*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or	reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

/************************************************************************
 *			Modification History
 *
 *  0004  Marc Teitelbaum, Mar 9 1886
 *	  Set pgrp of dialer tty so SIGHUP will be recieved
 *	  when dialer hangs up.
 *
 *  0003  Marc Teitelbaum, Dec 8 1985
 *	  Fix "dialing..." problem reported by Karen Meadows.
 *	  If the generic dialer is being used and the phone number was
 *	  looked up in /etc/phones, then the "dialing..." message was
 *	  not being generated. 
 *
 *  0002  Marc Teitelbaum, Dec 8 1985
 *        Fix QAR MST-197.
 *	  Code that outputs connection message (cm=XXXX capability) was
 *	  incorrectly outputing phone number instead.  For a direct connect
 *	  line there was no phone number and tip dumped core.  
 *
 *  0001  Marc Teitelbaum, Aug 1 1985
 *        Code that parses lines from /etc/phones was reporting
 *	  "unrecognizable host name" if a line was blank.  It now
 *	  ignores blank lines.
 *
 ***********************************************************************/
	
#include "tip.h"

static acu_t *acu = NOACU;
static int conflag;
static int acuabort();
static acu_t *acutype();
static jmp_buf jmpbuf;
/*
 * Establish connection for tip
 *
 * If DU is true, we should dial an ACU whose type is AT.
 * The phone numbers are in PN, and the call unit is in CU.
 *
 * If the PN is an '@', then we consult the PHONES file for
 *   the phone numbers.  This file is /etc/phones, unless overriden
 *   by an exported shell variable.
 *
 * The data base files must be in the format:
 *	host-name[ \t]*phone-number
 *   with the possibility of multiple phone numbers
 *   for a single host acting as a rotary (in the order
 *   found in the file).
 */
#ifdef GENACU
extern int gen_dialer(), gen_disconnect(), gen_abort();

acu_t gen = { "generic", gen_dialer, gen_disconnect, gen_abort };
#endif
char *
connect()
{
	register char *cp = PN;
	char *phnum, string[256];
	extern FILE *phonesfile;	/* opened in tip.c or cu.c */
	int tried = 0;
	int pgrp;
#ifdef GENACU
	extern int generrno;
#endif

	if (!DU) {		/* regular connect message */
		if (CM != NOSTR) {	/* 0002 */
			register char *cm=CM;
			pwrite(FD, cm, size(CM));
		}
		return (NOSTR);
	}
	/*
	 * @ =>'s use data base in PHONES environment variable
	 *	  otherwise, use /etc/phones
	 */
	signal(SIGINT, acuabort);
	signal(SIGQUIT, acuabort);
	if (setjmp(jmpbuf)) {
		signal(SIGINT, SIG_IGN);
		signal(SIGQUIT, SIG_IGN);
		printf("\ncall aborted\n");
		logent(value(HOST), "", "", "call aborted");
		if (acu != NOACU) {
			boolean(value(VERBOSE)) = FALSE;
			if (conflag)
				disconnect();
			else
				(*acu->acu_abort)();
		}
		delock(uucplock);
		exit(1);
	}
	if ((acu = acutype(AT)) == NOACU)
		return ("unknown ACU type");

	/*
	 * Set pgrp of dialer so HUP gets delivered.  marc-0004
	 * If it appears we don't have our own process group,
	 * (e.g. we were started by sh instead of ksh or csh),
	 * then don't do this otherwise we would be sending
	 * HUP to the shell also.  If the pgrp is the same as
	 * the pid we're safe.
	 */
	pgrp = getpgrp(0); 
	if (pgrp == getpid())
		ioctl(FD, TIOCSPGRP, &pgrp);

	if (*cp != '@') {
		while (*cp) {
			for (phnum = cp; *cp && *cp != ',' ; cp++)
				;
			if (*cp)
				*cp++ = '\0';


			generrno = 0;
			if (boolean(value(VERBOSE)) && acu == &gen) {
			/* "dialing" only if generic */
				printf("\ndialing...");
				fflush(stdout);
			}
			if (conflag = (*acu->acu_dialer)(phnum, CU)) {
				logent(value(HOST), phnum, acu->acu_name,
					"call completed");
				return (NOSTR);
			} else {
				logent(value(HOST), phnum, acu->acu_name,
					"call failed");
#ifdef GENACU
#define NOSYNC 1
#define BADDIAL 2
#define NOCAR 3
				if(generrno != 0) {
					if(generrno == NOSYNC) {
						printf("can't synchronize\n");
						logent(value(HOST), phnum, "generic", "can't synch up");
					} else if(generrno == BADDIAL) {
						printf("error dialing\n");
					} else if(generrno == NOCAR) {
						printf("No carrier detected...\n");
						logent(value(HOST), phnum, "generic", "dialer timeout");
					}
				}
#endif
			}
			tried++;
		}
	} else {
		if (phonesfile == NOFILE) {
			printf("%s: ", PH);
			return ("can't open phone number file");
		}
		while (fgets(string, sizeof(string), phonesfile) != NOSTR) {
			for (cp = string; !any(*cp, " \t\n"); cp++)
				;
			if (*cp == '\n') {
				continue;
			}
			*cp++ = '\0';
			if (strcmp(string, value(HOST)))
				continue;
			while (any(*cp, " \t"))
				cp++;
			if (*cp == '\n') {
				fclose(phonesfile);
				return ("missing phone number");
			}
			for (phnum = cp; any(*cp, "0123456789-*="); cp++)
				;
			*cp = '\0';

			if (boolean(value(VERBOSE)) && acu == &gen) { /*0003*/
			/* "dialing" only if generic */
				printf("\ndialing...");
				fflush(stdout);
			}
			if (conflag = (*acu->acu_dialer)(phnum, CU)) {
				fclose(phonesfile);
				logent(value(HOST), phnum, acu->acu_name,
					"call completed");
				return (NOSTR);
			} else
				logent(value(HOST), phnum, acu->acu_name,
					"call failed");
			tried++;
		}
		fclose(phonesfile);
	}
	if (!tried)
		logent(value(HOST), "", acu->acu_name, "missing phone number");
	else
		(*acu->acu_abort)();
	return (tried ? "call failed" : "missing phone number");
}

disconnect()
{
	if (!conflag)
		return;
	logent(value(HOST), "", acu->acu_name, "call terminated");
	if (boolean(value(VERBOSE)))
		printf("\r\ndisconnecting...");
	/* Ignore soft carrier here */
	{
		int temp = 0;
	        (void) ioctl(FD, TIOCNCAR, &temp);
	}
	(*acu->acu_disconnect)();
}

static int
acuabort(s)
{
	signal(s, SIG_IGN);
	longjmp(jmpbuf, 1);
}


static acu_t *
acutype(s)
	register char *s;
{
	register acu_t *p;
	extern acu_t acutable[];
#ifdef GENACU
	extern acu_t *gen_setup();
	char gbuf[BUFSIZ];

	if (agetent(gbuf, s) > 0) {
		(void) gen_setup(gbuf, FD);
		return (&gen);
	}
	else
#endif
	for (p = acutable; p->acu_name != '\0'; p++)
		if (!strcmp(s, p->acu_name))
			return (p);
	return (NOACU);
}

