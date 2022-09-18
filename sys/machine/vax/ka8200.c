#ifndef lint
static char *sccsid = "@(#)ka8200.c	4.1      ULTRIX  7/2/90";
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

/* ------------------------------------------------------------------------
 * Modification History:
 *
 * 27-Nov-89    Paul Grist
 *      added frame_type argument to logmck() call.
 *
 * 06-Nov-89    Paul Grist
 *      Made modifications to ka8200machcheck() routine:
 *        o For all machine checks, scan VAXBI and log any errors
 *        o Report recoverabe machine checks to error logger
 *        o Fixed problem (resulting from vmb) which caused lost dumps
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
 *  26-Jan-89	jaw
 *	fix up start/stop cpu.
 *
 * 18-Jun-88 -- jaw  change to new cpu data format.
 *
 * 05-Aug-86 -- jaw
 *	move bistop command after the bus scan for CPU's.  The Scan of
 *	the bus will cause BI errors to occur.
 *
 * 28-Jul-86 -- bjg
 *	Set cpu_subtype to indicate 8200, 8300, or 8400;
 *		and log cpu_subtype with MEM errs
 *
 * 21-Apr-86 -- jrs
 *	Add code to configure to do a "quick" scan down the bi so that
 *	we can determine what system identity to use.
 *
 * 09-Apr-86 -- jaw  make bierror routine work for multiple bi's.
 *
 * 02-Apr-86 -- jaw  add support for nautilus console and memory adapter
 *
 * 18-Mar-86 -- jrs
 *	Change mask to allow inter processor interrupts from slaves for
 *	scheduling purposes.
 *
 * 13-Mar-86 -- darrell
 *	Passing cpup into ka820init -- for consistency sake.
 *
 * 05-Mar-86 -- pmk
 * 	added arg recover to logmck and replaced display with cprintf
 *
 * 05-Mar-86 -- jaw  VAXBI device and controller config code added.
 *		     todr code put in cpusw.
 *
 * 03-Mar-86 -- jrs
 *	Cleaned up secondary processor start code to go through defined
 *	area in rpb rather than creating awkward direct jump.
 *
 * 25-Feb-86 -- jrs
 *	Added code to flush secondary processor output before trying attn.
 *
 * 19-Feb-86 -- pmk
 *	Added for loop to memenable for check of all controllers
 *
 * 18-Feb-86 -- jrs
 *	Added code to force the secondary processors to start automatically
 *	after autoconfig is complete.
 *
 * 18-Mar-86 -- jaw  add routines to cpu switch for nexus/unibus addresses
 *		   also got rid of some globals like nexnum.
 *
 * 12-Feb-86	Darrell Dunnuck
 *	Removed the routines memerr, memenable, setcache, and tocons
 *	from machdep.c and put them here for this processor type.
 *	Added a new routine cachenbl.
 *
 * 	04-feb-86 -- jaw  get rid of biic.h.
 *
 *	03-Feb-86 -- jaw -- added 8200 config routine.
 *
 *	15-Jan-86 -- darrell
 *		Moved this file from sys/vaxbi to sys/vax, and removed
 *		the structure definition for the generic machinecheck
 *		stack frame.
 *
 *	20-Jan-86 -- pmk  add machine check errlogging
 *
 *	11-Nov-85 -- jaw  test for time of last machine check was incorrect.
 *
 *	27-OCT-85 -- jaw  BDA reset bug fix.
 *
 *	26-Oct-85 -- jaw  Bug fixes to machine check handler.
 *
 *	05-Jul-85 -- jaw  fixup CRD handling.
 *
 *	26-Jun-85 -- jaw  Machine check handler not printing type if 
 *		 	  multiple errors have occurred.
 *
 * 	19-Jun-85 -- jaw VAX8200 name change.
 *
 *	05 Jun 85 -- jaw  clean up and setup port CSR.
 *
 * 	20 Mar 85 -- jaw  add support for VAX 8200
 *
 *
 * ------------------------------------------------------------------------
 */


#include "../machine/pte.h"
#include "../h/param.h"
#include "../h/smp_lock.h"
#include "../h/systm.h"
#include "../h/map.h"
#include "../h/buf.h"
#include "../h/time.h"
#include "../h/kernel.h"
#include "../h/errlog.h"
#include "../h/errno.h"
#include "../../machine/common/cpuconf.h"
#include "../h/cpudata.h"

#include "../machine/cons.h"
#include "../machine/cpu.h"
#include "../machine/mtpr.h"
#include "../machine/scb.h"
#include "../machine/clock.h"
#include "../machine/mem.h"
#include "../machine/nexus.h"
#include "../machine/rx50.h"
#include "../machine/ka8200.h"

#include "../io/uba/ubareg.h"
#include "../io/uba/ubavar.h"

#include "../io/bi/bimemreg.h"
#include "../io/bi/bireg.h"

extern int (*UNIvec[])();
extern int numuba;
extern int nNUBA;
extern int cache_state;
extern int ignorebi;
extern int cold;
extern struct bidata bidata[];
extern struct cpusw *cpup;	/* pointer to cpusw entry */

#define	TLKBUF	100
int	cputalking = -1;
int	cpureport = -1;
char	talkbuf[TLKBUF];
char	*talkin = talkbuf;
char	*talkout = talkbuf;

char   *mc8200[] = {
	"cpu bad ipl",
	"cpu microcode lost error",
	"cpu microcode parity error",
	"cpu DAL parity error",
	"BI bus error",
	"cpu BTB tag parity error",
	"cpu Cache tag parity error"
};
#define MC8200_BADIPL	0x1
#define MC8200_UERR	0x2
#define MC8200_UPARITY	0x4
#define MC8200_DPARITY	0x8
#define MC8200_BIERR	0x10
#define MC8200_BPARITY	0x20
#define MC8200_CPARITY	0x40

struct mc8200frame {
	int	mc8200_bcnt;
	int	mc8200_summary;
	int	mc8200_parm1;
	int	mc8200_va;
	int	mc8200_vap;
	int	mc8200_mar;
	int	mc8200_stat;
	int	mc8200_pcfail;
	int	mc8200_upcfail;
	int	mc8200_pc;
	int	mc8200_psl;
};
extern int smp;

ka8200conf()
{
	union cpusid cpusid;
	int bicpunode,nexndx;
	char *nxp;
	register int binode;
	register short type;
	struct bi_nodespace *nxv;
	register int procfound=0;
	char *nxpp;

	bicpunode = mfpr(BINID);

	cpusid.cpusid = mfpr(SID);
	bidata[0].bivirt = (struct bi_nodespace *) nexus;
	bidata[0].biphys = (struct bi_nodespace *) NEX8200(0);
	bidata[0].bivec_page = &scb.scb_stray;

	bidata[0].cpu_biic_addr = bidata[0].bivirt + bicpunode;
	nxp = (char *) (ka8200nexaddr(0,bicpunode));
	nxaccess(nxp,Nexmap[(bicpunode)],BINODE_SIZE);


	/* count number of cpus */
	ignorebi = 1;

	mtpr(BISTOP,0x0ffff);
	nxv = bidata[0].bivirt;
	for (binode = 0; binode < NBINODES; binode++, nxv++) {
		nxpp = (char *) ka8200nexaddr(0, binode);
		nxaccess(nxpp, Nexmap[binode], BINODE_SIZE);
		if ((*cpup->badaddr)((caddr_t) nxv, sizeof(long)) == 0) {
			type=(short) nxv->biic.biic_typ;
			if ( type== BI_KA820) {
			    if (binode != mfpr(BINID)) {
			        /* secondary processor startup needed */
				procfound |= 1 << binode;
				cpu_avail++;
			    }
			    else  /* take out else to make interrupts symmetric */
			    	bidata[0].biintr_dst |= (1 << binode);
			}
			if (type==BI_BUA || type==BI_BLA || type==BI_BDA ||
			    type==BI_COMB)
				bisst(&nxv->biic.biic_ctrl);
		}
	}

	printf("VAX 8%s0 hardware rev = %d, ucode patch rev = %d\n",
		((procfound != 0)? "30": "20"),
		cpusid.cpu8200.cp_hrev, cpusid.cpu8200.cp_patch);
	printf("		sec patch = %d,  ucode rev = %d\n",
		cpusid.cpu8200.cp_secp, cpusid.cpu8200.cp_urev);
	
	/* set cpu_subtype for error logger */
	if (procfound !=0) cpu_subtype = ST_8200;
	else cpu_subtype = ST_8300;

	bisetvec(0);

	(void) spl0();
	probebi(0);
	/* Write protect the scb and Unibus interrupt
	 * vectors 
	 */
	return(0);
}

ka8200stopcpu() 
{
      	spl7();
	CURRENT_CPUDATA->cpu_state &= ~CPU_RUN;
	asm("halt");
}

ka8200startcpu(cpu_num)
int cpu_num;
{

	struct bi_nodespace *nxv;

	nxv = bidata[0].bivirt;
	nxv += cpu_num;
	if (((bidata[0].binodes_alive & (1 << cpu_num)) != 0) &&
		(((short) nxv->biic.biic_typ) == BI_KA820)) {
		get_cpudata(cpu_num);
		bidata[0].biintr_dst |= (1<<cpu_num);

		/*ka820_setbi_int(); for symmetrix interrupts */
		ka820procinit(cpu_num);
		return(1);
	}
	return(0);
}
#ifdef notdef
/* 
 *	Set new interrupt destination field in BIIC of all
 *	MP safe boards.
 */
ka820_setbi_int() {
	int i;
	struct bisw *pbisw;
	struct bi_nodespace *nxv;
	
	nxv =bidata[0].bivirt;
	for(i=0; i<16 ; i++, nxv++) {
		if (bidata[0].binodes_alive & (1<<i)) {
			pbisw = bidata[0].bierr[i].pbisw;
			if ((pbisw->bi_flags & (BIF_DEVICE|BIF_CONTROLLER|BIF_ADAPTER)) &&
			    (pbisw->bi_flags & BIF_SMP)) {
			    	nxv->biic.biic_int_dst = bidata[0].biintr_dst;
			}	
		}
	}
}
#endif notdef

ka820init(nxv,nxp,binumber,binode)
char *nxp;
struct ka820_regs *nxv;
int binumber;
int binode;
{
	/* allow ipintr from all sources */
	nxv->ka820_biic.biic_ip_msk = 0xffff0000;

	/* map VAX8200 onboard options */
	nxaccess((char *) PORTV8200,V8200pmap,512);
	nxaccess((char *) WATCHV8200,V8200wmap,512);
	nxaccess((char *) RX50V8200,V8200rmap,512);

	v8200port |= (V8200_CRDCLR |
			V8200_CONSEN | V8200_CONSCLR | V8200_RXEN);

}


ka820slavehalt() {
	register int i;

	printf("\nhalting slave processors \n");

	for(i=0; i<32; i++) {
		if (CPUDATA(i)) {
			(void)	ka820txstr(CPUDATA(i)->cpu_num, "\020"); 
			(void)	ka820prompt();
		}
	}
}
/*
 * ka820procinit() -
 *
 *	Try and start the processor and the specified node.
 *
 */

ka820procinit(nexid)
int nexid;
{

		/* initialize buffer for capturing chars from secondary
		   processor and then tell interrupt routine who to capture */

		talkin = talkbuf;
		talkout = talkbuf;
		cputalking = nexid;

		/* flush any pending message secondary is trying to send */

		(void) ka820prompt();

		/* try and send control-p to get secondary's attention and
		   see if he sends us ">>>" prompt */

		if (ka820txstr(nexid, "\020") == 0) {
			printf("VAXBI node %d - Attention FAILED\n", nexid);
		} else if (ka820prompt() == 0) {
			printf("VAXBI node %d - No reply to attention\n",
				nexid);
			}

		/* Send init command to seconardy.  Check command echo
		   and receipt of new prompt */

		if (ka820txstr(nexid, "I\r") == 0) {
			printf("VAXBI node %d - First stage initialization FAILED\n",
				nexid);
		} else if (ka820matchl("I\r") == 0 || ka820prompt() == 0) {
			printf("VAXBI node %d - No reply to first stage initialization\n",
				nexid);
		} else {

			/* send the start string and check echo */

			if (ka820txstr(nexid, "S 100\r") == 0
					|| ka820matchl("S 100\r") == 0) {
				printf("VAXBI node %d - Startup failed\n",
					nexid);
			}
		}

		/* dialogue has ended, tell intr routine anything else is
		   unanticipated and erroneous */

		cputalking = -1;
}

ka820rxcd()
{
	register int binode;
	register short conschar;
	register int fromcpu;

	conschar = mfpr(RXCD);
	binode = mfpr(BINID);
	fromcpu = (conschar >> 8) & 0x1f;

	/* we are on master CPU */

	if (BOOT_CPU) {

		/* handle chars from cpu we are trying to handle */

		if (fromcpu == cputalking) {
			*talkin++ = (char) conschar;
			if ((talkin - talkbuf) >= TLKBUF) {
				talkin = talkbuf;
			}
			if (talkin == talkout) {
				printf("CPU conversation buffer overflow\n");
			}

		/* unsolicited chars from a cpu */

		} else {

			/* if we are booting, just return as we may not
			   be able to safely handle anything */

			if (cold != 0) {
				return;
			}

			/* is a label necessary ? */

			if (fromcpu != cpureport) {
				printf("VAXBI node %d: ", fromcpu);
				cpureport = fromcpu;
			}

			/* print the character we got */

			printf("%c",(char) conschar);

			/* end of line should reset reporting cpu so
			   we get new header on next line */

			if ((conschar & 0x7f) == '\n') {
				cpureport = -1;
			}
		}
	}
}

/*
 * ka820matchl -
 *
 *	Sit and match echoed characters from supplied command line.
 *	Make sure we handle extra characters from end of line.
 *	Return 1 if match succeeds, 0 if it fails.
 *
 */

ka820matchl(cmdline)
char *cmdline;
{
	register char *current;
	register int incoming;

	current = cmdline;

	/* loop until match string is exhausted or timeout occurs */

	while (*current != '\0') {
		if ((incoming = ka820nxtchr()) < 0 || incoming != *current) {
			break;
		}
		current++;
	}

	/* if string is okay so far, we must also match newline if last
	   character of match string (last thing we saw) was a return */

	if (*current == '\0') {
		if (incoming == '\r') {
			if (ka820nxtchr() != '\n') {
				return(0);
			}
		}
		return(1);
	}
	return(0);
}

/*
 * ka820prompt -
 *
 *	Sit until we find a prompt on the remote port and
 *	no further input.  Return 0 if timedout without prompt
 *	else 1 if prompt was last thing detected.
 *
 */

ka820prompt()
{
	register int anglecnt;
	register int current;

	anglecnt = 0;

	/* we sit in loop till timeout, not just ">>>" as we don't really
	   know what might be queued on secondardy cpu.  Need quiescent
	   state so we wait... */

	while ((current = ka820nxtchr()) >= 0) {
		if (current == '>') {
			anglecnt++;
		} else {
			anglecnt = 0;
		}
	}
	return((anglecnt >= 3)? 1: 0);
}

/*
 * ka820nxtchr -
 *
 *	Return next character from secondary processor buffer.
 *	Wait for interval is character not available.
 *	Return character if possible, else -1 if timeout occurs
 *	before character arrives.
 *
 */

ka820nxtchr()
{
	register int s;
	register int timeout;
	register result;

	/* wait a decent interval */

	timeout = 30000;
	while (--timeout > 0) {

		/* take from buffer if possible */

		if (talkin != talkout) {
			s = spl5();
			result = *talkout++;
			if ((talkout - talkbuf) >= TLKBUF) {
				talkout = talkbuf;
			}
			splx(s);
			return(result & 0xff);
		}
	}
	return(-1);
}

/*
 * ka820txstr -
 *
 *	Send string to remote bi node (usually secondary processor)
 *
 *	Return char count if successful or 0 if timeout exceeded
 *
 */

ka820txstr(node, string)
register int node;
char *string;
{
	register char *current;
	register int timeout;
	register int sendvalue;

	/* loop for entire send string */

	current = string;
	while ((sendvalue = *current++) != 0) {

		/* destination is part of value */

		sendvalue |= (node << 8);
		timeout = 30000;

		/* the following is a perverted while statement since
		   c has no clean way of checking the 'v' bit */

		asm("txloop:");
		if (--timeout > 0) {
			mtpr(RXCD, sendvalue);
			asm("bvs txloop");
		}

		/* secondary did not come ready in time, send is assumed
		   to have failed */

		if (timeout <= 0) {
			return(0);
		}
	}
	return(current - string);
}

unsigned dparity8200 = 0;
unsigned bierr8200 = 0;
unsigned bparity8200 = 0;
unsigned cparity8200 = 0;
int errcnt8200=0;

ka8200machcheck(cmcf)
caddr_t cmcf;
{
	struct bi_nodespace *nxv;
	int type;
	int recover;
	register struct mc8200frame *mcf = (struct mc8200frame	 *) cmcf;
	int index=3;  /* index to mc8200[] for recoverable error msg */

	nxv = (struct bi_nodespace *) nexus;
	nxv += (mfpr(BINID));
	type = ((struct mcframe *) cmcf)->mc_summary;
	recover = 1;
	cache_state = 0;

	/* unexpected interrupt when booting system */
	if (cold == 1) {
		errcnt8200++;
		if (errcnt8200 > 1000) recover=0;
	}
	else {
		/* BAD ipl is fatal error (microcode lost) */
		if ((type & MC8200_BADIPL) != 0) {
			cprintf("%s\n", mc8200[0]);
			recover = 0;
		}

		/* MICROCODE error is fatal (microcode lost) */
		if ((type & MC8200_UERR) != 0) {
			cprintf("%s\n", mc8200[1]);
			recover = 0;
		}

		/* MICROCODE parity error is fatal (microcode lost) */
		if ((type & MC8200_UPARITY) != 0) {
			cprintf("%s\n", mc8200[2]);
			recover = 0;
		}

		/* Cache parity...can be soft error.  Recover if no
		   state has been changed and another one hasn't happened
		   in last 1 sec. */
		if ((type & MC8200_DPARITY) != 0) {
			/* check to see if VAX state changed. */
			if (((mcf->mc8200_stat & VCR8200) != 0) &&
			    ((mcf->mc8200_summary & PFE8200) == 0)) {
				recover = 0;
				cprintf("%s\n", mc8200[3]);
			}
			/* is this happening often??? */
			if ((time.tv_sec - dparity8200) < MCHK_THRESHOLD) {
				recover = 0;
				cprintf("%s\n", mc8200[3]);
			}
			dparity8200 = time.tv_sec;
		        index = 3; /* to report error if recoverable */
		}

		/*  Can't recover from writes (PC will be past instruction).
		    try to recover if VAX CAN'T retry bit is clear. */
		if ((type & MC8200_BIERR) != 0) {
			if ((mcf->mc8200_summary & PFE8200) == 0) {
				/* check to see if VAX state changed. */
				if ((mcf->mc8200_stat & MEMWRITE) != 0 ||
				    (mcf->mc8200_stat & VCR8200) != 0) {
					recover = 0;
					cprintf("%s\n", mc8200[4]);
				}
			}
			/* is this happening often??? */
			if ((time.tv_sec - bierr8200) < MCHK_THRESHOLD) {
				recover = 0;
				cprintf("%s\n", mc8200[4]);
			}

			nxv->biic.biic_err = nxv->biic.biic_err;
			bierr8200 = time.tv_sec;
		        index = 4; /* to report error if recoverable */
		}

		/* BTB tag parity error...can be soft error.  Recover if no
		   state has been changed and another one hasn't happened
		   in last 1 sec. */
		if ((type & MC8200_BPARITY) != 0) {
			mtpr(TBIA, 0); /* clear bad TB entries! */
			/* check to see if VAX state changed. */
			if (((mcf->mc8200_stat & VCR8200) != 0) &&
			    ((mcf->mc8200_summary & PFE8200) == 0)) {
				recover = 0;
				cprintf("%s\n", mc8200[5]);
			}
			/* is this happening often??? */
			if ((time.tv_sec - bparity8200) < MCHK_THRESHOLD) {
				recover = 0;
				cprintf("%s\n", mc8200[5]);
			}
			bparity8200 = time.tv_sec;
		        index = 5; /* to report error if recoverable */
		}

		/* Cache tag parity error...can be soft error.  Recover if no
		   state has been changed and another one hasn't happened
		   in last 1 sec. */
		if ((type & MC8200_CPARITY) != 0) {
			/* check to see if VAX state changed. */
			if (((mcf->mc8200_stat & VCR8200) != 0) &&
			    ((mcf->mc8200_summary & PFE8200) == 0)) {
				recover = 0;
				cprintf("%s\n", mc8200[6]);
			}
			/* is this happening often??? */
			if ((time.tv_sec - cparity8200) < MCHK_THRESHOLD) {
				recover = 0;
				cprintf("%s\n", mc8200[6]);
			}
			cparity8200 = time.tv_sec;
		        index = 6; /* to report error if recoverable */
		}

		v8200port = v8200port;  /* clear portcontroller bits */

		logmck((int *)cmcf, ELMCKT_8200, mfpr(BINID), recover);
		if (recover == 0) {
			cprintf("\tsumpar\t= %x\n", mcf->mc8200_summary);
			cprintf("\tparm1\t= %x\n", mcf->mc8200_parm1);
			cprintf("\tva\t= %x\n", mcf->mc8200_va);
			cprintf("\tvap\t= %x\n", mcf->mc8200_vap);
			cprintf("\tmar\t= %x\n", mcf->mc8200_mar);
			cprintf("\tstatus\t= %x\n", mcf->mc8200_stat);
			cprintf("\tpc at failure\t= %x\n", mcf->mc8200_pcfail);
			cprintf("\tupc at failure\t= %x\n",mcf->mc8200_upcfail);
			cprintf("\ttrap pc\t= %x\n", mcf->mc8200_pc);
			cprintf("\ttrap psl\t= %x\n\n", mcf->mc8200_psl);
		}

		/* scan memory for additional errors and log */
		ka8200memerr();

		/* scan VAXBI for additional errors and log */
		log_bierrors(0, &mcf->mc8200_pc);
	}

	mtpr(MCESR,0);     /* clear condition flag */

	if (recover) 
           mprintf("MACHINE-CHECK RECOVERY OCCURED\ntype: %s",mc8200[index]);
	else 
 	   panic("mchk");
	

	return(0);
}


ka8200memerr ()
{
	int m;
	int merrtype = EL_UNDEF;
	struct bimem *mcr;
	struct el_rec *elrp;
	struct el_mem *mrp;

	for (m = 0; m < nmcr; m++) {
	    mcr = (struct bimem *)mcrdata[m].mcraddr;
	    if (((mcr->bimem_csr1 & (BI1_MERR|BI1_CNTLERR)) != 0) ||
	           ((mcr->bimem_csr2 & (BI1_RDS|BI1_CRDERR)) != 0)) {
	        elrp = ealloc(EL_MEMSIZE,EL_PRILOW);
	        if (elrp != NULL) {
		    LSUBID(elrp,ELCT_MEM,cpu_subtype,ELMCNTR_BI,EL_UNDEF,EL_UNDEF,EL_UNDEF);
		    if (mcr->bimem_csr2 & BI1_CRDERR)
			merrtype = 1;
		    else if (mcr->bimem_csr2 & BI1_RDS)
			merrtype = 2;
		    else if (mcr->bimem_csr1 & BI1_CNTLERR)
			merrtype = 3;
		    else if (mcr->bimem_csr1 & BI1_MERR)
			merrtype = 4;
		    mrp = &elrp->el_body.elmem;
		    mrp->elmem_cnt = 1;
		    mrp->elmemerr.cntl = mcr->bimem_biic.biic_ctrl & BICTRL_ID;
		    mrp->elmemerr.type = merrtype;
		    mrp->elmemerr.numerr = 1;
		    mrp->elmemerr.regs[0] = mcr->bimem_csr1;
		    mrp->elmemerr.regs[1] = mcr->bimem_csr2;
		    mrp->elmemerr.regs[2] = EL_UNDEF;
		    mrp->elmemerr.regs[3] = EL_UNDEF;
		    EVALID(elrp);
	        }
		mcr->bimem_csr1 = BI1_ICRD|BI1_MERR|BI1_CNTLERR;
		mcr->bimem_csr2 = mcr->bimem_csr2;
	    }
	    v8200port = (v8200port | V8200_CRDEN | V8200_CRDCLR ); 
	}
	return(0);
}

ka8200tocons(c)
	register int c;
{
	while ((mfpr (TXCS) & TXCS_RDY) == 0)
		continue;
	mtpr (TXDB, c);
	return(0);
}

/*
 * this routine sets the cache to the state passed.  enabled/disabled
 */

ka8200setcache(state)
int state;
{
	mtpr (CADR, state);
	return(0);
}

ka8200cachenbl()
{
	cache_state = 0;
	return(0);
}

/*
 * Memenable enables the memory controller corrected data reporting.
 * This runs at regular intervals, turning on the interrupt.
 * The interrupt is turned off, per memory controller, when error
 * reporting occurs.  Thus we report at most once per memintvl.
 */

ka8200memenable ()
{
	register struct bimem *mcr;
	register int m;

	for (m = 0; m < nmcr; m++) {
	    mcr = (struct bimem *)mcrdata[m].mcraddr;
	    bimemenable(mcr);
	}
	return(0);
}

ka8200nexaddr(ioadpt,nexnum) 
 	int ioadpt,nexnum;
{

	return((int)NEX8200(nexnum));

}

u_short *ka8200umaddr(ioadpt,ubanumber) 
 	int ubanumber,ioadpt;
{

	return(UMEM8200(ubanumber));

}

u_short *ka8200udevaddr(ioadpt,ubanumber) 
 	int ioadpt,ubanumber;
{

	return(UDEVADDR8200(ubanumber));

}


ka8200readtodr()
{
	u_int todr;
	char *v8200_lcl;	
	struct tm tm;
	int s;

	/*
	 * Copy the toy register contents into tm so that we can
	 * work with. The toy must be completely read in 2.5 millisecs.
	 *
	 *
	 * Wait for update in progress to be done.
	 */
	v8200_lcl = (char *) v8200watch;

	s = spl7();
	while( ((struct v8200watch *) v8200_lcl)->v8200_acsr & V8200_BUSY) ;
	tm.tm_sec = ((struct v8200watch *) v8200_lcl)->v8200_secs;	
	tm.tm_min = ((struct v8200watch *) v8200_lcl)->v8200_mins;	
	tm.tm_hour = ((struct v8200watch *) v8200_lcl)->v8200_hours;
	tm.tm_mday = ((struct v8200watch *) v8200_lcl)->v8200_mdays;
	tm.tm_mon = ((struct v8200watch *) v8200_lcl)->v8200_months;
	tm.tm_year = ((struct v8200watch *) v8200_lcl)->v8200_years;
	splx( s );

	tm.tm_sec = 0x0ff & ((tm.tm_sec << 7) | (tm.tm_sec >> 1));
	tm.tm_min = 0x0ff & ((tm.tm_min << 7) | (tm.tm_min >> 1));
	tm.tm_hour = 0x0ff & ((tm.tm_hour << 7) | (tm.tm_hour >> 1));
	tm.tm_mday = 0x0ff & ((tm.tm_mday << 7) | (tm.tm_mday >> 1));
	tm.tm_mon = 0x0ff & ((tm.tm_mon << 7) | (tm.tm_mon >> 1));
	tm.tm_year = 0x0ff & ((tm.tm_year << 7) | (tm.tm_year >> 1));

	todr = toyread_convert(tm);

	return(todr);
}
ka8200writetodr(yrtime)
u_int yrtime;
{
	char   *v8200_lcl;
	struct tm xtime;
	int s;

	toywrite_convert(&xtime,yrtime);

	v8200_lcl = (char *) v8200watch;

	((struct v8200watch *) v8200_lcl)->v8200_bcsr = V8200_SETUP;
	s = spl7();
	((struct v8200watch *) v8200_lcl)->v8200_secs = 0x0ff & (xtime.tm_sec << 1 | xtime.tm_sec >>7);
	((struct v8200watch *) v8200_lcl)->v8200_mins = 0x0ff & (xtime.tm_min << 1 | xtime.tm_min >>7);
	((struct v8200watch *) v8200_lcl)->v8200_hours = 0x0ff & (xtime.tm_hour << 1 | xtime.tm_hour >>7);
	((struct v8200watch *) v8200_lcl)->v8200_mdays = 0x0ff & (xtime.tm_mday << 1 | xtime.tm_mday >>7);
	((struct v8200watch *) v8200_lcl)->v8200_months = 0x0ff & (xtime.tm_mon << 1 | xtime.tm_mon >>7);
	((struct v8200watch *) v8200_lcl)->v8200_years = 0x0ff & (xtime.tm_year << 1 | xtime.tm_year >>7);
	/*
 	* Start the clock again.
 	*/
	((struct v8200watch *) v8200_lcl)->v8200_acsr = V8200_ASET;
	((struct v8200watch *) v8200_lcl)->v8200_bcsr = V8200_BSET;
	splx( s );


}


struct v8200rx50 v8200rx50[1];
extern struct rx5tab rx5tab;

ka8200rxopen(unit)
int unit;
{

	if (BADADDR(&v8200rx50->rx5cs0,1)) {
		return(1);
	}
	return(0);
}

ka8200startrx() {

register u_short  sector, track, command, dn, junk;

	/* drive is interleved */
	track = rx5tab.rx5blk/ 10 ;
	sector = (((rx5tab.rx5blk % 10)/5) + ((rx5tab.rx5blk+track)*2)) % 10;

	sector++ ;

    	if( ++track > 79 ) track = 0 ;

	dn = (minor(rx5tab.rx5_buf->b_dev)>>3) & 07;
	dn=dn-1;
	command = (dn & 01) << 1 ;

	if (rx5tab.rx5_buf->b_flags & B_READ ) {
		command |= READ ;
		junk = v8200rx50->rx5ca;
	} else {
		command |= WRITE;
		cs_transfer(rx5tab.rx5addr,FILL);
	}

	v8200rx50->rx5cs1 = track;
	v8200rx50->rx5cs2 = sector;
	v8200rx50->rx5cs0 = command&0x7f;
	junk = v8200rx50->rx5go; 
}


rx5_intr()
{
	int junk;
	
	/* if not busy...why did we interrupt */
	if((rx5tab.rx5_state & RX5BUSY) == 0 ) return;

	if((v8200rx50->rx5cs0 & RX50_DONE) ==0 ) return;

	if(v8200rx50->rx5cs0 & ERROR ) {
		printf("hard error cs%x\n",(rx5_unit+1));
		mprintf("cs* = %x, %x, %x, %x, %x\n",
			v8200rx50->rx5cs0,v8200rx50->rx5cs1,
			v8200rx50->rx5cs2,v8200rx50->rx5cs3,
			v8200rx50->rx5cs4);
    		mprintf("Reset drive\n");

		v8200rx50->rx5cs0 = RESTORE ;
		junk = v8200rx50->rx5go; 	
		rx5tab.rx5_buf->b_error = EIO;
		rx5tab.rx5_buf->b_flags |= B_ERROR;
		rx5tab.rx5_state &= ~ RX5BUSY;
		iodone(rx5tab.rx5_buf);	
		wakeup((caddr_t)&rx5tab);
		return;
	}

	/* store data */
	if (rx5tab.rx5_buf->b_flags & B_READ) 
		cs_transfer(rx5tab.rx5addr,EMPTY);

	
	if ( rx5tab.rx5resid > 512)
		rx5tab.rx5resid -= 512;
	else
		rx5tab.rx5resid = 0;
	rx5tab.rx5addr += 512;
	rx5tab.rx5blk++;


	if (rx5tab.rx5resid > 0) 
		cs_start();		/* more to do */
	else {
		iodone(rx5tab.rx5_buf);	/* all done */
		rx5tab.rx5_state &= ~ RX5BUSY;
		wakeup((caddr_t)&rx5tab);/* wakeup anyone waiting for drive */
	}
}       




cs_transfer(bpp,op)
	char *bpp;
	short   op;
{
	register short  nbytes;
	register char   *buf, *buf1;
	int junk,s;


	nbytes = v8200rx50->rx5ca;                   
	buf  = bpp;

	if (rx5tab.rx5resid >=512)
		nbytes = 512;
	else
		nbytes=rx5tab.rx5resid;

	if (op == FILL ) {
		/* FILL for floppy write */
		buf1 = (char *)&v8200rx50->rx5fdb;
		while(nbytes) {
			*buf1 = *buf++ ;
			--nbytes;
		}
	} else { 
		/* Empty for floppy read */
		buf1 = (char *)&v8200rx50->rx5edb;
		while(nbytes){
			*buf++ = *buf1;
			--nbytes;
		}

	}

}

