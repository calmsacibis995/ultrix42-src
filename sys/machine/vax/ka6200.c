#ifndef lint
static char *sccsid = "@(#)ka6200.c	4.3	ULTRIX	2/28/91";
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
 * 26-Feb-91  jas
 *	Mapped in XMI nodespace in ka6200conf().  Need to be able to
 *	access nodespace.
 *
 * 18-Jun-90  jas
 *      added support for XMA2 to xma routines.
 *
 * 30-Mar-90  jaw
 *	recover from hard errors on ident command (passive release).
 *
 * 29-Nov-89    Paul Grist
 *      modified ka6200machcheck() to call log_xmi_bierrors and log_xmierrors
 *      to look for any pending XBI or VAXBI errors. Added log_ka6200memerrs
 *      for outside callers to log xma errors (for log_bierrors).
 *        
 * 10-Nov-89	jaw
 *	move kmalloc of attach processors machdep structure to cca_startcpu.
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
 * 19-Aug-88	Tom Kong.  
 *		Changed ka6200machcheck so that instead of panicking,
 *		we try to simply terminating the user process that
 *		caused the machine check.  For several cases of machine
 *		checks, the instruction is simply retried and no process
 *		is killed.  Note that this only happens if KILL_USER is
 *		defined.  Otherwise the OS is killed.
 *
 *		Took out "extern int (*vax8800bivec[])()" since it isn't
 *		referenced in this file.
 *
 * 22-Jul-88	darrell Moved cca_send() and cca_setup() from this file
 *			to cvax.c so that it can be shared by VAX60
 *			(Firefox) machine dependent code.
 *
 * 07-Jun-88	darrell Removed ka650.h from include files and added
 *		        cvax.h.
 *
 * 26-Apr-88	jaw	fix to mcheck handler.
 *
 * 04-Feb-88	jaw 	work on soft error handling.
 *
 * 28-Jan-88	jaw 	Add logging for CRD's
 *
 **********************************************************************/

#include "../machine/pte.h"
#include "../h/param.h"
#include "../h/conf.h"
#include "../h/time.h"
#include "../h/errno.h"
#include "../h/systm.h"
#include "../h/types.h"
#include "../h/map.h"
#include "../h/buf.h"
#include "../h/errlog.h"
#include "../h/ioctl.h"
#include "../h/tty.h"
#include "../h/cpudata.h"
#include "../../machine/common/cpuconf.h"
#include "../h/kmalloc.h"
#include "../h/vmmac.h"

#include "../machine/cons.h"
#include "../machine/cons.h"
#include "../machine/cpu.h"
#include "../machine/clock.h"
#include "../machine/mtpr.h"
#include "../machine/mem.h"
#include "../machine/nexus.h"
#include "../machine/scb.h"
#include "../io/uba/ubareg.h"
#include "../io/uba/ubavar.h"
#include "../io/bi/buareg.h"
#include "../machine/sas/vmb.h"
#include "../machine/ka6200.h"
#include "../machine/cvax.h"

#include "../io/xmi/xmireg.h"
#include "../io/xmi/xmareg.h"

/*#define KILL_USER */
#ifdef KILL_USER
/*
 * Need u symbols, ...etc to terminate user process
 */
#include "../h/user.h"	
#include "../h/proc.h"
#include "../machine/psl.h"
#endif

extern struct bidata bidata[];
extern int cache_state;
extern int nNVAXBI;
extern char *ka6200_ip_addr;

struct ssc_regs cvqssc[1];		/* SSC regs */
struct v6200csr v6200csr[1];
char *mccvax[] = {
	"unknown machine check type code",		/* 0 */
	"CFPA protocol error",				/* 1 */
	"CFPA reserved instruction",			/* 2 */
	"CFPA protocol error",				/* 3 */
	"CFPA protocol error",				/* 4 */
	"process PTE in P0 space during TB miss",	/* 5 */
	"process PTE in P1 space during TB miss",	/* 6 */
	"process PTE in P0 space during M = 0",		/* 7 */
	"process PTE in P1 space during M = 0",		/* 8 */
	"hardware interrupt at unused IPL",		/* 9 */
	"undefined MOVC3 or MOVC5 state",		/* 10 */
	"cache/memory/bus read error",			/* 80 */
	"SCB, PCB or SPTE read error",			/* 81 */
	"cache/memory/bus write error",			/* 82 */
	"PCB or SPTE write error",			/* 83 */
};



/*
 * KA6200 machine check exception handler.
 * Environment:
 *	IPL		0x1f
 *	stack		Interrupt stack
 *	SCB vector <1:0> = 01.
 *
 * Parameter:
 *	mcf 		Points to machine check frame.  For Ka6200, the
 *			frame format:
 *				byte count	(0x10)
 *				mcheck code (1,2,3,4,5,6,7,8,9,a,80,81,82,or83)
 *				internal state info 1
 *				internal state info 2
 *				PC
 *				PSL
 * Returns:
 *	0
 */
ka6200machcheck (mcf)
register struct el_mc6200frame *mcf;
{
	register int cpunum;
	register struct xcp_reg *xcp_node;
	int xminode, recover;
	struct xmidata *xmidata;
	register struct el_mc6200frame *framep;
	register struct el_rec *elrp;
	struct el_mck *elmckp;
	register unsigned int mcode;
	long	time_now;
	struct	xcp_machdep_data *mchk_data; /* Pointer to an array		   */
	
	mchk_data =(struct xcp_machdep_data *) ( CURRENT_CPUDATA->cpu_machdep);

	time_now = mfpr(TODR);		/* Log the current time	*/

	recover = 0;			/* Assume we have to panic */

	cpunum=CURRENT_CPUDATA->cpu_num;

	if (mchk_data->mchk_in_progress == 0) {
		/* if no machine check in progress, we're ok */
		mchk_data->mchk_in_progress++;
		}
	else	{
		/* mcheck on another mcheck, we're in big trouble	*/
		asm("halt");
	}


	/*
	 * Case of the type of machine check, we may be able
	 * to recover from the machine check under certain conditions.
	 */
	mcode = ((struct mcframe *)mcf)->mc_summary;

	switch (mcode) {
	case 1:	/* CFPA protocol error */
	case 2:	/* CFPA reserved instruction */
	case 3:	/* CFPA unknown error */
	case 4:	/* CFPA unknown error */
	case 5:	/* calculated virtual address of process PTE in P0 space */
		/* TB miss flow */
	case 6:	/* calculated virtual address of process PTE in P1 space */
		/* TB miss flow */
	case 7:	/* calculated virtual address of process PTE in P0 space */
		/* M = 0 flow */
	case 8:	/* calculated virtual address of process PTE in P1 space */
		/* M = 0 flow */
	case 10:/* Impossible situations in microcode */

#ifdef KILL_USER
		/* 
		 * The above errors are not recoverable.  If we
		 * are running a user process, the user process should
		 * be killed. If we are running the OS, panic.
		 *
		 * Note: we assume we were running a user process during
		 * the machine check if the saved PSL on the stack indicate
		 * a current mode of USER, and that the process isn't "init".
		 */
		if (USERMODE(mcf->mc1_psl) && (u.u_procp->p_pid != 1)) {
			/* kill the user process */
			swkill(u.u_procp, "ka6200machcheck");
			recover = 1;	/* don't have to panic */
		}
#endif
		break;
		
			
	case 0x80: /* read bus error, normal read */
	case 0x81: /* read bus error, SPTE, PCB, or SCB read */
		 
		/*
		 * The instruction is restartable for the above errors if:
		 * 1) The saved PSL on the stack has FPD bit set, or
		 * 2) The VAX CAN'T RESTART bit on the mcheck frame is clear.
		 */
		if (((mcf->mc1_psl & 0x08000000) == 0x08000000)	||
		    ((mcf->mc1_internal_state2 & 0x8000) == 0)) {
			/*
			 * We can restart the instruction.
			 * Decide to allow restart if one or more of the 
			 * following is satisfied:
			 * 1) previous machine check was long ago.
			 * 2) previous machine check was different.
			 */
			if (time_now - mchk_data->time > 1000) {
				/* last mcheck was at least 10 secs ago */
				recover = 1;
			}

			if (mchk_data->code != mcf->mc1_summary)
				recover = 1;

			/*
			 * if recover == 1 (we are retrying the instruction)
			 * we should (in future) map out the bad physical 
			 * page here.
			 */
		}
		if (!recover)	{
			/*
			 * The instruction is not restartable, we
			 * need to kill the user process or the operating
			 * system.
			 */
#ifdef KILL_USER
			if (USERMODE(mcf->mc1_psl) && (u.u_procp->p_pid !=1)) {
				swkill(u.u_procp,"ka6200machcheck");
				recover = 1; /* kill user, but don't panic */
				/*
			 	 * In addition to killing the user process,
			 	 * we should map out the bad physical page in
			 	 * the future.  Do it here.
				 */
			}
#endif
		}
		break;
	case 0x82: /* write bus error, normal write */
	case 0x83: /* write bus error, SPTE or PCB write */
		/*
		 * The instruction is not restartable, we
		 * need to kill the user process or the operating
		 * system.
		 */

#ifdef KILL_USER
		if (USERMODE(mcf->mc1_psl) && (u.u_procp->p_pid !=1)) {
			swkill(u.u_procp,"ka6200machcheck");
			recover = 1; /* kill user, but don't panic */
			/*
			 * In addition to killing the user process,
			 * we should map out the bad physical page in
			 * the future.  Do it here.
			 */
		}
#endif
		break;
	case 9:	/* Interrupt controller requests an interrupt at an unused */
		/* IPL (IPL 0x18,0x19,and 0x1b)				   */
	default:   /* Unknown machine check type code */
		/* Panic on the above errors */
		break;
	}


	/*
	 * Log the machine check in the error log.
	 */
	xmidata = get_xmi(0);/* get pointer to xcp node that machine checked */
	xcp_node =(struct xcp_reg *)xmidata->xmivirt+(v6200csr->csr1 & 0xf);
	/* allocate a error log packet */
	elrp = ealloc((sizeof(struct el_mc6200frame)),
		recover ? EL_PRIHIGH:EL_PRISEVERE);

	if (elrp) {
		LSUBID(elrp,ELCT_MCK,ELMCKT_6200,cpu_subtype,cpunum,EL_UNDEF,
			mcf->mc1_summary);
		elmckp = &elrp->el_body.elmck;
		framep = (struct el_mc6200frame *) &elmckp->elmck_frame.
				el6200mcf.mc1_bcnt;
		framep->mc1_bcnt = mcf->mc1_bcnt;
		framep->mc1_summary = mcf->mc1_summary;
		framep->mc1_vap = mcf->mc1_vap;
		framep->mc1_internal_state1 = mcf->mc1_internal_state1;
		framep->mc1_internal_state2 = mcf->mc1_internal_state2;
		framep->mc1_pc = mcf->mc1_pc;
		framep->mc1_psl = mcf->mc1_psl;
		framep->xcp_dtype = xcp_node->xcp_dtype;
		framep->xcp_xbe = xcp_node->xcp_xbe;
		framep->xcp_csr2 = xcp_node->xcp_csr2;
		framep->xcp_csr1 = v6200csr->csr1;
		framep->xcp_mser = mfpr(MSER);
		EVALID(elrp);	/* Make error log packet valid */
	}

	/* look for any pending XMI errors */

	log_xmierrors(0,mcf->mc1_pc);
	log_xmi_bierrors(0,mcf->mc1_pc);


	/* 
	 * if not recovering then we print the machine check frame to 
	 * console device, and then panic.  
	 */
	if (!recover) {
		cprintf("cpu %x ",(v6200csr->csr1 & 0xf));
		if ( mcode > 10) {
			mcode = mcode - 0x75;
			if (mcode >14) mcode = 0;
	        } 
		cprintf("%s\n", mccvax[mcode]);
		cprintf("\tcode\t\t= %x\n", mcf->mc1_summary);
		cprintf("\tmost recent virtual addr\t=%x\n", mcf->mc1_vap);
		cprintf("\tinternal state 1\t=%x\n", mcf->mc1_internal_state1);
		cprintf("\tinternal state 2\t=%x\n", mcf->mc1_internal_state2);
		cprintf("\tpc\t\t= %x\n", mcf->mc1_pc);
		cprintf("\tpsl\t\t= %x\n\n", mcf->mc1_psl);
	
		xma_check_errors(xcp_node,EL_PRISEVERE);
		panic("mchk");
		} 
	else 	{
		/* We are trying to recover */
		ka6200_enable_cache();
		/* 
		 * Log the contents of the machine check frame 
		 * Note that we don't have to log the contents if we are
		 * to panic.  This info is used only if we don't crash.
		 */
		mchk_data->time = time_now;
		mchk_data->code = mcf->mc1_summary;
		mchk_data->maddr = mcf->mc1_vap;
		mchk_data->istate1 = mcf->mc1_internal_state1;
		mchk_data->istate2 = mcf->mc1_internal_state2;
		mchk_data->pc = mcf->mc1_pc;
		mchk_data->psl = mcf->mc1_psl;
	}
		

	/* clean out error and retry */
	mtpr(MSER, mfpr(MSER));
	xcp_node->xcp_csr2 = xcp_node->xcp_csr2;
	xcp_node->xcp_xbe = xcp_node->xcp_xbe;

	/*
	 * when we get here, we are not crashing the system.
	 * clear flag to indicate we're done handling the machine check.
	 */

	if ( mcode > 10) {
		mcode = mcode - 0x75;
		if (mcode >14) mcode = 0;
        } 
	printf("MACHINE CHECK RECOVERY occured\ntype: %s\n", mccvax[mcode]);

	mchk_data->mchk_in_progress = 0;	
	return(0);	
}


/* initialization code that the slave processor must run
   before starting up */

ka6200initslave() {
	struct xcp_reg *xcp_node;
	register struct xmidata *xmidata ;

	xmidata = get_xmi(0);
	xcp_node =(struct xcp_reg *)xmidata->xmivirt+(v6200csr->csr1 & 0xf);

	xcp_node->xcp_csr2 = xcp_node->xcp_csr2;
	xcp_node->xcp_xbe = xcp_node->xcp_xbe;

	ka6200_enable_cache() ;

}

ka6200conf()
{
	char *nxv;
	int 	nxp;

	extern struct xmi_reg xmi_start[];
	register int i;
	register int xcp_node;
	register struct xmidata *xmidata;
	struct xcp_reg *xcp;
	char *start;
	struct pte *pte;
	struct xcp_reg *nxvirt;
	int cpu_subtype;
	union cpusid cpusid;
	cpusid.cpusid = mfpr(SID);

	/* allocate up 1 xmi structure */
	KM_ALLOC(xmidata,struct xmidata *,sizeof(struct xmidata ),KM_DEVBUF,KM_NOW_CL_CO_CA);

	head_xmidata = xmidata;
	xmidata->next = 0;
	xmidata->xminum = 0;

	/* fill in table of IP interrupt addresses.  Addresses 
	   are of the form:

		2101nnnn  where nnnn is the decoded XMI node number
		
	*/

	start = (char *) &ka6200_ip_addr;

	for (i=0 ; i< MAX_XMI_NODE ; i++ ) {
		nxp = 0x21010000 + (1<<i);
		if (i < 9)
			nxv =  start + (1<<i);
		else {
			nxv = start + ((i-8)*NBPG);
		}
		ka6200_ip[i]=nxv;
		nxaccess(nxp,&Sysmap[btop((int)(nxv) & ~VA_SYS)],512);
	}

	nxaccess (CVQSSCADDR, CVQSSCmap, CVQSSCSIZE);

	xcp_node =v6200csr->csr1 & 0xf;
	xmidata->xmiintr_dst = 1<<xcp_node;
	ka6200_enable_cache() ;
	
	xmidata->xmiphys = (struct xmi_reg *) XMI_START_PHYS;
	xmidata->xmivec_page = &scb.scb_stray;
    
	cpu_avail  = cca_setup() + 1;
	
	spl0();  			/* Ready for interrupts */

	/* Allocate memory to store machine check information */
	KM_ALLOC((CURRENT_CPUDATA->cpu_machdep), char *,
		sizeof(struct xcp_machdep_data), KM_MBUF,KM_CLEAR | KM_CONTIG);

	/* test for type of XCP */
	if ((cpu_systype & 0xff00) == 0x200)
		printf("VAX63%d0",cpu_avail);
	else 
		printf("VAX62%d0",cpu_avail);

	/* test for time share verses server */
	if ((cpu_systype & 0xff) == 1) 
		printf(" time-share\n");
	else
		printf(" compute-server\n");

	/*
	 * Map the node space of the XCP so that the processor
	 * can get to its node space CSRs.
	 */
	xmidata = get_xmi(0);
	xmidata->xmivirt = xmi_start;
	nxp = ka6200nexaddr(0,xcp_node);
	nxvirt = (struct xcp_reg *)xmidata->xmivirt + xcp_node;
	nxaccess(nxp,&Sysmap[btop((int)(nxvirt) & ~VA_SYS)],1024);

	spl0();  			/* Ready for interrupts */
	
	xmiconf(0);		/* Config all XMI, BI, ..etc devices*/
	
	/* clear warm and cold boot flags */
	ccabase.cca_hflag &= ~(CCA_V_BOOTIP|CCA_V_WARMIP);
	return(0);
}


ka6200_enable_cache() {

	int save_led_val;

	/* enable the cache */
	mtpr(CADR, 0xec);  	/* first level */

	save_led_val = v6200csr->csr1 & 0x1ffff; /* save led values */

	/* now enable cache by method specified in XCP spec */
	v6200csr->csr1 = 0x00040000|save_led_val;	/* force miss */
	v6200csr->csr1 = 0x00140000|save_led_val;	/* invalidate */
	v6200csr->csr1 = 0|save_led_val;		/* enable cache */

}


ka6200mapcsr() {
	nxaccess(CSRV6200,V6200csr,512);
}

/*
 *	ka6200memerr  ---  Hard errors are reported through SCB vector
 *	0x60, at IPL 0x1d.  These errors include the following:
 *	This routine runs on the interrupt stack.
 *		
 *		XBE:WDNAK  -- write data noAck.
 *		XBE:TE	   -- Transmit Error.
 *		XBE:CNAK   -- Command NoAck.
 *		CSR1:WDPE  -- DAL write Parity Error.
 *		XBE:WEI    -- XMI write Error IVINTR's.
 * 		XBE:XFAULT -- XMI fault assertion.
 *		All forms of Ident errors (which bits?).
 */

ka6200memerr()
{
	int cpunum;
	struct xcp_reg *xcp_node;
	int save_led_val;
	int xminode;
	struct xmi_reg	*nxv;	/* virtual pointer to XMI node */
	register struct xmidata *xmidata;
	struct el_xcp60 *ptr;
	struct el_rec *elrp;

	xmidata = get_xmi(0);
	xcp_node =(struct xcp_reg *)xmidata->xmivirt+(v6200csr->csr1 & 0xf);
 	elrp = ealloc(sizeof(struct el_xcp60),EL_PRILOW);
	
	if (elrp != NULL) {
		LSUBID(elrp,ELCT_6200_INT60,EL_UNDEF,EL_UNDEF,EL_UNDEF,EL_UNDEF,EL_UNDEF);

		/* load vector 60 registers */
		ptr = &elrp->el_body.el_xcp60;
		ptr->xcp_csr1 = v6200csr->csr1;
		ptr->xcp_cadr = mfpr(CADR);
		ptr->xcp_mser = mfpr(MSER);

		ptr->xcp_dtype = xcp_node->xcp_dtype;
		ptr->xcp_xbe = xcp_node->xcp_xbe;
		ptr->xcp_fadr = xcp_node->xcp_fadr;
		ptr->xcp_gpr = xcp_node->xcp_gpr;
		ptr->xcp_csr2 = xcp_node->xcp_csr2;
		
  	        EVALID(elrp);
	}
   	if (   (xcp_node->xcp_xbe & (XMI_RER|XMI_NRR)) && ((xcp_node->xcp_xbe & 0xf)==9)) {
		ka6200_clear_xbe();
		return(0);
	}
	/* we are crashing so print out error */
	cprintf("Fatal error detected by XCP at node %x\n",v6200csr->csr1&0xf);
	cprintf("cadr      = %x\n",mfpr(CADR));
	cprintf("mser 	   = %x\n",mfpr(MSER));
	cprintf("xcp_dtype = %x\n",xcp_node->xcp_dtype);
	cprintf("xcp_xbe   = %x\n",xcp_node->xcp_xbe);
	cprintf("xcp_fadr  = %x\n",xcp_node->xcp_fadr);
	cprintf("xcp_csr2   = %x\n",xcp_node->xcp_csr2);

	/* print out any pending XMA errors */
	xma_check_errors(xcp_node,EL_PRIHIGH);

	/* kill system */
	panic("memory error");

	return(0);
}


log_ka6200memerrs()

/*
 * function: to log xma errors from outside exception handlers,
 *           for example, log_bierrors, to get a more complete
 *           picture of the machine at error time.
 */

{
	struct xcp_reg *xcp_node;
	struct xmi_reg	*nxv;	/* virtual pointer to XMI node */
	register struct xmidata *xmidata;


	xmidata = get_xmi(0);
	xcp_node =(struct xcp_reg *)xmidata->xmivirt+(v6200csr->csr1 & 0xf);

	/* log any pending XMA errors */
	xma_check_errors(xcp_node,EL_PRILOW);

	return(0);
}

xma_check_errors(xcp,priority)
struct xcp_reg *xcp;
int priority;
{
	struct el_xma *xma_pkg;
	struct xmi_reg *nxv;
	struct xma_reg *xma;
	struct xmidata *xmidata;
	int xminode;
	struct el_rec *elrp;
	
	xmidata = (struct xmidata *)get_xmi(0);
	nxv = xmidata->xmivirt;
	for(xminode = 0; xminode < MAX_XMI_NODE; xminode++,nxv++) {
	  if ((xmidata->xminodes_alive & (1<<xminode)) &&
	      ((short) (nxv->xmi_dtype) == XMI_XMA)) {
	    /* The node is a memory */
	    xma = (struct xma_reg *) nxv;
	    if (nxv->xmi_dtype == XMI_XMA) {
	      if ((xma->xma_xbe & XMI_ES)  ||
		  (xma->xma_mctl1 & (XMA_CTL1_LOCK_ERR|
		    XMA_CTL1_UNLOCK_ERR|XMA_CTL1_RDS_WRITE)) ||
		  (xma->xma_mecer & (XMA_ECC_RDS_ERROR))) {
		log_mem_error(xcp,xma,priority);
	      }
	    }
	    else /* Must be XMA2 */
	      if (xma->xma_xbe & XMI_ES)
		log_mem_error(xcp,xma,priority);
	  }	
	}
}

ka6200_clear_xbe()
{
	struct xmidata *xmidata;
	struct xcp_reg *xcp_node;

	/* get pointer to xcp node that machine checked */
	xmidata = get_xmi(0);
	xcp_node =(struct xcp_reg *)xmidata->xmivirt+(v6200csr->csr1 & 0xf);
	
	xcp_node->xcp_xbe = xcp_node->xcp_xbe & ~XMI_XBAD;
	
}

/* log XMA memory error.  If priority is not LOW then print to 
   console.
*/
log_mem_error(xcp,xma,priority)
	int priority;
	struct xma_reg *xma;
	struct xcp_reg *xcp;
{
	struct el_xma *xma_pkg;
	struct xmidata *xmidata;
	struct el_rec *elrp;
	
	
	/* If xma2, go to its handler */
	if((xma->xma_type & XMA2_MASK) == XMI_XMA2)
	  log_xma2_mem_error(xma,priority);
	else {
	/* log the XMA error */
	elrp = ealloc(sizeof(struct el_xma),priority);
	if (elrp) {
		LSUBID(elrp,ELCT_MEM,EL_UNDEF,ELMCNTR_6200,EL_UNDEF,EL_UNDEF,EL_UNDEF);
		xma_pkg = &elrp->el_body.el_xma;
		xma_pkg->xma_node = ((svtophy(xma))>>19)&0xf;
		xma_pkg->xma_dtype = xma->xma_type;
		xma_pkg->xma_xbe = xma->xma_xbe;
		xma_pkg->xma_seadr = xma->xma_seadr;
		xma_pkg->xma_mctl1 = xma->xma_mctl1;
		xma_pkg->xma_mecer = xma->xma_mecer;
		xma_pkg->xma_mecea = xma->xma_mecea;
		xma_pkg->xma_mctl2 = xma->xma_mctl2;
					
  	
     		EVALID(elrp);
	}
	/* print to console if HIGH priority */
	if (priority < EL_PRILOW) {
		cprintf("Fatal memory error \n");
		cprintf("xma_phys = %x\n",svtophy(xma));
		cprintf("xma_type = %x\n",xma->xma_type);
		cprintf("xma_xbe = %x\n",xma->xma_xbe);
		cprintf("xma_seadr = %x\n",xma->xma_seadr);
		cprintf("xma_mctl1 = %x\n",xma->xma_mctl1);
		cprintf("xma_mecer = %x\n",xma->xma_mecer);
		cprintf("xma_mecea = %x\n",xma->xma_mecea);
		cprintf("xma_mctl2 = %x\n",xma->xma_mctl2);
				
	}
      } /* END IF-ELSE */
}

/* 
 * log XMA2 memory error.  If priority is not LOW then print to 
 * console.
 */
log_xma2_mem_error(xma2,priority)
int priority;
register struct xma_reg *xma2;
{
	register struct el_xma2 *xma_pkg;
	register struct el_rec *elrp;
	
	/* log the XMA2 error */
	elrp = ealloc(sizeof(struct el_xma2),priority);
	if (elrp) {
                LSUBID(elrp,ELCT_MEM,EL_UNDEF,ELMCNTR_XMA2,EL_UNDEF,EL_UNDEF,EL_UNDEF);
		xma_pkg = &elrp->el_body.el_xma2;
		xma_pkg->xma_node = ((svtophy(xma2))>>19)&0xf;
		xma_pkg->xma_dtype = xma2->xma_type;
		xma_pkg->xma_xbe = xma2->xma_xbe;
		xma_pkg->xma_seadr = xma2->xma_seadr;
		xma_pkg->xma_mctl1 = xma2->xma_mctl1;
		xma_pkg->xma_mecer = xma2->xma_mecer;
		xma_pkg->xma_mecea = xma2->xma_mecea;
		xma_pkg->xma_mctl2 = xma2->xma_mctl2;
		xma_pkg->xma_becer = xma2->xma_becer;
		xma_pkg->xma_becea = xma2->xma_becea;
		xma_pkg->xma_stadr = xma2->xma_stadr;
		xma_pkg->xma_enadr = xma2->xma_enadr;
		xma_pkg->xma_intlv = xma2->xma_intlv;
		xma_pkg->xma_mctl3 = xma2->xma_mctl3;
		xma_pkg->xma_mctl4 = xma2->xma_mctl4;
		xma_pkg->xma_bsctl = xma2->xma_bsctl;
		xma_pkg->xma_bsadr = xma2->xma_bsadr;
		xma_pkg->xma_eectl = xma2->xma_eectl;
		xma_pkg->xma_tmoer = xma2->xma_tmoer;
     		EVALID(elrp);
	}
	/* print to console if HIGH priority */
	if (priority < EL_PRILOW) {
		cprintf("Memory error \n");
		cprintf("xma_phys = %x\n",svtophy(xma2));
		cprintf("xma_type = %x\n",xma2->xma_type);
		cprintf("xma_xbe = %x\n",xma2->xma_xbe);
		cprintf("xma_seadr = %x\n",xma2->xma_seadr);
		cprintf("xma_mctl1 = %x\n",xma2->xma_mctl1);
		cprintf("xma_mecer = %x\n",xma2->xma_mecer);
		cprintf("xma_mecea = %x\n",xma2->xma_mecea);
		cprintf("xma_mctl2 = %x\n",xma2->xma_mctl2);
		cprintf("xma_becer = %x\n", xma2->xma_becer);
		cprintf("xma_becea = %x\n", xma2->xma_becea);
		cprintf("xma_stadr = %x\n", xma2->xma_stadr);
		cprintf("xma_enadr = %x\n", xma2->xma_enadr);
		cprintf("xma_intlv = %x\n", xma2->xma_intlv);
		cprintf("xma_mctl3 = %x\n", xma2->xma_mctl3);
		cprintf("xma_mctl4 = %x\n", xma2->xma_mctl4);
		cprintf("xma_bsctl = %x\n", xma2->xma_bsctl);
		cprintf("xma_bsadr = %x\n", xma2->xma_bsadr);
		cprintf("xma_eectl = %x\n", xma2->xma_eectl);
		cprintf("xma_tmoer = %x\n", xma2->xma_tmoer);
	}
}

/*
 *  ka6200crderr() --- "Soft" errors are reported through SCB vector
 *  0x54, at IPL 0x1a.  These errors include the following:
 *  This routine runs on the interrupt stack.
 *
 *		XBE:CRD    - Correctable main memory errors.
 *
 * 	2nd Level cache errors.
 *		CSR2:VBPE  - Valid bit parity error.
 *		CSR2:TPE   - Tag parity error.
 *		CSR2:IQO   - Invailidate Queue Overflow.
 *		CSR2:DTPE  - Duplicate Tag Store Parity Error.
 *		CSR2:CFE   - Cache Fill Error.,
 *		
 *	XMI soft errors.
 *		XBE:CC	   - Corrected conformation Errors.
 *		XBE:IPE    - Inconsistent Parity Error.
 *		XBE:PE	   - Parity Error.
 */

ka6200crderr()
{
	struct xcp_reg *xcp_node;
	int save_led_val;
	register struct xmidata *xmidata ;
	int node;
	struct xcp_machdep_data *xcpdata;

	register struct el_xcpsoft *sptr;
	register int csr2_errs,xbe_errs, flush_cache=0;

	xmidata = get_xmi(0);
	node = v6200csr->csr1 &0xf;

	xcpdata = (struct xcp_machdep_data *) CURRENT_CPUDATA->cpu_machdep;

	sptr = &xcpdata->xcpsoft;


	xcp_node =(struct xcp_reg *)xmidata->xmivirt+(v6200csr->csr1 & 0xf);
	
	csr2_errs = xcp_node->xcp_csr2;
	xbe_errs = xcp_node->xcp_xbe;

	xcp_node->xcp_csr2 = xcp_node->xcp_csr2;
	xcp_node->xcp_xbe = xcp_node->xcp_xbe;

	/* test for IQO error -- count and dismiss */
	if (csr2_errs & CSR2_IQO) {
		if (sptr)
			sptr->xcp_iqo++;
		flush_cache++;
	} 

	/* test for cache error */
	if (csr2_errs&(CSR2_VBPE|CSR2_TPE|CSR2_CFE|CSR2_DTPE)) {
		if (sptr) {
			if (csr2_errs & CSR2_VBPE) sptr->xcp_vbpe++;
			if (csr2_errs & CSR2_TPE) sptr->xcp_tpe++;
			if (csr2_errs & CSR2_CFE) sptr->xcp_cfe++;
			if (csr2_errs & CSR2_DTPE) sptr->xcp_dtpe++;
			if (CURRENT_CPUDATA->cpu_state & CPU_SOFT_DISABLE==0)
				xcp_log_soft(xcp_node,sptr);
		}
		flush_cache++;
	}

    	/* test for XMI error */
	if (xbe_errs & (XMI_CC|XMI_IPE|XMI_PE)) {

		if (sptr) {
			if (xbe_errs & XMI_CC) sptr->xcp_cc++;
			if (xbe_errs & XMI_IPE) sptr->xcp_ipe++;
			if (xbe_errs & XMI_PE) sptr->xcp_pe++;
			if (CURRENT_CPUDATA->cpu_state& CPU_SOFT_DISABLE==0)
				xcp_log_soft(xcp_node,sptr);
		}
		flush_cache++;
	} 

	/* test for CRD error */
	if (xbe_errs & (XMI_CRD)) 
		xma_check_crd(xcp_node,xmidata);

	if (flush_cache) {
		/* now enable cache by method specified in XCP spec */
		save_led_val = v6200csr->csr1 & 0x1ffff;  /* save led values */
		v6200csr->csr1 = 0x00040000|save_led_val; /* force miss */
		v6200csr->csr1 = 0x00140000|save_led_val; /* invalidate */
		v6200csr->csr1 = 0|save_led_val;	  /* enable cache */
	}
	return(0);
}

/*
 *	Routine xcp_log_soft
 *
 *	Discription:  This routine logs a soft error for processor pointed
 *	to by the pointer xcp_node.  A maximum of 1 packet are logged every
 * 	15 minutes.
 *
 */

xcp_log_soft(xcp_node,sptr) 
	struct xcp_reg *xcp_node;
	register struct el_xcpsoft *sptr;

{
	struct el_rec *elrp;
	register struct el_xcp54 *ptr;

	elrp = ealloc(sizeof(struct el_xma),EL_PRILOW);
	if (elrp) {
		LSUBID(elrp,ELCT_6200_INT54,EL_UNDEF,EL_UNDEF,EL_UNDEF,EL_UNDEF,EL_UNDEF);
		ptr = &elrp->el_body.el_xcp54;
		ptr->xcp_csr1 = v6200csr->csr1;
		ptr->xcp_cadr = mfpr(CADR);
		ptr->xcp_mser = mfpr(MSER);
		ptr->xcp_dtype = xcp_node->xcp_dtype;	
		ptr->xcp_xbe = xcp_node->xcp_xbe;	
		ptr->xcp_fadr = xcp_node->xcp_fadr;	
		ptr->xcp_gpr = xcp_node->xcp_csr2;	
		ptr->xcp_csr2 = xcp_node->xcp_csr2;
		ptr->xcp_soft = *sptr;
	
    		EVALID(elrp);	
	}
	else mprintf("log failed\n");


	CURRENT_CPUDATA->cpu_state|= CPU_SOFT_DISABLE;

}


/*
 *	Routine:	xma_check_crd
 *
 *	Discription:	This routine scans all of the Calypso arrays
 *	looking for the board that signaled the CRD error.  When found
 *	the error is logged and CRD's are disabled from that board.
 */
xma_check_crd(xcp,xmidata) 
	struct xcp_reg *xcp;
	struct xmidata *xmidata;
{
	int xminode;
	struct xma_reg *xma;

	xma = (struct xma_reg *) xmidata->xmivirt;
	for(xminode = 0; xminode < MAX_XMI_NODE; xminode++,xma++) {

		/* if node present then check to see if it is an 
		   XMA */
		if ((xmidata->xminodes_alive & (1<<xminode)) &&
		   ((short) (xma->xma_type) == XMI_XMA)) {

	   		/* check for CRD error and check that CRD 
			   logging for board has not been disabled */
			if ((xma->xma_mecer&(XMA_ECC_CRD_ERROR)) && 
			   ((xma->xma_mctl1&XMA_CTL1_CRD_DISABLE)==0)){
				/* log error */
				log_mem_error(xcp,xma,EL_PRILOW);

				/* clear CRD request */
				xma->xma_mecer = XMA_ECC_CRD_ERROR;
		
				/* disable further CRD's */
				xma->xma_mctl1|=(XMA_CTL1_CRD_DISABLE);
			}

		}
	}	
}



/*
 * Memenable enables the memory controller corrected data reporting.
 * This runs at regular intervals, turning on the interrupt.
 * The interrupt is turned off, per memory controller, when error
 * reporting occurs.  Thus we report at most once per memintvl.
 */

ka6200memenable ()
{
	int xminode;
	struct xmi_reg	*nxv;	/* virtual pointer to XMI node */
	struct xmidata *xmidata;

	xmidata = get_xmi(0);

	nxv = xmidata->xmivirt;
	for(xminode = 0; xminode < MAX_XMI_NODE; xminode++,nxv++) {
		if ((xmidata->xminodes_alive & (1<<xminode)) &&
		    ((short) (nxv->xmi_dtype) == XMI_XMA)) {
	
			/* clear disable CRD flag in memory controller */
			((struct xma_reg *)nxv)->xma_mctl1 
					      &= ~(XMA_CTL1_CRD_DISABLE);
		}
	}
	return(0);

}

/*
 * this routine sets the cache to the state passed.  enabled/disabled
 */

ka6200setcache(state)
int state;

{
  	if (state) {
		/* enable the cache */
		ka6200_enable_cache();
		return(0);
	}
}

ka6200cachenbl()
{
	cache_state = 0x1;
	return(0);
}


ka6200tocons(c)
	register int c;
{
	register int timeo;

	timeo = 100000;
	while ((mfpr (TXCS) & TXCS_RDY) == 0) {
		if (timeo-- <= 0) {
			return(0);
		}
	}
	mtpr (TXDB, c);
	return(0);
}

int ka6200badaddr(addr,len)
caddr_t addr;
int len;
{
	register int foo,s,i;	
	register struct bi_nodespace *biptr;
	
#ifdef lint
	len=len;
#endif lint

	s=spl7();

	for (i=0; i < nNVAXBI ; i++) {
		if (biptr = bidata[i].cpu_biic_addr) {
			biptr->biic.biic_err = biptr->biic.biic_err;
			foo = biptr->biic.biic_gpr0;
		}
	}

	foo = bbadaddr(addr,len);


	for (i=0; i < nNVAXBI ; i++) {
		if (biptr = bidata[i].cpu_biic_addr) {
			if ((biptr->biic.biic_err 
				& ~BIERR_UPEN)!=0) foo=1;
			biptr->biic.biic_err = biptr->biic.biic_err;
		}
	}

	splx(s);
	return(foo);
}


/* reboot VAX62xx machine */
ka6200reboot() {

	struct xmidata *xmidata;
	struct xcp_reg *xcp_node;


	/* set O/S reboot flag so reboot happens */
	ccabase.cca_hflag |=CCA_V_REBOOT;
	
	/* get pointer to xcp node to reboot*/
	xmidata = get_xmi(0);
	xcp_node =(struct xcp_reg *)xmidata->xmivirt+(v6200csr->csr1 & 0xf);
	
	/* hit the halt bit... so long */
	xcp_node->xcp_xbe = (xcp_node->xcp_xbe & ~XMI_XBAD) | XMI_NHALT;

	DELAY(10000);	/* give time for the "XMI_NHALT" to work */
	
	/* just in case "XMI_NHALT" doesn't work. We should never reach
	   the halt */
	asm("halt");  
}

/* halt VAX62xx machine at console */
ka6200halt() {


	struct xmidata *xmidata;
	struct xcp_reg *xcp_node;

	/* get pointer to xcp node that machine checked */
	xmidata = get_xmi(0);
	xcp_node =(struct xcp_reg *)xmidata->xmivirt+(v6200csr->csr1 & 0xf);

	/* hit the halt bit... so long */
	xcp_node->xcp_xbe = (xcp_node->xcp_xbe & ~XMI_XBAD) | XMI_NHALT;

	DELAY(10000);	/* give time for the "XMI_NHALT" to work */
	
	/* just in case "XMI_NHALT" doesn't work. We should never reach
	   the halt */
	asm("halt");  
}

xcpinit(nxv,nxp,xminumber,xminode)
char *nxv;
char *nxp;
int xminumber,xminode;
{
#ifdef lint
	nxv=nxv; nxp=nxp;
	xminumber=xminumber;
	xminode = xminode;
#endif lint


}

ka6200nexaddr(xminumber,xminode) 
 	int xminode,xminumber;
{
	return((XMI_START_PHYS+(xminode * 0x80000)));		
}

ka6200umaddr(binumber,binode) 
 	int binumber,binode;
{

	return(((int) bidata[binumber].biphys)  
		+ 0x400000 + (0x40000 * binode));

}

ka6200udevaddr(binumber,binode) 
 	int binumber,binode;
{
	return(((int) bidata[binumber].biphys)  
		+ 0x400000 + 0x3e000 + (0x40000 * binode));
}
