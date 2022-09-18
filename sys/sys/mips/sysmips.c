#ifndef lint
char *sccsid="@(#)sysmips.c	4.1	(ULTRIX)	7/2/90";
#endif lint
/************************************************************************
 *									*
 *			Copyright (c) 1988 by				*
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
#ifdef mips
/*
 * MIPS specific syscall interface.
 *
 * To Use:
 * 
 * Get Yourself a Number from the list, if you want to write any
 * code which will reside in sysmips(), use a number from 0x0 to 0xff.
 * If you just need a function vector, use a number above 0x100.
 *
 * Slap together a routine that takes up to four arguments.  Do not use
 * typical system call interface, you will be passed up to four arguments
 * using the normal procedure interface.  Remember also, you are not
 * to touch u.u_error in your function. You should return an error
 * value, and sysmips() will deal with u.u_error.
 *
 * If you will not be using the simple function vector mechanism, add your
 * number and appropriate code or call to your routine to the
 * switch statement in sysmips().
 *
 * Have a homebrew :-)
 */

/*
 * Revision History:
 *
 * 18 Apr 90 jaa
 *	change indirect syscall to re-arrange u_ap for cacheflush/cachectl
 *
 * 11-Dec-1989 jas
 *      atomic_op changes - check for unaligned addresses.
 *
 * 13-Oct-1989 gmm
 *	smp changes - made fpowner mp safe
 *
 * 10-Oct-1989 Joe Szczypek
 *             Added atomic_op().
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/user.h"
#include "../h/sysmips.h"
#include "../h/utsname.h"
#include "../h/proc.h"
#include "../h/buf.h"
#include "../h/lock.h"


#include "../machine/cpu.h"
#include "../machine/cpu_board.h"
#include "../machine/fpu.h"
#include "../machine/hwconf.h"
#include "../machine/pte.h"

extern unsigned fptype_word;

int mipskopt();
int mipshwconf();
int mips_getrusage();
int mips_wait();
int cacheflush();
int cachectl();

int (*func_vector[])() = {
	mipskopt,
	mipshwconf,
	mips_getrusage,
	mips_wait,
	cacheflush,
	cachectl
	};

#ifdef oldmips
struct utsname utsname = {
	"amnesia",
	"amnesia",
	R_2_1,
	V_UMIPSBSD,
	M_MIPS,
	MT_M500,
	BR_43_BSD
	};
#endif oldmips

sysmips()
{
	register struct a {
		int vector;
		int arg1,arg2,arg3,arg4;
	} *uap;

	uap = (struct a *)u.u_ap;

	if (uap->vector < 0 || uap->vector > MIPS_VECTOR_SIZE)
		{
		u.u_error = EINVAL;
		return;
		}

	if (uap->vector >= MIPS_VECTOR_DIVIDER)
		{

		int fnct;

		if ((fnct = (uap->vector - MIPS_VECTOR_DIVIDER)) >
			(sizeof(func_vector) / sizeof(func_vector[0])))
			{
			u.u_error = EINVAL;
			return;
			}

		if(fnct == 4 || fnct == 5) {
			uap->vector = uap->arg1;
			uap->arg1 = uap->arg2;
			uap->arg2 = uap->arg3;
			uap->arg3 = uap->arg4 = 0;
			u.u_error = (*func_vector[fnct])();
		} else
			u.u_error = (*func_vector[fnct])
				(uap->arg1,uap->arg2,uap->arg3,uap->arg4);
		} /* if */
	else
		switch (uap->vector)
			{
#ifdef oldmips
			case MIPS_UNAME:
				switch (hwconf.cpubd_type) {
					case BRDTYPE_R2300:
						strcpy(utsname.m_type, MT_M500);
						break;
					case BRDTYPE_R2600:
						strcpy(utsname.m_type, MT_M800);
						break;
					case BRDTYPE_R2800:
						strcpy(utsname.m_type,MT_M1000);
						break;
				}
				strncpy(utsname.sysname, hostname, SYS_NMLN);
				strncpy(utsname.nodename, hostname, SYS_NMLN);
				u.u_error = copyout(&utsname, uap->arg1,
					sizeof(utsname));
				return;
#endif oldmips

			case MIPS_FPSIGINTR:
				u.u_procp->p_fp = uap->arg1;
				u.u_error = 0; /* should I do this? */
				return;

			case MIPS_FPU:
				/*
				 * You must be super-user to do this.
				 * If the argument is non-zero turn the fpu
				 * back on. Else turn it off.
				 */
				if (!suser())
					return;
				if(uap->arg1){
					fptype_word =
						hwconf.fpu_processor.ri_uint &
						IRR_IMP_MASK;
				}
				else{
					if(CURRENT_CPUDATA->cpu_fpowner != 0)
						checkfp(CURRENT_CPUDATA->cpu_fpowner, 0);

					fptype_word = 0;
				}
				u.u_error = 0; /* should I do this? */
				return;

			case MIPS_FIXADE:
				if(uap->arg1)
					u.u_procp->p_mips_flag |= SFIXADE;
				else
					u.u_procp->p_mips_flag &= ~SFIXADE;
				u.u_error = 0; /* should I do this? */
				return;

			default:
				u.u_error = EINVAL;
				return;
			} /* switch vector*/
	return;
} /* sysmips() */

/*
 * Procedure atomic_op()
 *
 * Purpose:
 * 
 * To perform an atomic "interlocked" operation on the location specified by arg. 
 */

atomic_op()
{
	struct a {
		int opcode;   /* if set or clear */
		int *address; /* word to lock */
	} *uap = (struct a*)u.u_ap;

	if (!useracc(uap->address, sizeof(int), PROT_UW)) { 
	    	u.u_error = EACCES;
		return;
	}
	if ((int)uap->address % (sizeof (int))) {
		u.u_error = EALIGN;        /* error if not word boundary */
		return;
	}
	switch(uap->opcode) {
	      case ATOMIC_SET:
			if(!set_bit_atomic(ATOMIC_LOCKBIT,uap->address))
				u.u_error = EBUSY;        /* error if already set */
			break;
	      case ATOMIC_CLEAR:
			clear_bit_atomic(ATOMIC_LOCKBIT,uap->address);
			break;
	      default:
			u.u_error = EINVAL;   /* error if illegal atomic op */
	}

}

#endif mips
