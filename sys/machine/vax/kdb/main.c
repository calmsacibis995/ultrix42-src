#ifndef lint
static	char	*sccsid = "@(#)main.c	4.1	ULTRIX	7/2/90";
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
 * adb - main command loop and error/interrupt handling
 */
#include "defs.h"
#include "../h/smp_lock.h"
#include "../h/ioctl.h"
#include "../h/tty.h"
#include "../h/proc.h"

MSG		NOEOR;

INT		kdb_executing;
INT		infile;
CHAR		*kdb_lp;
L_INT		kdb_maxpos;
long		kdb_maxoff = MAXOFF;
STRING		kdb_errflg;

CHAR		kdb_lastc;
INT		kdb_eof;

INT		kdb_lastcom;

long	kdb_maxpos = MAXPOS;
char	*Ipath = "/usr/lib/adb";

extern char kdb_strings[];
extern char *panicstr;
kern_debugger()
{
	L_INT		exitflg;
	struct proc *proc;
	struct pte *pte;
	static int first_time = 1;

	/*kdb_getconsole();*/
	kdb_flush_input();
	if (first_time) {
		kdbred();
		if (verify_kdb() == 0)
			return;
		mkioptab();

		first_time = 0;
		setsym(); 
		setcor(); 
		setvar();
	} 
	readregs();
	cprintf("ULTRIX Kernel Debugger\n");
	cprintf("type  :c  to leave the debugger\n");
	cprintf("type  $q  to sync the disks and reboot \n");
	if(panicstr) {
		cprintf("Paniced cpu = %d\n",kdb_req_cpu);
		if (proc = get_proc(kdb_req_cpu)) {
			pte = proc->p_addr;
			getpcb(pte);
		}

	}
	kdb_eof = 0;
	if (kdb_executing)
		delbp();
	kdb_executing = 0;
	for (;;) {
		if (kdb_errflg) {
			cprintf("%s\n", kdb_errflg);
			exitflg = kdb_errflg;
			kdb_errflg = 0;
		}
		kdb_lp=0; 
		rdc(); 
		kdb_lp--;
		if (kdb_eof) {
			done();
			return;
		} 
		else	exitflg = 0;
		command(0, kdb_lastcom);
		if (kdb_lp && kdb_lastc!='\n')
			error(NOEOR);
	}
	/*NOTREACHED*/
}

done()
{
	endpcs();
	return;
}

L_INT
round(a,b)
register L_INT a, b;
{
	register L_INT w;
	w = (a/b)*b;
	if( a!=w ){ 
		w += b; 
	}
	return(w);
}

/*
 * If there has been an error or a fault, take the error.
 */
chkerr()
{
	if (kdb_errflg)
		error(kdb_errflg);
}

/*
 * An error occurred; save the message for later printing,
 */
error(n)
char *n;
{
	kdb_errflg = n;
	cprintf("%s\n", n);
}

int kdb_tty_flags;
int kdb_tty_state;
extern struct tty cons[];
kdb_getconsole()
{
	/************
	struct tty *tp = &cons[0];
		tp->t_flags |= ECHO;
		kdb_tty_flags = tp->t_flags;
		kdb_tty_state = tp->t_state & ~(TS_ONDELAY|TS_NBIO);
		tp->t_state |= TS_ONDELAY|TS_NBIO;
		************/
}

kdb_putconsole()
{
	/*************
	struct tty *tp = &cons[0];
		tp->t_flags = kdb_tty_flags;
		tp->t_state = kdb_tty_state;
		*****************/
}
