#ifndef lint
static	char	*sccsid = "@(#)errlog.c	4.1	(ULTRIX)	7/2/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1986 - 1989 by			*
 *		Digital Equipment Corporation, Maynard, MA		*
 *			All rights reserved.				*
 *									*
 *   This software is furnished under a license and may be used and	*
 *   copied  only  in accordance with the terms of such license and	*
 *   with the  inclusion  of  the  above  copyright  notice.   This	*
 *   software  or  any  other copies thereof may not be provided or	*
 *   otherwise made available to any other person.  No title to and	*
 *   ownership of the software is hereby transferred.			*
 *                                                                      *
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/
/*
 *   File name: errlog.c
 *
 *   Source file description: 
 *   	This file contains vax dependent error logging routines.
 *
 *   Functions:
 *	logmck			log mackine check
 *	logstray		Log a stray interrupt
 *	logsbi			log sbi error 
 *	nexusinfo		load nexus related info for sbi logging
 *
 *   Modification history:
 *
 *      27-Nov-89       Paul Grist
 *              Modified logmck() by adding the frame_type argument. The
 *              routine was incorrectly passing the unique cpu number to
 *              LSUBID() instead of the frame-type. This was causing
 *              several types of machines to log an incorrect frame type
 *              which caused uerf to incorrectly format, or be unable to
 *              format the machine check packets.
 *
 *	24-May-89	darrell
 *		Removed the v_ prefix from all cpusw fields, removed cpup
 *		from any arguments being passed in function args.  cpup is
 *		now defined globally -- as part of the new cpusw.
 *
 *	24-May-89	darrell
 *		Changed the #include for cpuconf.h to find it in it's new
 *		home --	sys/machine/common/cpuconf.h
 *
 *	Jul 27 87	jaw
 *		make stray interrupts only log 1 per hour for
 *		vector zero.
 *
 *	Jul 28 86	bjg
 *		Add cpu_subtype to subid.type for machine check logging
 *
 *	Jun 09 86	bjg
 *		Remove logstart call. 
 *
 *	Apr 02 86	bjg
 *		Change loop in nexusinfo to correctly interpret sbi_there
 *		bits.
 *
 *	Mar 12 86	bjg
 *		Initial Creation
 * 
 *	
 *
 */
#include "../h/types.h"
#include "../h/time.h"
#include "../h/kernel.h"
#include "../h/param.h"
#include "../vax/cpu.h"
#include "../../machine/common/cpuconf.h"
#include "../h/errlog.h"
#include "../vax/nexus.h"
extern long sbi_there;	/* defined in autoconf.c */
extern struct cpusw *cpup;	/* defined in machdep.c */


/*
 * Function: logmck(mckfp,frame_type,cpunum,recover)
 *
 * Function description: Log a machine check frame message; called by 
 *	the cpusw type machinecheck() routine.
 *
 * Arguments: int *mckfp	machine check frame pointer on stack
 *            int frame_type    machine check frame type (in errlog.h)
 *	      int cpunum 	cpu number
 *	      int recover	recover if 1
 * Return value: None
 *
 * Side effects: None
 *
 */
logmck(mckfp,frame_type,cpunum,recover)
int *mckfp;
int frame_type;   /* frame types defined in errlog.h */
int cpunum;
int recover;
{
	int bcnt;
	int i;
	int *framep;
	struct el_rec *elrp;
	struct el_mck *elmckp;

	bcnt = *mckfp + 12;
	elrp = ealloc(bcnt, recover ? EL_PRIHIGH : EL_PRISEVERE);
	if (elrp != NULL) {
	    LSUBID(elrp,ELCT_MCK,frame_type,cpu_subtype,cpunum,EL_UNDEF,*(mckfp+1));
	    elmckp = &elrp->el_body.elmck;
	    framep = &elmckp->elmck_frame.el8600mcf.mc8600_bytcnt;
	    for ( i = 0; i < (bcnt >> 2); i++)
	        *framep++ = *mckfp++;
	    EVALID(elrp);
	}
}

/*
 * Function: logstray(type, el_ipl, el_vec)
 *
 * Function description:  Log a stray interrupt (SCB or UBA) 
 *
 * Arguments: type - type of stray (SCB or UBA)
 *	      el_ipl - ipl level
 *	      el_vec - interrupt vector
 *
 * Return value: None
 *
 * Side effects: None
 *
 */
unsigned int stray_time=0;

logstray(type, el_ipl, el_vec)
long type;
long el_ipl;
long el_vec;
{
	struct el_rec *elrp;
	
	/* if zero vector, only log one error every hour */
	if (el_vec == 0) {
		if (((unsigned)time.tv_sec) - stray_time < 3600)
				return;

		stray_time = time.tv_sec;
	}
	elrp = ealloc(sizeof(struct el_strayintr), EL_PRILOW);
	if (elrp != EL_FULL) {
	    LSUBID(elrp,ELCT_SINT,type,EL_UNDEF,EL_UNDEF,EL_UNDEF,EL_UNDEF);
	    elrp->el_body.elstrayintr.stray_ipl = (u_char)el_ipl;
	    elrp->el_body.elstrayintr.stray_vec = (short)el_vec;
	    EVALID(elrp);
	}
}
/*
 * Function: logsbi(sbi_num, sbi_type, pc_psl)
 *
 * Function description: Log an sbi error; called from locore.s upon
 *	SBI interrupt
 *
 * Arguments: long sbi_num - number of the sbi
 *            long sbi_type - longword containing sbi type; pushed on stack
 *            long pc_psl - addrs of long: ps/psl; pushed on stack
 *
 * Return value: None
 *
 * Side effects: None
 *
 */

logsbi(sbi_num,sbi_type,pc_psl)
long sbi_num;
long sbi_type;
long *pc_psl;
{

	if ((*cpup->harderr_intr)(sbi_num, sbi_type, pc_psl) < 0 )
		panic("No harderr_intr handler configured\n");
}
/*
 * Function: nexusinfo(elrp, sbi_num, cpup, csrptr)
 *
 * Function description: 
 *
 * Arguments:   
 *		
 * Return value: None
 *
 * Side effects: None
 *
 */

nexusinfo(elrp, sbi_num, csrptr)
struct el_rec *elrp;
long sbi_num;
int *csrptr;
{

	register char  *nxv;
	union nexcsr lnexcsr;
	int nexnum;

	nxv = (char *)nexus + (sbi_num * cpup->pc_nnexus * cpup->pc_nexsize);
	for( nexnum= 0; nexnum < (cpup->pc_nnexus); 
		nexnum++, nxv += cpup->pc_nexsize) {
		if (sbi_there & ( 1<< (sbi_num * cpup->pc_nnexus) + nexnum)) {
			lnexcsr = ((struct nexus *)nxv)->nexcsr;
			*csrptr++ = lnexcsr.nex_csr;
		}
		else {
			*csrptr++ = 0;
		}
	}
}

