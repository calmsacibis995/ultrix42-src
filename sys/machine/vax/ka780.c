#ifndef lint
static char *sccsid = "@(#)ka780.c	4.1	ULTRIX	7/2/90";
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
 * 24-May-89	darrell
 *	Changed the #include for cpuconf.h to find it in it's new home --
 *	sys/machine/common/cpuconf.h
 *
 * 24-May-89	darrell
 *	Removed the v_ prefix from all cpusw fields, removed cpup from any
 *	arguments being passed in function args.  cpup is now defined
 *	globally -- as part of the new cpusw.
 *
 * 11-Jun-86    bjg
 *	Add sbi number to cprintf output
 *
 * 02-Apr-86	pmk
 *	changed ka780memerr check low and hi for 780E memory
 *
 * 15-Mar-86	Darrell Dunnuck
 *	Moved ka780 specific parts of configure() here into ka780conf.
 *
 * 12-Mar-86 -- bjg
 *	moved sbi logging routines to ka780.c from kern_errlog.c
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
 *	Added a new routines cachenbl and nullcpu.
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

extern long sbi_there;	/* defined in autoconf.c */
extern int nexusinfo();	/* defined in errlog.c */

char   *mc780[] = {
	"cp read timeout or error confirmation",/* 0 */
	"control store parity error",		/* (F)1 */
	"cp tb parity error",			/* 2 */
	"cp cache parity error",		/* 3 */
	0,					/* 4 */
	"cp read data substitute",		/* 5 */
	"microcode lost",			/* (F)6 */
	0, 0, 0,				/* 7, 8, & 9 */
	"ib tb parity error",			/* A */
	0,					/* B */
	"ib read data substitute",		/* C */
	"ib read timeout or error confirmation",/* D */
	0,					/* E */
	"ib cache parity error" 		/* F */

};

struct mc780frame {
	int	mc8_bcnt;			/* byte count == 0x28 */
	int	mc8_summary;			/* summary parameter */
	int	mc8_cpues;			/* cpu error status */
	int	mc8_upc;			/* micro pc */
	int	mc8_vaviba;			/* va/viba register */
	int	mc8_dreg;			/* d register */
	int	mc8_tber0;			/* tbuf error reg 0 */
	int	mc8_tber1;			/* tbuf error reg 1 */
	int	mc8_timo;			/* timeout address */
	int	mc8_parity;			/* parity */
	int	mc8_sbier;			/* sbi error register */
	int	mc8_pc; 			/* trapped pc */
	int	mc8_psl;			/* trapped psl */
};


ka780machcheck (cmcf)
caddr_t cmcf;
{
	register u_int type = ((struct mcframe	 *) cmcf) -> mc_summary;
	register struct mc780frame *mcf = (struct mc780frame   *) cmcf;
	register int	sbifs;
	int recover = 0;
	int cpunum = 0;
	unsigned t;

	setcache(0x218000);	/* disable cache */

	logmck((int *)cmcf, ELMCKT_780, cpunum, recover);
	if (recover == 0) {
	    cprintf("\nmachine check %x: ", type);
	    switch (type & 0xff) {
		case 0:
		case 2:
		case 3:
		case 5:
		case 0xa:
		case 0xc:
		case 0xd:
		case 0xf:
		case 0xf0:
		case 0xf1:
		case 0xf2:
		case 0xf3:
		case 0xf5:
		case 0xf6:
			cprintf("%s%s\n", mc780[type & 0xf],
				(type & 0xf0) ? " abort" : " fault");
			break;
		default:
			cprintf("%s\n", "Unknown machine check type code");
			break;
	    }
	    cprintf("\tsumpar\t= %x\n", mcf -> mc8_summary);
	    cprintf("\tcpues\t= %x\n", mcf -> mc8_cpues);
	    cprintf("\tupc\t= %x\n", mcf -> mc8_upc);
	    cprintf("\tva/viba\t= %x\n", mcf -> mc8_vaviba);
	    cprintf("\tdreg\t= %x\n", mcf -> mc8_dreg);
	    cprintf("\ttber0\t= %x\n", mcf -> mc8_tber0);
	    cprintf("\ttber1\t= %x\n", mcf -> mc8_tber1);
	    cprintf("\ttimo\t= %x\tphyadr\t= %x\n",
		mcf -> mc8_timo, ((mcf -> mc8_timo & 0x1fffffff) * 4));
	    cprintf("\tparity\t= %x\n", mcf -> mc8_parity);
	    cprintf("\tsbier\t= %x\n", mcf -> mc8_sbier);
	    cprintf("\tpc\t= %x\n", mcf -> mc8_pc);
	    cprintf("\tpsl\t= %x\n\n", mcf -> mc8_psl);
	    sbifs = mfpr (SBIFS);
	    cprintf("\tsbifs\t= %x\n", sbifs);
	}
	/* THE FUNNY BITS IN THE FOLLOWING ARE FROM THE ``BLACK */
	/* BOOK'' AND SHOULD BE PUT IN AN ``sbi.h'' */
	mtpr (SBIFS, sbifs & ~0x2000000);
	/* clear SBIER but leave any mem err bits */
	mtpr (SBIER, 0x90c0);

	memerr ();
	panic ("mchk");
	return(0);
}

/*
 *  Function:
 *	ka780memerr()
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

ka780memerr()
{
	int m;
	int low = 0;
	int hi = 0;
	struct mcr *mcr;
	struct el_rec *elrp;
	struct el_mem *mrp;

	for (m = 0; m < nmcr; m++) {
	    mcr = mcrdata[m].mcraddr;
	    switch (mcrdata[m].memtype) {
	    case MEMTYPE_MS780C:
		if (M780C_ERR (mcr)) {
		    elrp = ealloc(EL_MEMSIZE,EL_PRILOW);
		    if (elrp != NULL) {
			LSUBID(elrp,ELCT_MEM,EL_UNDEF,ELMCNTR_780C,EL_UNDEF,EL_UNDEF,EL_UNDEF);
			mrp = &elrp->el_body.elmem;
			mrp->elmem_cnt = 1;
			mrp->elmemerr.cntl = m + 1;
			mrp->elmemerr.type = M780C_HRDERR(mcr) ? 2 : 1;
			mrp->elmemerr.numerr = 1;
			mrp->elmemerr.regs[0] = mcr->mc_reg[0];
			mrp->elmemerr.regs[1] = mcr->mc_reg[1];
			mrp->elmemerr.regs[2] = mcr->mc_reg[2];
			mrp->elmemerr.regs[3] = EL_UNDEF;
			EVALID(elrp);
		    }
		    M780C_INH(mcr);
		}
		break;
	    case MEMTYPE_MS780E:
		if ((low = M780E_ERR(mcr,0)) || (hi = M780E_ERR(mcr,1))) {
		    elrp = ealloc(EL_MEMSIZE,EL_PRILOW);
		    if (elrp != NULL) {
			LSUBID(elrp,ELCT_MEM,EL_UNDEF,ELMCNTR_780E,EL_UNDEF,EL_UNDEF,EL_UNDEF);
			mrp = &elrp->el_body.elmem;
			mrp->elmem_cnt = 1;
			mrp->elmemerr.cntl = m + 1;
			if (low)
			    mrp->elmemerr.type = M780E_HRDERR(mcr,0) ? 2 : 1;
			else
			    mrp->elmemerr.type = M780E_HRDERR(mcr,1) ? 2 : 1;
			mrp->elmemerr.numerr = 1;
			mrp->elmemerr.regs[0] = mcr->mc_reg[0];
			mrp->elmemerr.regs[1] = mcr->mc_reg[1];
			mrp->elmemerr.regs[2] = mcr->mc_reg[2];
			mrp->elmemerr.regs[3] = mcr->mc_reg[3];
			EVALID(elrp);
		    }
		    if (low > 0)
		        M780E_INH(mcr,0);
		    if (hi > 0)
		        M780E_INH(mcr,1);
		}
		break;
	    }
	}
	return(0);
}

/*
 * this routine sets the cache to the state passed.  enabled/disabled
 */

ka780setcache(state)
int state;
{
	mtpr (SBIMT, state);
	return(0);
}

/*
 * Memenable enables the memory controller corrected data reporting.
 * This runs at regular intervals, turning on the interrupt.
 * The interrupt is turned off, per memory controller, when error
 * reporting occurs.  Thus we report at most once per memintvl.
 */

ka780memenable () 
{
	register struct mcr *mcr;
	register int	m;

	for (m = 0; m < nmcr; m++) {
		mcr = mcrdata[m].mcraddr;
		switch (mcrdata[m].memtype) {
			case MEMTYPE_MS780C:
				M780C_ENA (mcr);
				break;
			case MEMTYPE_MS780E: {
				int interleave = mcr->mc_reg[0] & 0x7;

				if (interleave != 2 && interleave != 3)
					M780E_ENA (mcr, 0);
				if (interleave != 0 && interleave != 1)
					M780E_ENA (mcr, 1);
				break;
				}
		}
	}
	return(0);
}

/*
 * Enable cache
 */

extern	int	cache_state;

ka780cachenbl()
{
	cache_state = 0x200000;
	return(0);
}

ka780tocons(c)
	register int c;
{
	while ((mfpr (TXCS) & TXCS_RDY) == 0)
		continue;
	mtpr (TXDB, c);
	return(0);
}

ka780conf()
{
	union cpusid cpusid;

	cpusid.cpusid = mfpr(SID);
	printf("VAX 11/78%s, serial no. %d, hardware level = %d\n",
		((cpusid.cpu780.cp_eco & 0x10) ? "5" : "0"),
		cpusid.cpu780.cp_sno, cpusid.cpu780.cp_eco);
	probesbi(0);
	return(0);
}

#ifdef TRENDATA
/*
 * Figure out what chip to replace on Trendata boards.
 * Assumes all your memory is Trendata or the non-Trendata
 * memory never fails..
 */
struct {
	u_char m_syndrome;
	char	m_chip[4];
}
	memlogtab[] = {
		0x01, "C00", 0x02, "C01", 0x04, "C02", 0x08, "C03",
		0x10, "C04", 0x19, "L01", 0x1A, "L02", 0x1C, "L04",
		0x1F, "L07", 0x20, "C05", 0x38, "L00", 0x3B, "L03",
		0x3D, "L05", 0x3E, "L06", 0x40, "C06", 0x49, "L09",
		0x4A, "L10", 0x4c, "L12", 0x4F, "L15", 0x51, "L17",
		0x52, "L18", 0x54, "L20", 0x57, "L23", 0x58, "L24",
		0x5B, "L27", 0x5D, "L29", 0x5E, "L30", 0x68, "L08",
		0x6B, "L11", 0x6D, "L13", 0x6E, "L14", 0x70, "L16",
		0x73, "L19", 0x75, "L21", 0x76, "L22", 0x79, "L25",
		0x7A, "L26", 0x7C, "L28", 0x7F, "L31", 0x80, "C07",
		0x89, "U01", 0x8A, "U02", 0x8C, "U04", 0x8F, "U07",
		0x91, "U09", 0x92, "U10", 0x94, "U12", 0x97, "U15",
		0x98, "U16", 0x9B, "U19", 0x9D, "U21", 0x9E, "U22",
		0xA8, "U00", 0xAB, "U03", 0xAD, "U05", 0xAE, "U06",
		0xB0, "U08", 0xB3, "U11", 0xB5, "U13", 0xB6, "U14",
		0xB9, "U17", 0xBA, "U18", 0xBC, "U20", 0xBF, "U23",
		0xC1, "U25", 0xC2, "U26", 0xC4, "U28", 0xC7, "U31",
		0xE0, "U24", 0xE3, "U27", 0xE5, "U29", 0xE6, "U30"
};

memlog (m, mcr)
int	m;
struct mcr *mcr;
{
	register	i;


	for (i = 0; i < (sizeof (memlogtab) / sizeof (memlogtab[0])); i++)
		if ((u_char) (M780C_SYN (mcr)) == memlogtab[i].m_syndrome) {
			printf ("mcr%d: replace %s chip in %s bank of memory board %d (0-15)\n",
				m, memlogtab[i].m_chip,
				(M780C_ADDR (mcr) & 0x8000) ? "upper" : "lower",
				(M780C_ADDR (mcr) >> 16));
			return;
		}
	printf ("mcr%d: multiple errors, not traceable\n", m);
	break;
}
#endif TRENDATA

short *ka780nexaddr(ioadpt,nexnum) 
 	int ioadpt,nexnum;
{

	return(NEX780(nexnum));

}

u_short *ka780umaddr(ioadpt,ubanumber) 
 	int ubanumber,ioadpt;
{

	return(UMEM780(ubanumber));

}

u_short *ka780udevaddr(ioadpt,ubanumber) 
 	int ubanumber,ioadpt;
{

	return(UDEVADDR780(ubanumber));

}

ka780logsbi(sbi_num,sbi_type,pc_psl)
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

	elrp = ealloc(sizeof(struct el_sbi_aw780), EL_PRISEVERE);
	if (elrp != EL_FULL) {
		LSUBID(elrp,ELCT_BUS,ELBUS_SBI780,EL_UNDEF,sbi_num,EL_UNDEF,sbi_type);

		/* load nexus information */
		(void)fill780(elrp, sbi_num);

		pc_pslptr = (struct pc_psl_temp *) pc_psl;
		elrp->el_body.elsbiaw780.sbiaw_pc = pc_pslptr->el_pc;
		elrp->el_body.elsbiaw780.sbiaw_psl = pc_pslptr->el_psl;

		EVALID(elrp);

		cprintf("\nsbi error: sbi %d\n", sbi_num);
		cprintf("sbiaw_er= %8x\tsbiaw_toa= %8x\tsbiaw_fs= %8x\nsbiaw_sc= %8x\tsbiaw_mt= %8x\n", 
		Sbi780.sbiaw_er, Sbi780.sbiaw_toa, Sbi780.sbiaw_fs, Sbi780.sbiaw_sc, Sbi780.sbiaw_mt);
		DELAY(1000);
		cprintf("silo regs\n");
		for (cntr = 0; cntr < EL_SIZE16; cntr++) {
			cprintf("%8x\t", Sbi780.sbiaw_silo[cntr]);
			if ((cntr+1) %4 == 0) {
				 cprintf("\n");
				 DELAY(1000);
			}
		}
		DELAY(1000);
		cprintf("\ncsrs\n");
		for (cntr = 0; cntr < EL_SIZE16; cntr++) {
			cprintf("%8x\t", Sbi780.sbiaw_csr[cntr]);
			if ((cntr+1) %4 == 0) {
				cprintf("\n");
				DELAY(1000);
			}
		}
		DELAY(1000);
		cprintf("\npc= %8x\tpsl= %8x\n", Sbi780.sbiaw_pc,
			Sbi780.sbiaw_psl);
	}
	else {
		cprintf("\nsbi error: sbi %d\n", sbi_num);
	}
}

/*
 * Function: fill780(elrp, sbi_num)
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
fill780(elrp, sbi_num)
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
		case VAX_780:
			elrp->el_body.elsbiaw780.sbiaw_er = mfpr(SBIER);
			elrp->el_body.elsbiaw780.sbiaw_toa = mfpr(SBITA);
			elrp->el_body.elsbiaw780.sbiaw_fs = mfpr(SBIFS);
			elrp->el_body.elsbiaw780.sbiaw_sc = mfpr(SBISC);
			elrp->el_body.elsbiaw780.sbiaw_mt = mfpr(SBIMT);
			siloptr = elrp->el_body.elsbiaw780.sbiaw_silo;
			for (cnt = 0; cnt < EL_SIZE16; cnt++) {
				*siloptr++ = mfpr(SBIS);
			}
			csrptr = elrp->el_body.elsbiaw780.sbiaw_csr;
			nexusinfo(elrp, sbi_num, csrptr);
			break;

		      default: 
			break;

	}
}
