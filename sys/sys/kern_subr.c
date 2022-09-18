#ifndef lint
static	char	*sccsid = "@(#)kern_subr.c	4.4	(ULTRIX)	4/11/91";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1985,86 by			*
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
/************************************************************************
 *
 *			Modification History
 *
 * 11-Apr-91	dlh
 * 	added include of vectors.h
 * 	
 * 	switch_affinity():
 * 		sanity check at beginning and end - cpu_mask must be in process
 * 		affinity
 * 	
 * 		when a vector process is switched, if it's state is VPC_LOAD or
 * 		VPC_LIMBO, then it's context should be saved before it's
 * 		affinity can be changed.
 *
 * 19-Dec-90 -- jaw
 *	real fix for mips affinity routine to handle floating point.
 *
 * 15-Oct-90 -- burns
 *	Fix to switch affinity for mips to fix up saved affinity
 *	when disassiciating a process from a particular cpu due to
 *	prior floating point use.
 *
 * 19-Jun-90 -- jmartin
 *	Fixes for "panic: vrelvm rssize"
 *
 * 14-Nov-89 -- gmm
 *	Save FPU  registers if the process whose affinity is being 
 *	changes is owning the FPU in switch_affinity().
 *
 * 09-Feb-89 -- jaw
 *	Move CPU_TBI state flag to seperate longword.  This was done
 *	because the state flag field can only be written by the cpu
 *	who owns it.
 *
 *  26-Jan-89	jaw
 *	SMP clean up 
 *
 * 10-Oct-88 -- jaw
 *	replace switch_to_master with general routine switch_affinity
 *
 * 18-Jun-88 -- jaw  change to new cpu data format.
 *
 * 18-Mar-86 -- jrs
 *	cleaned up cpu determination and preemption
 *
 * 12-Feb-86 -- jrs
 *	Added tbsync() function for control of translation buffer in mp
 *	systems.
 *
 *	Stephen Reilly, 09-Sept-85
 *	Modified to handle the new 4.3BSD namei code.
 *
 ***********************************************************************/
#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/uio.h"
#include "../h/cpudata.h"
#include "../h/proc.h"
#if defined(__vax)
#include "../machine/vectors.h"
#endif /* __vax */

uiomove(cp, n, rw, uio)
	register caddr_t cp;
	register int n;
	enum uio_rw rw;
	register struct uio *uio;
{
	register struct iovec *iov;
	u_int cnt;
	int error = 0;

	while (n > 0 && uio->uio_resid) {
		iov = uio->uio_iov;
		cnt = iov->iov_len;
		if (cnt == 0) {
			uio->uio_iov++;
			uio->uio_iovcnt--;
			continue;
		}
		if (cnt > n)
			cnt = n;
		switch (uio->uio_segflg) {

 		case UIO_USERSPACE:
 		case UIO_USERISPACE:
			if (rw == UIO_READ)
				error = copyout(cp, iov->iov_base, cnt);
			else
				error = copyin(iov->iov_base, cp, cnt);
			if (error)
				return (error);
			break;

 		case UIO_SYSSPACE:
			if (rw == UIO_READ)
				bcopy((caddr_t)cp, iov->iov_base, cnt);
			else
				bcopy(iov->iov_base, (caddr_t)cp, cnt);
			break;
		}
		iov->iov_base += cnt;
		iov->iov_len -= cnt;
		uio->uio_resid -= cnt;
		uio->uio_offset += cnt;
		cp += cnt;
		n -= cnt;
	}
	return (error);
}

/*
 * Give next character to user as result of read.
 */
ureadc(c, uio)
	register int c;
	register struct uio *uio;
{
	register struct iovec *iov;

again:
	if (uio->uio_iovcnt == 0)
		panic("ureadc");
	iov = uio->uio_iov;
	if (iov->iov_len <= 0 || uio->uio_resid <= 0) {
		uio->uio_iovcnt--;
		uio->uio_iov++;
		goto again;
	}
	switch (uio->uio_segflg) {

 	case UIO_USERSPACE:
		if (subyte(iov->iov_base, c) < 0)
			return (EFAULT);
		break;

 	case UIO_SYSSPACE:
		*iov->iov_base = c;
		break;

 	case UIO_USERISPACE:
		if (suibyte(iov->iov_base, c) < 0)
			return (EFAULT);
		break;
	}
	iov->iov_base++;
	iov->iov_len--;
	uio->uio_resid--;
	uio->uio_offset++;
	return (0);
}

#ifdef notdef
/*
 * Get next character written in by user from uio.
 */
uwritec(uio)
	struct uio *uio;
{
	register struct iovec *iov;
	register int c;

again:
	if (uio->uio_iovcnt <= 0 || uio->uio_resid <= 0)
		panic("uwritec");
	iov = uio->uio_iov;
	if (iov->iov_len == 0) {
		uio->uio_iovcnt--;
		uio->uio_iov++;
		goto again;
	}
	switch (uio->uio_segflg) {

 	case UIO_USERSPACE:
		c = fubyte(iov->iov_base);
		break;

 	case UIO_SYSSPACE:
		c = *iov->iov_base & 0377;
		break;

 	case UIO_USERISPACE:
		c = fuibyte(iov->iov_base);
		break;
	}
	if (c < 0)
		return (-1);
	iov->iov_base++;
	iov->iov_len--;
	uio->uio_resid--;
	uio->uio_offset++;
	return (c & 0377);
}
#endif

/*
 * tbsync() - 
 *
 *	inform other processors that their translation buffer may
 *	be out of date
 */

tbsync()
{
	register int index, self;
	register struct cpudata *pcpu;
	register int s; 

	s = splhigh();
	smp_lock(&lk_rq, LK_RETRY);

	self = CURRENT_CPUDATA->cpu_num;
	for (index = lowcpu; index <= highcpu; index++) {
		pcpu = CPUDATA(index);
		if (pcpu && index != self) {
			pcpu->cpu_tbi_flag = CPU_TBI;
		}
	}
	smp_unlock(&lk_rq);
	(void)splx(s);
}

/*
 * routine switch_affinity makes the current running process
 * run on a cpu in the new affinity mask and returns the old
 * affinity.
 */
switch_affinity(affinity)
	register unsigned long affinity;
{
	register int s;
	register struct cpudata *pcpu;
	register int saveaffinity;
	register struct proc *p;

	pcpu = CURRENT_CPUDATA;
	p = u.u_procp;

	if (! (p->p_affinity & pcpu->cpu_mask) )
		panic ("p_affinity & cpu_mask entrance mismatch");
#ifdef mips
	/* save off fpu context before switching. */
	if(pcpu->cpu_fpowner == p) {
		checkfp(p,0);
	}
#endif mips
#ifdef vax
	if (p->p_vpcontext) {
		/* if process is a vector process, then save it's
		 * vpcontext.  This way switch_affinity() is free to muck
		 * around with the affinity without risking loss of
		 * vpcontext.
		 */
		int	state;
		struct	vpcontext	*v;
		v = p->p_vpcontext;
		state = v->vpc_state;
		if ((state == VPC_LOAD) || (state == VPC_LIMBO)) {
			if (p != pcpu->cpu_vpdata->vpd_proc) {
				panic ("switch_affinity: proc ptr mismatch");
			}
			if (vp_contextsave (p) == VP_FAILURE ) {
				psignal (p, SIGKILL);
			}
			p->p_affinity = v->vpc_affinity & vpmask;
			v->vpc_state = VPC_SAVED;
			pcpu->cpu_vpdata->vpd_proc = NULL;
			set_bit_atomic (pcpu->cpu_num, &vpfree);
		}
	}
#endif vax
	saveaffinity = p->p_affinity;
	p->p_affinity = affinity;
	if ((affinity & pcpu->cpu_mask) == 0) {

#ifdef SMP_DEBUG
		/* NOTE ---- this check should be outside of the affinity 
			     check but cannot be until the hack to synch
			     and mount is fixed.
 		*/
		if (smp_debug){
			sleep_check();
			if (affinity == 0) panic("zero affinity");
		}
#endif

		/* move held lock chain from the per-cpudata structrue to
              	 *  the proc structure. 
		 */
		pcpu->cpu_proc->p_hlock = pcpu->cpu_hlock;
		pcpu->cpu_hlock=0;

		/* aquire the run queue lock.  "swtch" will release it. */
		s = spl6();
		smp_lock(&lk_rq,LK_RETRY);

		pcpu->cpu_proc->p_pri = PWAIT;
		setrq(u.u_procp);	/* put on run queue */
		u.u_ru.ru_nivcsw++;
		swtch();		/* switch processors */


		pcpu = CURRENT_CPUDATA; /* could change CPU's on switch */
#ifdef SMP_DEBUG
		if (smp_debug && pcpu->cpu_hlock) panic("hold lock after switch");
#endif
		/* move held lock chain from proc structure to the 
		 * per-cpudata structure.
  		 */
		pcpu->cpu_hlock = pcpu->cpu_proc->p_hlock;
		pcpu->cpu_proc->p_hlock=0;
		(void) splx(s);
	}
	if (! (p->p_affinity & pcpu->cpu_mask) )
		panic ("p_affinity & cpu_mask exit mismatch");
	return(saveaffinity); /* return the old affinity */
}



