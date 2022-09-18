#ifndef lint
static char *sccsid = "@(#)sys_sysinfo.c	4.8	(ULTRIX)	2/12/91";
#endif

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

/*
 *
 *   Modification history:
 *
 * 9-Oct-90	jas
 *	added console support code - 2 new operations for getsysinfo().
 *
 * 4-Sep-90	dlh
 *	added vector processor support code - new operation for getsysinfo()
 *
 * 13-Aug-90	sekhar
 *	modified GSI_MMAPALIGN for mmap support on both vax and mips.
 *
 * 09-Jul-90 Fred L. Templin
 *	Added GSI_BOOTTYPE
 *
 * 22-Jun-90 sekhar
 *	Added GSI_MMAPALIGN for mmap support(mips only).
 *
 * 15-Dec-89 Alan Frechette
 *	Added GSI_WSD_TYPE and GSI_WSD_UNITS.
 *
 * 02-Nov-89 Matt Thomas
 *	Add hooks for getting DNA UIDs.
 *
 * 04-Aug-89 jaw
 *	lmf_set_login was called with procqs locked.  That routine
 * 	does a kmalloc call with the wait option.  This causes a panic
 *	since you can't hold a spin lock if you could context switch.  Since
 *	lmf_set_login does not need procqs protected, I moved the unlock
 *	before the call.
 *
 * 25-Jul-89 Giles Atkinson
 *	Add call to lmf_set_login in SSI_LOGIN.
 *
 * 20-Jul-89 jaw
 *	SSI_LOGIN fuction locking procqs at wrong IPL.
 *
 * 12 Jun 89 -- dws
 *	Added initialization of terminal session ID in SSI_LOGIN
 *
 * 01 Jun 89 -- Giles Atkinson
 *    Add SSI_LOGIN function
 *
 * 08 May 89 -- Giles Atkinson
 *    Add LMF functions SSI/GSI_LMF
 *
 * 06 Feb 89 -- prs
 *	Added root check to individual functions in setsysinfo().
 *
 * 22 Mar 88 -- map
 *	Add getsysinfo() functionality for GSI_PROG_ENV, GSI_MAX_UPROCS
 *	and GSI_TTYP.
 *
 * 09 Mar 88 -- chet
 *	Created this file.
 *
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/user.h"
#include "../h/tty.h"
#include "../h/proc.h"
#include "../h/sysinfo.h"
#include "../h/smp_lock.h"
#include "../sas/mop.h"
#include "../h/exec.h"
#ifdef vax
#include "../machine/vectors.h"
#endif

/*
 *	Routines for generic retrieval and storage of kernel information.
 *	These routines are meant to serve as a substitute for the abuse of
 *	ioctl(), fcntl(), and reading/writing /dev/mem.
 *
 *      These system calls are extensible.  In order to  prevent  them
 *      from becoming dumping grounds for inappropriate operations, new
 *      operations should be proposed and reviewed via the design review
 *	process.
 */


#ifdef mips
extern  int     k_puac;
extern  char	consmagic[4];
extern	char	bootctlr[2];
#else
char	consmagic[4];
char	bootctlr[2];
#endif 

extern	int	ws_display_type;
extern	int	ws_display_units;
extern	int	maxuprc;
extern 	struct netblk *netblk_ptr;
extern	char	boottype[];
char *bootdev;			/* a generic place to hold a pointer
				   to the name of the bootdevice */

int (*dna_uid_g_create_rtn)();	/* a place to store the UID creation
				   routine (which may or may not be
				   present on all systems) */


/*
 *	Retrieve system information
 */
int
getsysinfo()
{
	register struct a {
		unsigned	 op;
		char		*buffer;
		unsigned	 nbytes;
		int		*start;
		char		*arg;
		unsigned	 flag;
	} *uap = (struct a *) u.u_ap;

	int i;

	switch (uap->op) {

#ifdef mips
	      case GSI_UACPROC:	
		if( uap->nbytes < sizeof(u.u_procp->p_puac) ) 
		    u.u_error = EINVAL;
		else {
		    u.u_error = copyout((caddr_t)&u.u_procp->p_puac,
			(caddr_t)uap->buffer, sizeof(u.u_procp->p_puac) );
		    u.u_r.r_val1 = 1;
		}
		break;

	      case GSI_UACPARNT:	
		if( uap->nbytes < sizeof(u.u_procp->p_pptr->p_puac) ) 
		    u.u_error = EINVAL;
		else {
		    u.u_error = copyout((caddr_t)&u.u_procp->p_pptr->p_puac,
			(caddr_t)uap->buffer, sizeof(u.u_procp->p_puac) );
		    u.u_r.r_val1 = 1;
		}
		break;

	      case GSI_UACSYS:
		if( uap->nbytes < sizeof(k_puac) ) 
		    u.u_error = EINVAL;
		else {
		    u.u_error = copyout((caddr_t)&k_puac,
			(caddr_t)uap->buffer, sizeof(k_puac) );
		    u.u_r.r_val1 = 1;
		}
		break;
#endif mips

	      case GSI_PROG_ENV:	/* get programming environment */
		if( uap->nbytes < sizeof(u.u_procp->p_progenv) ) 
		    u.u_error = EINVAL;
		else {
		    u.u_error = copyout((caddr_t)&u.u_procp->p_progenv,
			(caddr_t)uap->buffer, sizeof(u.u_procp->p_progenv) );
		    u.u_r.r_val1 = 1;
		}
		break;

	      case GSI_MAX_UPROCS:	/* get max number of procs per uid */
		if( uap->nbytes < sizeof(maxuprc) ) 
		    u.u_error = EINVAL;
		else {
		    u.u_error = copyout((caddr_t)&maxuprc,
			(caddr_t)uap->buffer, sizeof(maxuprc) );
		    u.u_r.r_val1 = 1;
		}
		break;

	      case GSI_TTYP:		/* get controlling tty dev */
		if( uap->nbytes < sizeof(dev_t) ) 
		    u.u_error = EINVAL;
		else {
		 if( u.u_procp->p_ttyp ) {
		   u.u_error = copyout((caddr_t)&u.u_procp->p_ttyp->t_dev,
			(caddr_t)uap->buffer, 
			sizeof(dev_t) );
		   u.u_r.r_val1 = 1;
		 }
		 else
		   u.u_r.r_val1 = 0;
		}
		break;

	      case GSI_NETBLK:		/* get contents of netblk structure */
		if( uap->nbytes < sizeof(struct netblk) ) 
		    u.u_error = EINVAL;
		else {
		    u.u_error = copyout((caddr_t)netblk_ptr,
			(caddr_t)uap->buffer, sizeof(struct netblk) );
		    u.u_r.r_val1 = 1;
		}
		break;

#define BOOTDEVLEN 80

	      case GSI_BOOTDEV:	
		if( uap->nbytes < BOOTDEVLEN ) 
		    u.u_error = EINVAL;
		else {
		    u.u_error = copyout((caddr_t)bootdev,
			(caddr_t)uap->buffer, BOOTDEVLEN );
		    u.u_r.r_val1 = 1;
		}
		break;


	      case GSI_BOOTCTLR:	
		if( uap->nbytes < sizeof(bootctlr) ) 
		    u.u_error = EINVAL;
		else {
		    u.u_error = copyout((caddr_t)bootctlr,
			(caddr_t)uap->buffer, sizeof(bootctlr) );
		    u.u_r.r_val1 = 1;
		}
		break;

	      case GSI_CONSTYPE:	
		if( uap->nbytes < sizeof(consmagic) ) 
		    u.u_error = EINVAL;
		else {
		    u.u_error = copyout((caddr_t)consmagic,
			(caddr_t)uap->buffer, sizeof(consmagic) );
		    u.u_r.r_val1 = 1;
		}
		break;

#define	BOOTTYPELEN 10

	      case GSI_BOOTTYPE:
		if( uap->nbytes < BOOTTYPELEN )
		    u.u_error = EINVAL;
		else {
		    u.u_error = copyout((caddr_t)boottype,
			(caddr_t)uap->buffer, BOOTTYPELEN );
		    u.u_r.r_val1 = 1;
		}
		break;

             case GSI_LMF:             /* License management functions */
		getlminfo();
		break;

	      case GSI_WSD_TYPE:	
		if( uap->nbytes < sizeof(ws_display_type) )
		    u.u_error = EINVAL;
		else {
		    u.u_error = copyout((caddr_t)&ws_display_type,
			(caddr_t)uap->buffer, sizeof(ws_display_type) );
		    u.u_r.r_val1 = 1;
		}
		break;

	      case GSI_WSD_UNITS:	
		if( uap->nbytes < sizeof(ws_display_units) )
		    u.u_error = EINVAL;
		else {
		    u.u_error = copyout((caddr_t)&ws_display_units,
			(caddr_t)uap->buffer, sizeof(ws_display_units) );
		    u.u_r.r_val1 = 1;
		}
		break;

	      case GSI_MMAPALIGN:	
		if( uap->nbytes < sizeof(int) )
		    u.u_error = EINVAL;
		else {
		    i = get_mmap_alignment();
		    u.u_error = copyout((caddr_t)&i,
			(caddr_t)uap->buffer, sizeof(int) );
		    u.u_r.r_val1 = 1;
		}
		break;

	      case GSI_PHYSMEM: {
#if PGSHIFT < 10
		u.u_r.r_val1 = physmem >> (10 - PGSHIFT);
#else
		u.u_r.r_val1 = physmem << (PGSHIFT - 10);
#endif
		break;
	      }

	      case GSI_DNAUID: {
		if ( uap->nbytes < 16 )
		    u.u_error = EINVAL;
		else if (dna_uid_g_create_rtn != 0) {
		    u_char auid[16], ats[16];
		    u.u_r.r_val1 = 1;
		    u.u_error = (*dna_uid_g_create_rtn)(auid, ats);
		    if (u.u_error)
			break;
		    u.u_error = copyout( (caddr_t) auid, 
			(caddr_t) uap->buffer, 16 );
		    if (u.u_error || uap->arg == NULL)
			break;
		    u.u_error = copyout( (caddr_t) ats,
			(caddr_t) uap->arg, 16 );
		} else u.u_error = EOPNOTSUPP;
		break;
	      }


#ifdef vax
	      case GSI_VPTOTAL:	/* get # of vector processors in the system */
		if( uap->nbytes < sizeof(vptotal) ) 
		    u.u_error = EINVAL;
		else {
		    u.u_error = copyout((caddr_t)&vptotal,
			(caddr_t)uap->buffer, sizeof(vptotal) );
		    u.u_r.r_val1 = 1;
		}
		break;
#endif vax
	      default:
		u.u_error = EINVAL;
		break;
	}

}





/*
 *	Set/Store system information
 */

#define MAX_NVPAIRS_NAMES 100	/* max number of name-value pairs per call */

extern int	nfsportmon;	/* NFS secure port monitoring? */
extern int	kernel_locking; /* kernel style file locking? */

int
setsysinfo()
{

	register struct a {
		unsigned	op;
		caddr_t		buffer;
		unsigned	nbytes;
		caddr_t		arg;
		unsigned	flag;
	} *uap = (struct a *) u.u_ap;

	register int		i;
	register caddr_t	p;
	register struct proc *up = u.u_procp;
	int			name;
	int			int_val;

/*
 * "normal" indentation not followed here to leave a meaningful amount
 * of space per line for the action routines
 */

switch (uap->op) {

      /*
       * Name-Value pairs form of input
       *
       * An action routine for each `name' is found in the switch statement
       * below. It is the responsibility of this action routine to validate
       * the invoker, its value (setting u.u_error if an error exists) and
       * also to advance the argument buffer pointer `p' so that it is ready
       * for the next `name' copyin in the for-loop below.
       */

      case SSI_NVPAIRS:

	if (uap->nbytes < 1 || uap->nbytes > MAX_NVPAIRS_NAMES) {
		/* nbytes = number of pairs, let's not get crazy here */
		u.u_error = EINVAL;
		break;
	}

	p = uap->buffer;

	for (i = 0; i < uap->nbytes; ++i) {
		if (u.u_error = copyin(p, &name, sizeof(int)))
			break;
		p += sizeof(int);

		switch (name) {

#ifdef mips
		      case SSIN_UACSYS:
			if(!suser()) {
				u.u_error = EPERM;
				break;
			}
			if(u.u_error = copyin(p, &int_val, sizeof(int)))
				break;
			p += sizeof(int);
			if(int_val == UAC_MSGON || int_val == UAC_MSGOFF) 
				k_puac = int_val;
			else
				u.u_error = EINVAL;
			break;

		      case SSIN_UACPARNT:
			if(u.u_error = copyin(p, &int_val, sizeof(int)))
				break;
			p += sizeof(int);
		        if(int_val != UAC_MSGON && int_val != UAC_MSGOFF) {
				u.u_error = EINVAL;
				break;
			}
			if(up->p_pptr->p_pid != 1) 
				up->p_pptr->p_puac = int_val;
			else
				u.u_error = EPERM;
			break;

		      case SSIN_UACPROC:
			if(u.u_error = copyin(p, &int_val, sizeof(int)))
				break;
			p += sizeof(int);
			if(int_val == UAC_MSGON || int_val == UAC_MSGOFF) 
				up->p_puac = int_val;
			else
				u.u_error = EINVAL;
			break;
#endif mips

			case SSIN_PROG_ENV:
			if(u.u_error = copyin(p, &int_val, sizeof(int)))
				break;
			p += sizeof(int);
			/*
			 * set u_structure flags if in POSIX mode/SYSV mode
			 */
			if(int_val != A_BSD && int_val != A_POSIX && 
			   int_val != A_SYSV ) {
				u.u_error = EINVAL;
				break;
			}
			if((up->p_progenv = int_val) == A_POSIX || 
			up->p_progenv == A_SYSV) 
				/* Set all bits ON so will interrupt */
				u.u_sigintr = ~0;
			break;

		      /*
		       * Turn NFS secure port monitoring on/off
		       */
		      case SSIN_NFSPORTMON:
			if (u.u_uid != 0) {
				u.u_error = EPERM;
				break;
			}
			if (u.u_error = copyin(p, &int_val, sizeof(int)))
				break;
			if (int_val < 0 || int_val > 1) {
				u.u_error = EINVAL;
				break;
			}
			p += sizeof(int);
			nfsportmon = int_val;
		        break;

		      /*
		       * Turn kernel style file locking on/off
		       */
		      case SSIN_NFSSETLOCK:
			if (u.u_uid != 0) {
				u.u_error = EPERM;
				break;
			}
			if (u.u_error = copyin(p, &int_val, sizeof(int)))
				break;
			if (int_val < 0 || int_val > 1) {
				u.u_error = EINVAL;
				break;
			}
			p += sizeof(int);
			kernel_locking = !int_val;
			break;

		      default:
			u.u_error = EINVAL;
			break;
		}
		if (u.u_error)
			return;
	}
	break;

      case SSI_ZERO_STRUCT:	/* zero a structure */
	u.u_error = EOPNOTSUPP;
	break;

      case SSI_SET_STRUCT:	/* set a structure to supplied */
                                /* values */
	u.u_error = EOPNOTSUPP;
	break;

      case SSI_LMF:		/* License management functions */
	setlminfo();
	break;

      case SSI_LOGIN:		/* Identify caller as a login process,
				 * unless it already has a login ancestor. 
				 */
	if (!suser())
		break;
        if (up->p_type & SLOGIN || up->p_pid == 1)
		break;

	(void)spl5();
        smp_lock(&lk_procqs, LK_RETRY);
	while (!(up->p_type & SLOGIN) && up->p_pid != 1 && (up = up->p_pptr))
		;
	if (!up || up->p_pid == 1) {
		u.u_procp->p_type |= SLOGIN;
		if (u.u_procp->p_ttyp != NULL)
			u.u_procp->p_ttyp->t_sid = u.u_procp->p_pid;
		smp_unlock(&lk_procqs);
		(void) spl0();
		set_lmf_login();
	} else {
		u.u_error = EINVAL;
		smp_unlock(&lk_procqs);
		(void) spl0();
	}
	break;

      default:
	u.u_error = EINVAL;
	break;
}

}
