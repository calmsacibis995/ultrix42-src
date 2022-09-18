#ifndef lint
static char *sccsid = "@(#)ka8600.c	4.1	ULTRIX	7/2/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1984,85,86 by			*
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

/***********************************************************************
 *
 * Modification History:
 *
 * 27-Nov-89    Paul Grist
 *      added frame_type argument to logmck() call.
 *
 * 31-May-89	darrell
 *	Changed the call to tocons to call cons_putc.
 *
 * 24-May-89	darrell
 *	Changed the #include for cpuconf.h to find it in it's new home --
 *	sys/machine/common/cpuconf.h
 *
 * 24-May-89	darrell
 *	Removed the v_ prefix from all cpusw fields, removed cpup from any
 *	arguments being passed in function args.  cpup is now defined
 *	globally -- as part of the new cpusw.
 *
 * 13-Jun-86    bjg
 *	Initialize genp in genserver(); also change  treshhold checking 
 *
 * 11-Jun-86    bjg
 *	add sbi# to cprintf string
 *
 * 16-Apr-86	darrell
 *	moved structure and header file type things into ka8600.h
 *
 * 12-Apr-86    bjg
 *	Check that ioav is mapped before accessing (ka8600logsbi).
 *
 * 15-Mar-86	Darrell Dunnuck
 *	Moved ka8600 specific parts of configure() here into ka8600conf.
 *
 * 12-Mar-86 -- bjg
 *	moved sbi logging routines to ka8600.c from kern_errlog.c
 *
 * 05-Mar-86 -- pmk
 *	added arg recover to logmck and replaced display with cprintf
 *
 * 18-Mar-86 -- jaw  add routines to cpu switch for nexus/unibus addreses
 *		     also got rid of some globals like nexnum.
 *
 * 12-Feb-86	Darrell Dunnuck
 *	Removed the routines memerr, memenable, setcache, and tocons
 *	from machdep.c and put them here for this processor type.
 *	Added a new routine cachenbl.
 *
 * 12-Dec-85 	Darrell Dunnuck
 *	Created this file to as part of machdep restructuring.
 *
 **********************************************************************/

#include "../../machine/common/cpuconf.h"
#include "../h/types.h"
#include "../h/time.h"
#include "../h/param.h"
#include "../machine/cons.h"
#include "../h/errlog.h"
#include "../machine/mtpr.h"
#include "../machine/cpu.h"
#include "../machine/mem.h"
#include "../machine/pte.h"
#include "../machine/nexus.h"
#include "../io/uba/ubareg.h"
#include "../machine/ioa.h"
#include "../machine/ka8600.h"

extern long sbi_there;	/* defined in autoconf.c */
extern int nexusinfo();	/* defined in errlog.c */
extern struct cpusw *cpup;	/* pointer to cpusw entry */

int	mem_err_report; 	/* state of memory error reporting */
int	memerrs = 0;		/* number of times memerr has been entered */

struct ibox_errcnt ibox_errcnt;
struct fbox_errcnt fbox_errcnt;
struct ebox_errcnt ebox_errcnt;
struct mbox_errcnt mbox_errcnt;
struct tb_errcnt tb_errcnt;
struct csh_errcnt csh_errcnt;
struct mbox_1d_errcnt mbox_1d_errcnt;
struct generic_errcnt generic_errcnt;

ka8600machcheck (cmcf)
caddr_t cmcf;
{
	register struct mc8600frame *mcf = (struct mc8600frame *) cmcf;
	int dbl = 0;

	/* 
	 * Due to write back cache, cache SHOULD NOT be disabled
	 * on an 8600.
	 */

	if(mcf->mc8600_ebcs & M_MBOX_FE)
		mcf->mc8600_ehm_sts |= C_MBOX;
	if(mcf->mc8600_ehm_sts & M_FBOX)
		if ((mcf->mc8600_ehm_sts & 0xf) == 0)
			mcf->mc8600_ehm_sts |= C_FBOX;
		else
			dbl++;
	if(mcf->mc8600_ebcs & EBOX_ERR_MASK)
		if ((mcf->mc8600_ehm_sts & 0xf) == 0)
			if(mcf->mc8600_ebcs & M_EDP_PE)
				mcf->mc8600_ehm_sts |= C_MBOX;
			else
				mcf->mc8600_ehm_sts |= C_EBOX;
		else
			dbl++;
	if(mcf->mc8600_ehm_sts & M_IBOX_ERR)
		if ((mcf->mc8600_ehm_sts & 0xf) == 0)
			mcf->mc8600_ehm_sts |= C_IBOX;
		else
			dbl++;
	if((mcf->mc8600_cslint & M_MBOX_1D) == M_MBOX_1D)
		if ((mcf->mc8600_ehm_sts & 0xf) == 0)
			mcf->mc8600_ehm_sts |= C_MBOX_1D;
		else
			dbl++;

	if (dbl) {
		badmchk(mcf);
		panic("Too many machine check errors to recover... \n");
	}
	else {
		switch(mcf->mc8600_ehm_sts & C_MCHK_MASK) {
			case C_FBOX:
				fboxserv(mcf);
				break;
			case C_EBOX:
				eboxserv(mcf);
				break;
			case C_IBOX:
				iboxserv(mcf);
				break;
			case C_MBOX:
				mboxserv(mcf);
				break;
			case C_TB_ERR:
			case C_MBOX_1D:
				genericserv(mcf);
				break;
			default:
				cprintf("unknown machine check type %x\n",
					(mcf->mc8600_ehm_sts & C_MCHK_MASK));
				badmchk(mcf);
				panic("mchk");
		}
	}
	if(mcf->mc8600_ebcs & M_ABORT_MASK) {
		badmchk(mcf);
		panic("VAX state lost...not recoverable\n");
	}
	logmck((int *)cmcf, ELMCKT_8600, 0, 1);
	mtpr (EHSR, 0); 	/* Clear EHSR
				 - resets the VMS entered bit */
	return(0);
}

/*
 * badmchk is called when we have an unrecoverable machine check,
 * and we need to log the machine check stack frame to the console
 */
badmchk(mcf)
register struct mc8600frame *mcf;
{
	logmck((int *)mcf, ELMCKT_8600, 0, 0);
	cprintf("\nmachine check %x: %s\n",(mcf->mc8600_ehm_sts & 0xf),
		    mc8600[(mcf->mc8600_ehm_sts & 0xf)]);
	cprintf("\tehm.sts\t= %x\n", mcf->mc8600_ehm_sts);
	cprintf("\tevmqsav\t= %x\n", mcf->mc8600_evmqsav);
	cprintf("\tebcs\t= %x\n", mcf->mc8600_ebcs);
	cprintf("\tedpsr\t= %x\n", mcf->mc8600_edpsr);
	cprintf("\tcslint\t= %x\n", mcf->mc8600_cslint);
	cprintf("\tibesr\t= %x\n", mcf->mc8600_ibesr);
	cprintf("\tebxwd1\t= %x\n", mcf->mc8600_ebxwd1);
	cprintf("\tebxwd2\t= %x\n", mcf->mc8600_ebxwd2);
	cprintf("\tivasav\t= %x\n", mcf->mc8600_ivasav);
	cprintf("\tvibasav\t= %x\n", mcf->mc8600_vibasav);
	cprintf("\tesasav\t= %x\n", mcf->mc8600_esasav);
	cprintf("\tisasav\t= %x\n", mcf->mc8600_isasav);
	cprintf("\tcpc\t= %x\n", mcf->mc8600_cpc);
	cprintf("\tmstat1\t= %x\n", mcf->mc8600_mstat1);
	cprintf("\tmstat2\t= %x\n", mcf->mc8600_mstat2);
	cprintf("\tmdecc\t= %x\n", mcf->mc8600_mdecc);
	cprintf("\tmerg\t= %x\n", mcf->mc8600_merg);
	cprintf("\tcshctl\t= %x\n", mcf->mc8600_cshctl);
	cprintf("\tmear\t= %x\n", mcf->mc8600_mear);
	cprintf("\tmedr\t= %x\n", mcf->mc8600_medr);
	cprintf("\taccs\t= %x\n", mcf->mc8600_accs);
	cprintf("\tcses\t= %x\n", mcf->mc8600_cses);
	cprintf("\tpc\t= %x\n", mcf->mc8600_pc);
	cprintf("\tpsl\t= %x\n", mcf->mc8600_psl);
}

/* iboxserv is used to service ibox errors as part of machine check
 * recovery.  If there are three ibox errors within IBOX_THRESH
 * we panic and die.
 */
iboxserv(mcf)
register struct mc8600frame *mcf;
{
	struct ibox_errcnt *iboxp;
	int time;

	iboxp = &ibox_errcnt;
	time = mfpr(TODR);
	iboxp->ibox_total++;
	if((time - iboxp->ibox_prev) <= IBOX_THRESH) {
		badmchk(mcf);
		panic("Too many IBOX errors to recover...\n");
	}
	else {
		iboxp->ibox_prev = iboxp->ibox_last;
		iboxp->ibox_last = time;
	}
}

/*
 * mboxserv is used to service mbox fatal errors.  We will panic and
 * die on all mbox fatal errors for now.
 */
mboxserv(mcf)
register struct mc8600frame *mcf;
{
	struct mbox_errcnt *mboxp;
	int time;

	mboxp = &mbox_errcnt;
	time = mfpr(TODR);
	/* always panic on MBOX_FE for now */
	badmchk(mcf);
	panic("Too many MBOX errors to recover...\n");
}

/*
 * fboxserv turns off the FBOX if there are three errors within 
 * the time of FBOX_THRESH.
 */
fboxserv(mcf)
register struct mc8600frame *mcf;
{
	struct fbox_errcnt *fboxp;
	int time;

	fboxp = &fbox_errcnt;
	time = mfpr(TODR);
	fboxp->fbox_total++;
	if((time - fboxp->fbox_prev) <= FBOX_THRESH) {
		mtpr(ACCS, 0);
		printf("FBOX turned off due to errors\n");
	}
	else {
		fboxp->fbox_prev = fboxp->fbox_last;
		fboxp->fbox_last = time;
	}
}

/*
 * eboxserv services ebox errors.  First check to see of this is 
 * really an MBOX problem.  If three EBOX errors occur during 
 * the time of EBOX_THRESH, panic.
 */
eboxserv(mcf)
register struct mc8600frame *mcf;
{
	struct ebox_errcnt *eboxp;
	int time;

	eboxp = &ebox_errcnt;
	time = mfpr(TODR);
	if(mcf->mc8600_ebcs & M_EDP_PE)
		mboxserv();
	else {
		eboxp->ebox_total++;
		if ((time - eboxp->ebox_prev) <= EBOX_THRESH) {
			badmchk(mcf);
			panic("Too many EBOX errors to recover...\n");
		}
		else {
			eboxp->ebox_prev = eboxp->ebox_last;
			eboxp->ebox_last = time;
		}
	}
}

/*
 * genericserv services all other types of machinechecks for now.
 * We will check the abort bits, log the error and rei.  If we find
 * ourselves back here twice within GENERIC_THRESH, panic.
 */
genericserv(mcf)
register struct mc8600frame *mcf;
{
	struct generic_errcnt *genp;
	int time;

	genp = &generic_errcnt;
	time = mfpr(TODR);
	genp->gen_total++;
	if ((time - genp->gen_prev) <= GENERIC_THRESH) {
		badmchk(mcf);
		panic("Too many generic machine checks to recover\n");
	}
	else {
		genp->gen_prev = genp->gen_last;
		genp->gen_last = time;
	}
}

/*
 *  Function:
 *	ka8600memerr()
 *
 *  Description:
 *	log memory errors in kernel buffer
 *
 *  Arguments:
 *	none
 *
 *  Return value:
 *	none
 *
 *  Side effects:
 *	none
 */


u_long pammdata = 0;

ka8600memerr()
{
	/*
	 *     PLEASE NOTE!!!!
	 *
	 *  DO NOT put any register definitions in front of the
	 *  definition for "reg_11".  The definition for "reg_11" MUST
	 *  be the FIRST register defined in this routine!!!
	 *  "reg_11" is in fact general register 11,
	 *  and must remain as such so that we know where the data
	 *  returned from the mfpr in the following is going to be put
	 */
	 /*
	  * At the time of a single bit error the
	  * memory registers are written into scratch pad
	  * registers 0x25, 0x26, 0x27 & 0x2a in the
	  * Ebox.  The registers should only be read once
	  * into temporary locations.  Inserting any
	  * instructions between the mtpr and mfpr in
	  * the assembly code will result in
	  * unpredictable results -- according to the
	  * VENUS processors registers spec.
	  */
	register int reg_11;

	register int tmp_mdecc, tmp_mear;
	int	tmp_mstat1, tmp_mstat2;
	struct el_rec *elrp;
	struct el_mem *mrp;

	memerrs++;	/* number of times this routine is entered */
   	/* mtpr (ESPA, SPAD_MDECC);		*/
   	/* tmp_mdecc = (mfpr(ESPD));		*/
	asm("mtpr $0x27,$0x4e; mfpr $0x4f,r11");
	tmp_mdecc = reg_11;

	if (mem_err_report != 1) {
		mem_err_report++;
		elrp = ealloc(EL_MEMSIZE,EL_PRILOW);
		if (elrp != NULL) {
		    LSUBID(elrp,ELCT_MEM,EL_UNDEF,ELMCNTR_8600,EL_UNDEF,EL_UNDEF,pammdata);
		    /* mtpr (ESPA, SPAD_MEAR);			*/
		    /* tmp_mear = (mfpr(ESPD));			*/
		    asm("mtpr $0x2a,$0x4e; mfpr $0x4f,r11");
		    tmp_mear = reg_11;

		    /* mtpr (ESPA, SPAD_MSTAT1);		*/
		    /* tmp_mstat1 = (mfpr (ESPD));		*/
		    asm("mtpr $0x25,$0x4e; mfpr $0x4f,r11");
		    tmp_mstat1 = reg_11;

		    /* mtpr(ESPA, SPAD_MSTAT2);			*/
		    /* tmp_mstat2 = (mfpr(ESPD));		*/
		    asm("mtpr $0x26,$0x4e; mfpr $0x4f,r11");
		    tmp_mstat2 = reg_11;

		    mrp = &elrp->el_body.elmem;
		    mrp->elmem_cnt = 1;
		    mrp->elmemerr.cntl = 1;
		    mrp->elmemerr.type = 1;
		    mrp->elmemerr.numerr = 1;
		    mrp->elmemerr.regs[0] = tmp_mdecc;
		    mrp->elmemerr.regs[1] = tmp_mear;
		    mrp->elmemerr.regs[2] = tmp_mstat1;
		    mrp->elmemerr.regs[3] = tmp_mstat2;
		    EVALID(elrp);
		}
	}
	return(0);
}

/*
 * this routine sets the cache to the state passed.  enabled/disabled
 */

ka8600setcache(state)
int state;
{
	mtpr (CSWP, state);
	return(0);
}

/*
 * Memenable enables the memory controller corrected data reporting.
 * This runs at regular intervals, turning on the interrupt.
 * The interrupt is turned off, per memory controller, when error
 * reporting occurs.  Thus we report at most once per memintvl.
 */

ka8600memenable ()
{
	register struct mcr *mcr;

	M8600_ENA;
	mem_err_report = 0;
	return(0);
}

ka8600tocons (c)
	register int c;
{
	int register timo,s;

	c &= TXDB_DATA;
	timo = 30000;
	s = mfpr(TXCS); 	/* save old TXCS */
	while ((mfpr(TXCS)&TXCS_RDY) == 0)
		if(--timo == 0)
			break;
	/* enable logical console */
	mtpr(TXCS, LOGICAL_CONS|WMASKNOW);
	timo = 30000;
	while ((mfpr (TXCS) & TXCS_RDY)==0)
		if(--timo == 0)
			break;
	if ((mfpr(TXCS)&TXDB_ID)==LOGIC_CONS_ID)
		mtpr (TXDB, c);
	timo = 30000;
	while ((mfpr(TXCS)&TXCS_RDY) == 0)
		if(--timo == 0)
			break;
	mtpr(TXCS, s|WMASKNOW);
	return(0);
}

/* get pamm data for memory errors 8600 only! */
#define TXDB_RAC 0x13 
getpammdata()
{
	int register timo;
	int i, s;
	int data[3];
	int mask1 = 0x80;
	int mask2 = 0xc0;
	int mask3 = 0xc0;
	int shift1 = 24;
	int shift2 = 6;
	int shift3 = 22;
	int tmp = 0;

	s = mfpr(RXCS);
	mtpr(RXCS,0);
	cons_putc(TXDB_RAC);
	timo = 30000;
	while ((mfpr(RXCS) & RXCS_DONE) == 0)
		if (--timo == 0)
			break;
	if ((mfpr(RXDB) & 0xff) == TXDB_RAC) {
		for (i = 0; i < 3; i++) {
			timo = 30000;
	        	while ((mfpr(RXCS) & RXCS_DONE) == 0)
		    	if (--timo == 0)
				break;
			data[i] = mfpr(RXDB);
		}
		for (i = 0; i < 8; i++) {
			tmp = data[0] & mask1;
			tmp <<= shift1;
			pammdata |= tmp;
			mask1 >>= 1;
			shift1 -= 3;
			if (i < 4) {
				tmp = data[2] & mask3;
				tmp <<= shift3;
				pammdata |= tmp;
				mask3 >>= 2;
				shift3 -= 2;
			}
			else {
				tmp = data[1] & mask2;
				tmp <<= shift2;
				pammdata |= tmp;
				mask2 >>= 2;
				shift2 -= 2;
			}
		}
	}
	mtpr(RXCS,s);
	printf("pammdata formatted %x\n",pammdata);
}

/*
 * Enable cache and enable floating point accelerator
 */

extern	int	cache_state;

ka8600cachenbl()
{
	cache_state = 0x3;
	if (mfpr(ACCS)) 	/* is there an accelerator */
		mtpr(ACCS, 0x8000);
	getpammdata();
	return(0);
}

ka8600conf()
{
	union cpusid cpusid;
	register int *ip,i;
	extern char Sysbase[];

	cpusid.cpusid = mfpr(SID);
	printf("VAX 86%s0, serial no. %d, hardware level = %d\n",
		((cpusid.cpu8600.cp_eco & 0x80) ? "5" : "0"),
		cpusid.cpu8600.cp_sno, cpusid.cpu8600.cp_eco);
	/*
	 * Identify the IO adapters,  probenexus
	 * will be called when an SBIA is identified.
	 * When other IO adapters exist, an appropriate
	 * probe routine will be called, i.e. BIA
	 */
	probeioa();
}

short *ka8600nexaddr(ioadpt,nexnum) 
 	int ioadpt,nexnum;
{

	return(NEX8600(ioadpt,nexnum));

}

u_short *ka8600umaddr(ioadpt,ubanumber) 
 	int ioadpt,ubanumber;
{

	return(UMEM8600(ioadpt,ubanumber));

}

u_short *ka8600udevaddr(ioadpt,ubanumber) 
 	int ioadpt,ubanumber;
{

	return(UDEVADDR8600(ioadpt,ubanumber));

}

ka8600logsbi(sbi_num,sbi_type,pc_psl)
long sbi_num;
long sbi_type;
long *pc_psl;
{

	extern int cpu;
	int cntr;
	struct el_rec *elrp;
	struct pc_psl_temp {
		int el_pc;
		int el_psl;
	} *pc_pslptr;
	char *cptr;
	int *siloptr;
	int *ip;

	elrp = ealloc(sizeof(struct el_sbia8600), EL_PRISEVERE);
	if (elrp != EL_FULL) {
	    ip = (int *) Ioamap[sbi_num];
	    if (*ip & PG_V) {
		LSUBID(elrp,ELCT_BUS,ELBUS_SBI8600,EL_UNDEF,sbi_num,EL_UNDEF,sbi_type);
		/* load sbi related information */
		(void)fill8600(elrp,sbi_num);

		/* load pc and psl */
		pc_pslptr = (struct pc_psl_temp *) pc_psl;

		elrp->el_body.elsbia8600.sbia_pc = pc_pslptr->el_pc;
		elrp->el_body.elsbia8600.sbia_psl = pc_pslptr->el_psl;

		EVALID(elrp);

		cprintf("\nsbi error: sbi %d\n", sbi_num);
		cprintf("ioaba=  %8x\tdmacid= %8x\tdmacca= %8x\n",
	        Sbi8600.sbia_ioaba, Sbi8600.sbia_dmacid, Sbi8600.sbia_dmacca);
		cprintf("dmabid= %8x\tdmabca= %8x\tdmaaid= %8x\n",
		Sbi8600.sbia_dmabid, Sbi8600.sbia_dmabca,Sbi8600.sbia_dmaaid);
		cprintf("dmaaca= %8x\tdmaiid= %8x\tdmaica= %8x\n",
		Sbi8600.sbia_dmaaca,Sbi8600.sbia_dmaiid,Sbi8600.sbia_dmaica);
		cprintf("ioadc=  %8x\tioaes=  %8x\tioacs=  %8x\n",
		Sbi8600.sbia_ioadc,Sbi8600.sbia_ioaes,Sbi8600.sbia_ioacs);

		cprintf("ioacf=  %8x\ter=     %8x\tto=     %8x\n",
		Sbi8600.sbia_ioacf,Sbi8600.sbia_er,Sbi8600.sbia_to);

		cprintf("fs=     %8x\tsc=     %8x\tmr=     %8x\n",
		Sbi8600.sbia_fs,Sbi8600.sbia_sc,Sbi8600.sbia_mr);
		DELAY(25000);

		cprintf("silo regs\n");
		for (cntr = 0; cntr < EL_SIZE16; cntr++) {
			cprintf("%8x\t", Sbi8600.sbia_silo[cntr]);
			if ((cntr+1) %4 == 0) {
				 cprintf("\n");
				 DELAY(25000);
			}
		}
		DELAY(25000);
		cprintf("\ncsrs\n");
		for (cntr = 0; cntr < EL_SIZE16; cntr++) {
			cprintf("%8x\t", Sbi8600.sbia_csr[cntr]);
			if ((cntr+1) %4 == 0) {
				 cprintf("\n");
				 DELAY(25000);
			}
		}
		DELAY(25000);
		cprintf("\npc= %8x\tpsl= %8x\n", Sbi8600.sbia_pc,
			Sbi8600.sbia_psl);
	    }
	    else {
		LSUBID(elrp,ELCT_BUS,ELBUS_SBI8600,EL_UNDEF,sbi_num,EL_UNDEF,sbi_type);
		cprintf("\nsbi error: sbi %d\n", sbi_num);
		/* don't validate */
	    }
		
	}
	else {
		cprintf("\nsbi error: sbi %d\n", sbi_num);
	}
}
/*
 * Function: fill8600(elrp, sbi_num)
 *
 * Function description:  load sbi related info into errorlog buffer 
 *
 * Arguments:   elrp - pointer to loc to write err log packet
 *		sbi_num - sbi number being logged
 *
 * Return value: None
 *
 * Side effects: None
 *
 */
fill8600(elrp, sbi_num)
struct el_rec *elrp;
long sbi_num;
{
	union cpusid cpusid;
	register char  *ioav;
	struct sbia_regs *sbiv;
	int cnt;
	int *csrptr;
	int *siloptr;

	cpusid.cpusid = mfpr(SID);

	switch(cpu) {
		case VAX_8600:
			ioav = (char *)ioa;	/* ioa in ioa.h */
			ioav += (cpup->pc_ioasize) * sbi_num;
			sbiv = (struct sbia_regs *)ioav;

			elrp->el_body.elsbia8600.sbia_ioaba=(int) ioav;
			elrp->el_body.elsbia8600.sbia_dmacid=sbiv->sbi_dmacid;
			elrp->el_body.elsbia8600.sbia_dmacca=sbiv->sbi_dmaccs;
			elrp->el_body.elsbia8600.sbia_dmabid=sbiv->sbi_dmabid;
			elrp->el_body.elsbia8600.sbia_dmabca=sbiv->sbi_dmabcs;
			elrp->el_body.elsbia8600.sbia_dmaaid=sbiv->sbi_dmaaid;
			elrp->el_body.elsbia8600.sbia_dmaaca=sbiv->sbi_dmaaca;
			elrp->el_body.elsbia8600.sbia_dmaiid=sbiv->sbi_dmaiid;
			elrp->el_body.elsbia8600.sbia_dmaica=sbiv->sbi_dmaica;
			elrp->el_body.elsbia8600.sbia_ioadc=sbiv->sbi_dctl;
			elrp->el_body.elsbia8600.sbia_ioaes=sbiv->sbi_errsum;
			elrp->el_body.elsbia8600.sbia_ioacs=sbiv->sbi_csr;
			elrp->el_body.elsbia8600.sbia_ioacf=sbiv->sbi_cfg;

			elrp->el_body.elsbia8600.sbia_er=sbiv->sbi_error;
			elrp->el_body.elsbia8600.sbia_to=sbiv->sbi_timo;
			elrp->el_body.elsbia8600.sbia_fs=sbiv->sbi_fltsts;
			elrp->el_body.elsbia8600.sbia_sc=sbiv->sbi_silcmp;
			elrp->el_body.elsbia8600.sbia_mr=sbiv->sbi_maint;

			siloptr = elrp->el_body.elsbia8600.sbia_silo;
			for(cnt=0; cnt < EL_SIZE16; cnt++) {
				*siloptr++ =  sbiv->sbi_silo;
			}
			csrptr = elrp->el_body.elsbia8600.sbia_csr;
			nexusinfo(elrp, sbi_num, csrptr);
			break;

		default: 
			break;

	}
}
