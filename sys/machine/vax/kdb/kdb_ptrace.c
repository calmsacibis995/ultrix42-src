#ifndef lint
static	char	*sccsid = "@(#)kdb_ptrace.c	4.1	ULTRIX	7/2/90";
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
 *	Stolen from sys_process.c for making kernel debugger
 *	MBH	12/22/86
 *
 * -----------------------------------------------------------
 */

#include "../machine/reg.h"
#include "../machine/psl.h"
#include "../machine/pte.h"

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/gnode.h"
#include "../h/text.h"
#include "../h/vm.h"
#include "../h/buf.h"
#include "../h/acct.h"

/*
 * Priority for tracing
 */
#define	IPCPRI	PZERO

struct {
	int	ip_lock;
	int	ip_req;
	int	*ip_addr;
	int	ip_data;
} 
kipc;

extern struct user *uptr;
int kernel_trace;
/*
 * sys-trace system call.
 */
kdb_ptrace(req, pid, addr, data)
int *addr;
int pid;	/* ignored */
{
	if (req <= 0) {
		cprintf("kdb shouldn't ever have a req of < 0\n");
		return(0);
	}

	if (kipc.ip_lock) {
		cprintf("kipc.ip_lock is held:  can't trace\n");
		return(0);
	}
	kipc.ip_lock = pid;
	kipc.ip_data = data;
	kipc.ip_addr = addr;
	kipc.ip_req = req;

	kernel_trace = 1;

	/* now do kdb_procxmt code here */
	kdb_procxmt();
	kipc.ip_lock = 0;
	return(kipc.ip_data);
}

#ifdef vax
#define	NIPCREG 16
int kipcreg[NIPCREG] =
{
	R0,R1,R2,R3,R4,R5,R6,R7,R8,R9,R10,R11,AP,FP,SP,PC};
#endif

#define	PHYSOFF(p, o) \
((physadr)(p)+((o)/sizeof(((physadr)0)->r[0])))

extern int single_stepping;
extern int *kdb_psl_ptr;
/*
 * do procxmt half of a ptrace request
 */
kdb_procxmt()
{
	register int i;
	register *p;
	register struct text *xp;
	int op1, op2;

	i = kipc.ip_req;
	kipc.ip_req = 0;
	switch (i) {

		/* read user I and D */
	case 1:
	case 2:
		/* is there an equivalent of useracc for kernel? 
				 *	if (!useracc((char *)kipc.ip_addr, 4, B_READ))
				 *		goto error;
				 */
		kipc.ip_data = (short)*kipc.ip_addr;
		break;

		/* read u */
	case 3:
		i = (int)kipc.ip_addr;
		if (i<0 || i >= ctob(UPAGES))
			goto error;
		kipc.ip_data = *(int *)((char *)uptr+i);
		break;

		/* write user I and D */
		/* Must set up to allow writing */
	case 4: 
	case 5:

		op1 = kdb_chgprot((char *)kipc.ip_addr, PG_KW);
		op2 = kdb_chgprot((char *)kipc.ip_addr+(sizeof(int)-1), PG_KW);
		*(char *)kipc.ip_addr = kipc.ip_data;
		kdb_chgprot((char *)kipc.ip_addr, op1);
		kdb_chgprot((char *)kipc.ip_addr+(sizeof(int)-1), op2);
		break;

		/* write u */
	case 6:
		i = (int)kipc.ip_addr;
		p = *(int *)((char *)uptr+i);
		for (i=0; i<NIPCREG; i++)
			if (p == &uptr->u_ar0[kipcreg[i]])
				goto ok;
		if (p == &uptr->u_ar0[PS]) {
			kipc.ip_data |= PSL_USERSET;
			kipc.ip_data &=  ~PSL_USERCLR;
			goto ok;
		}
		goto error;

ok:
		op1 = kdb_chgprot((char *)p, PG_KW);
		op2 = kdb_chgprot((char *)p+(sizeof(int)-1), PG_KW);
		*p = kipc.ip_data;
		kdb_chgprot((char *)p, op1);
		kdb_chgprot((char *)p+(sizeof(int)-1), op2);
		break;

		/* set signal and continue */
		/* one version causes a trace-trap */
	case 9:
	case 7:
		/* need to set and return */
		if (i == 9) {
			single_stepping = 1;
			*kdb_psl_ptr |= PSL_T;
		} 
		else	single_stepping = 0;
		break;

		/* force exit */
	case 8:
		cprintf("forcing exit from kdb?\n");

	default:
error:
		kipc.ip_req = -1;
		return(0);
	}
	return (1);
}

kdb_priv()
{
	u.u_uid = 0;
}
