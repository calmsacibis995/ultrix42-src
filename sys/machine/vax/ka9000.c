#ifndef lint
static char *sccsid = "@(#)ka9000.c	4.13	ULTRIX	4/11/91";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1988,1989,1990 by			*
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
 * Modification History:	ka9000.c
 * 
 * 11-Apr-91	dlh
 * ka9000startcpu()
 * 	moved where vpdata area is KM_ALLOC'd.  This will ensure that
 * 	the area has been allocated before the cpu is actually
 * 	started.
 *
 * 20-dec-90
 *	added parameter to vp_reset() call
 *	added call to vp_reset() during ka9000startcpu()
 *
 * 14-Nov-90 -- stuarth
 * 	added is_adapt_configured to ensure xmi is configured
 * 	moved address computations in xja_err to before where addrs are used
 * 
 * 13-Nov-90 -- paradis
 *	Put console back in a useful state when it signals that
 *	it has rebooted.
 *
 * 12-Nov-90 -- stuarth
 * 	SPU msgs with only a header are now processed.
 * 	SPU timestamp msgs are now not logged.
 * 
 * 16-Oct-90 -- stuarth
 * 	SPU log, swap args in memory message.  Add UNDEF for default case.
 * 
 * 15-Oct-90 -- paradis
 *	Copy return status to SPARAM field of RXFCT when done.
 *
 * 10-Oct-90 -- paradis
 *	Added VAX 9000 vector support.
 *
 * 17-Sep-90 -- stuarth (Stuart Hollander)
 * 	Added XJA error handling.
 * 	Added logging in machine check code.
 * 	Removed setting XJA_IPE in errintr because IPE is not in that reg.
 * 	Removed kmalloc of only one xmidata struct.  We kmalloc 4 of them.
 * 
 * 31-Aug-90 -- paradis
 *	Removed TXFCT asynchronous request mechanism; not used.
 *	Added error-logging support (stuarth)
 *	Added machine-check handler
 *	Removed unnecessary XMI reset from ka9000_reboot()
 *	Simplified DMD allocation scheme
 *	General cleanups
 *
 * 22-Nov-89 -- paradis (Jim Paradis)
 *	Merged in SPU support
 *
 * 13-Nov-89 -- rafiey (Ali Rafieymehr)
 *	Merged in changes required for booting VAX9000
 *
 * 27-Sep-89 -- rafiey (Ali Rafieymehr)
 *	Created this file for VAX9000 (Aquarius)
 *
 **********************************************************************/

#include "../../machine/common/cpuconf.h"
#include "../h/types.h"
#include "../h/time.h"
#include "../h/param.h"
#include "../vax/cons.h"
#include "../h/smp_lock.h"
#include "../h/errlog.h"
#include "../vax/mtpr.h"
#include "../vax/cpu.h"
#include "../machine/scb.h"
#include "../vax/mem.h"
#include "../vax/pte.h"
#include "../vax/nexus.h"
#include "../io/uba/ubareg.h"
#include "../vax/ioa.h"
#include "../h/user.h"
#include "../h/cmap.h"
#include "../h/vmmac.h"
#include "../h/kmalloc.h"
#include "../vax/psl.h"
#include "../h/cpudata.h"
#include "../vax/ka9000.h"
#include "../machine/cvax.h"
#include "../h/proc.h"
#include "../h/ipc.h"
#include "../h/shm.h"
#include "../h/conf.h"
#include "../h/errno.h"
#include "../h/systm.h"
#include "../h/map.h"
#include "../h/vm.h"
#include "../h/tty.h"
#include "../machine/vectors.h"

#include "../io/xmi/xmireg.h"
#include "../io/xmi/xmareg.h"

extern long sbi_there;	/* defined in autoconf.c */
extern int nexusinfo();	/* defined in errlog.c */

extern int catcher[256];

int	ka9000mem_err_report; 	/* state of memory error reporting */
int	ka9000memerrs = 0;	/* number of times memerr has been entered */

extern struct bidata bidata[];
extern int cache_state;
extern int nNVAXBI;
extern char *ka9000_ip_addr;
struct v9000csr v9000csr[1];
extern struct xja_regs xja_mem[];
extern struct xmi_reg xmi_start[];
int ali_debug_flag = 0;


/* Structures to hold soft-error statistics */
struct mc9000_softerr_stats	cpu_softerrs[NCPU_9K];
struct mc9000_softerr_stats	vbox_softerrs[NCPU_9K];
struct mc9000_softerr_stats	mem_softerrs;


/* External data */
extern	int	boot_cpu_mask;
extern	int	boot_cpu_num;
extern	struct sminfo sminfo;

/* Static data local to this module */

/* Datagram allocator */
struct dmd *	spu_dmdlist = (struct dmd *)NULL;

/* Keepalive system */
int	ka9000_keepalive_enabled = 0;
int	ka9000_keepalive_misses = 0;

/* Spurious interrupt counters */
int	ka9000_addcpu_reqs = 0;
int	ka9000_rmcpu_reqs = 0;
int	ka9000_opcom_reqs = 0;

/* Error logger */
int	ka9000_errlog_enabled = 0;

/*ARGSUSED*/
xja0_errvec(pc, ps) caddr_t pc; int ps; {ka9000xja_err(0); }
/*ARGSUSED*/
xja1_errvec(pc, ps) caddr_t pc; int ps; {ka9000xja_err(1); }
/*ARGSUSED*/
xja2_errvec(pc, ps) caddr_t pc; int ps; {ka9000xja_err(2); }
/*ARGSUSED*/
xja3_errvec(pc, ps) caddr_t pc; int ps; {ka9000xja_err(3); }

int (*xja_err_vectors[4])() = { xja0_errvec, xja1_errvec,
				xja2_errvec, xja3_errvec };

extern	int	max_vec_procs;

ka9000conf(cpup)
	struct cpusw *cpup;
{
	struct xmi_reg  *nxv;   /* virtual pointer to XMI node */
	struct xmi_reg  *nxp;   /* physical pointer to XMI node */

	register int timeout;
	register struct xmidata *xmidata;
	struct xja_regs *xja;
	struct pte *pte;
	int cpu_subtype;
	int cpucnf;
	int i;
	int xja_node;
	union cpusid cpusid;
	extern char Sysbase[];
	extern int nNXMI;

	cpusid.cpusid = mfpr(SID);

/*
 * Map XJA private space
 * The relevant part of XJA private space is a structure
 * that fits in under one page.  So we use one virtual
 * page to map only one physical page of each xja priv spc
 */

        for(i = 0; i<nNXMI; i++){
	    if(mfpr(CPUCNF) & (1 << (CPUCNF_XJA_PRESENT+i)))
	    	nxaccess(XJA_START_PHYS+XJA_SIZE*i, ((struct pte *)XJAmap)+i, NBPG);
	}
/* allocate xmi structures */
	KM_ALLOC(xmidata,struct xmidata *,nNXMI*sizeof(struct xmidata ),
					KM_DEVBUF,KM_NOW_CL_CO_CA);
/*
 * We allocate all xmi structs, even if not configured.
 * But we invalidate (xminum = -1) the non-configured ones.
 */

	head_xmidata = xmidata;
	xmidata->next = 0;
	xmidata->xminum = -1;
	for(i=0; i<nNXMI; i++, xmidata++) {
		if(i == nNXMI-1)
			xmidata->next = 0;
		else
			xmidata->next = xmidata+1;
		xmidata->xminum = -1;
		if(!(mfpr(CPUCNF) & (1<< (CPUCNF_XJA_PRESENT+i))))
			continue;
			/* was xmi configured? */
		if(is_adapt_configured("xmi", i) == 0) {
			printf("xmi %x not configured\n", i);
			continue;
		}
		xmidata->xminum = i;
		xja =(struct xja_regs *) (((char *)xja_mem) + i*NBPG);

			/* clear error flags */
		xja->xja_errs = xja->xja_errs;
#define INTSTK 1
			/* set entry in SCB */
		*((caddr_t *) ((caddr_t)&scb+0x50+(0x200*i)) ) =
			(caddr_t)xja_err_vectors[i] + INTSTK;

			/* Enable all XJA interrupts */
		xja->xja_errintr = XJA_XMI_ARB | XJA_JXDI | XJA_TRANS_TOUT
	    		| XJA_CMD_NOACK | XJA_RD_RES | XJA_RD_SEQ_ERR
			| XJA_REATTEMP_TOUT | XJA_CRD | XJA_WR_NOACK
			| XJA_RD_NOACK | XJA_WR_SEQ_ERR
	    		| XJA_PAR_ERR | XJA_CC ;

		xja_node = (xja->xja_cnf >> 12) & 0xf;
		xmidata->xmiintr_dst = 1<< xja_node;
		xmidata->xmiphys = (struct xmi_reg *) (XMI_START_PHYS + 0x800000 * i);
		xmidata->xmivec_page = &scb.scb_stray + i*128;

			/* Map xja node space */
		nxv = xmi_start;
		nxv += (i*16) + xja_node;
		nxp = (struct xmi_reg *) ka9000nexaddr(i,xja_node);
		nxaccess(nxp,&Sysmap[btop((int)(nxv) & ~VA_SYS)],XMINODE_SIZE);
	}

	/* Initialize machine check soft error statistics */
	for(i = 0; i < NCPU_9K; i++) {
		cpu_softerrs[i].err_cnt = 0;
		cpu_softerrs[i].lasterr_time = 0;
		vbox_softerrs[i].err_cnt = 0;
		vbox_softerrs[i].lasterr_time = 0;
	}
	mem_softerrs.err_cnt = 0;
	mem_softerrs.lasterr_time = 0;

	cpucnf = mfpr(CPUCNF);
	cpu_avail = 0;
	for (i = 0; i < 4; i++) {
	    if (cpucnf & (1 << i))
		cpu_avail++;
	}

	/* Configure vector processors (if any) */
	vpmask = vpfree = vptotal = 0;
	if(max_vec_procs > 0) {
		/*
		 * Set up vector processing system wide variables.  Record
		 * which scalar cpu's have an attached vector processor.
		 */
		vpmask = (cpucnf >> 8) & 0xf;
		vpfree = 1 ;
		for(i = 0; i < 4; i++) {	/* Should be constant */
			if(vpfree & vpmask) {
			    printf("VBOX processor attached to cpu %d\n", i);
			    vptotal++;
			}
			vpfree <<=1;
		}
		vpfree = vpmask;
		num_vec_procs = 0;

		KM_ALLOC (( CURRENT_CPUDATA->cpu_vpdata), struct vpdata *,
		  sizeof(struct vpdata), KM_VECTOR, KM_CLEAR | KM_CONTIG );

		if (vpmask & CURRENT_CPUDATA->cpu_mask) {
			vp_reset (CURRENT_CPUDATA->cpu_num);
		}
	}

	/* Make sure the vector processor is disabled.  Take advantage
	 * of the fact that (according to section 13.2.3 of the VAX
	 * architecture reference manual) neither a mtpr(VPSR) nor a
	 * mfpr(VPSR) will cause a reserved operand fault on a
	 * vector-absent VAX.
	 */
	mtpr(VPSR, 0);

	if (cpusid.cpu9000.cp_type_id)
		printf("VAX92%d0%s, ", cpu_avail, (vptotal > 0) ? "-VP" : "");
	else
		printf("VAX94%d0%s, ", cpu_avail, (vptotal > 0) ? "-VP" : "");
	printf("serial no. %d, plant code = %d, hardware rev level = %d\n",
		cpusid.cpu9000.cp_sno, cpusid.cpu9000.cp_plant,
		cpusid.cpu9000.cp_rev);
/*
 * Set the Interrupt mode to absolute mode (not Round Robin) 
 */

	timeout = 20000000;
	while((mfpr(TXFCT) & TXFCT_RDY) == 0) {
		if(--timeout == 0) {
			panic("ka9000conf: TXFCT not ready");
		}
	}
	mtpr(TXPRM, boot_cpu_mask);		/* Select boot cpu only */
/*
 * Set the interrupt mode to absolute (not round robin) 
 */
	mtpr (TXFCT, TXFCT_SET_INTMODE);

	timeout = 20000000;
	while((mfpr(TXFCT) & TXFCT_RDY) == 0) {
		if(--timeout == 0) {
			panic("ka9000conf: TXFCT not ready");
		}
	}

	printf("\tCPU Configuration = 0x%x\n", mfpr(CPUCNF));
	
	/* Set up the SPU */
	ka9000_init_spu();

	spl0();  			/* Ready for interrupts */

	for(i=0; i<nNXMI; i++){
		if(mfpr(CPUCNF) & (1 << (CPUCNF_XJA_PRESENT+i)))
			xmiconf(i);
	}
	
	return(0);
}


ka9000_enable_cache() {

}


extern struct xmidata *get_xmi();
ka9000mapcsr() {
	struct xja_regs *xja;
	register int i;
	register int xja_node;

/*
 * Map XJA private space
 */
        for(i = 0; i<nNXMI; i++){
	    if(mfpr(CPUCNF) & (1 << (CPUCNF_XJA_PRESENT+i))) {
	    	nxaccess(XJA_START_PHYS+XJA_SIZE*i, ((struct pte *)XJAmap)+i, NBPG);
		xja =(struct xja_regs *) (((char *)xja_mem) + i*NBPG);
		xja->xja_errs = xja->xja_errs;
		xja_node = (xja->xja_cnf >> 12) & 0xf;
			/* XJA fatal errors always vector to SCB 50H.
			  Other XJA errors use xja_errscb to determine 
			  SCB vector.  Upon XJA init, xja_errscb is set to 64H.
			  We change it to 50H so all xja errs go to same vec.
			 */
		xja->xja_errscb = 0x50 + (0x200 * i);

	    }
	}
}

/*
 *	ka9000memerr  ---  Hard errors are reported through SCB vector
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


ka9000_xma_check_errors(xcp,priority)
struct xcp_reg *xcp;
int priority;
{
}

ka9000_clear_err()
{
	struct xmi_reg  *nxv;   /* virtual pointer to XMI node */
	struct xja_regs *xja;
	register int i;
	register int xja_node;
	int s;

	s = spl7();
        for(i = 0; i<nNXMI; i++){
	    if(mfpr(CPUCNF) & (1 << (CPUCNF_XJA_PRESENT+i))) {
		nxv = xmi_start;
		nxv += (i * 16);
		xja =(struct xja_regs *) (((char *)xja_mem) + i*NBPG);
		xja_node = (xja->xja_cnf >> 12) & 0xf;
		nxv += xja_node;
		nxv->xmi_xbe = nxv->xmi_xbe & ~XMI_XBAD;
	    }
	}
	splx(s);

}


ka9000_clear_xbe()
{
	struct xmi_reg  *nxv;   /* virtual pointer to XMI node */
	struct xja_regs *xja;
	register int i;
	register int xja_node;

        for(i = 0; i<nNXMI; i++){
	    if(mfpr(CPUCNF) & (1 << (CPUCNF_XJA_PRESENT+i))) {
		nxv = xmi_start;
		nxv += (i * 16);
		xja =(struct xja_regs *) (((char *)xja_mem) + i*NBPG);
		xja_node = (xja->xja_cnf >> 12) & 0xf;
		nxv += xja_node;
		nxv->xmi_xbe = nxv->xmi_xbe & ~XMI_XBAD;
	    }
	}
}

/* log XMA memory error.  If priority is not LOW then print to 
   console.
*/
ka9000_log_mem_error(xcp,xma,priority)
	int priority;
	struct xma_reg *xma;
	struct xcp_reg *xcp;
{
	struct el_xma *xma_pkg;
	struct xmidata *xmidata;
	struct el_rec *elrp;
}

/*
 *  ka9000crderr() --- "Soft" errors are reported through SCB vector
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

ka9000crderr()
{
	struct xcp_reg *xcp_node;
	int save_led_val;
	register struct xmidata *xmidata ;
	int node;
	struct xcp_machdep_data *xcpdata;

	register struct el_xcpsoft *sptr;
	register int csr2_errs,xbe_errs, flush_cache=0;
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

ka9000_xcp_log_soft(xcp_node,sptr) 
	struct xcp_reg *xcp_node;
	register struct el_xcpsoft *sptr;

{
	struct el_rec *elrp;
	register struct el_xcp54 *ptr;

}


/*
 *	Routine:	xma_check_crd
 *
 *	Discription:	This routine scans all of the Calypso arrays
 *	looking for the board that signaled the CRD error.  When found
 *	the error is logged and CRD's are disabled from that board.
 */
ka9000_xma_check_crd(xcp,xmidata) 
	struct xcp_reg *xcp;
	struct xmidata *xmidata;
{
	int xminode;
	struct xma_reg *xma;
}





int ka9000badaddr(addr,len)
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


ka9000reboot() {
	int	timeout;
	struct xmi_reg  *nxv;   /* virtual pointer to XMI node */
	struct xja_regs *xja;
	register int i;
	register int xja_node;
/*
 * Clear warm restart
 */
	timeout = 20000000;
	while((mfpr(TXFCT) & TXFCT_RDY) == 0) {
		if(--timeout == 0) {
			panic("ka9000reboot: TXFCT not ready");
		}
	}
	mtpr (TXFCT, TXFCT_CLEAR_WS | (boot_cpu_num << TXFCT_SPARAM_SHIFT));
	timeout = 20000000;
	while((mfpr(TXFCT) & TXFCT_RDY) == 0) {
		if(--timeout == 0) {
			panic("ka9000reboot: TXFCT not ready");
		}
	}
/*
 * Clear cold restart
 */
	timeout = 20000000;
	while((mfpr(TXFCT) & TXFCT_RDY) == 0) {
		if(--timeout == 0) {
			panic("ka9000reboot: TXFCT not ready");
		}
	}
	mtpr (TXFCT, TXFCT_CLEAR_CS | (boot_cpu_num << TXFCT_SPARAM_SHIFT));
	timeout = 20000000;
	while((mfpr(TXFCT) & TXFCT_RDY) == 0) {
		if(--timeout == 0) {
			panic("ka9000reboot: TXFCT not ready");
		}
	}
	timeout = 20000000;
	while((mfpr(TXFCT) & TXFCT_RDY) == 0) {
		if(--timeout == 0) {
			panic("ka9000reboot: TXFCT not ready");
		}
	}
	mtpr (TXFCT, TXFCT_REBOOT_SYSTEM);
	timeout = 20000000;
	while((mfpr(TXFCT) & TXFCT_RDY) == 0) {
		if(--timeout == 0) {
			panic("ka9000reboot: TXFCT not ready");
		}
	}
	
	/* Execute instructions until we're halted by SPU */
	for(;;);
}

ka9000halt() {

	int	i;
	long	cpucnf;

	cpucnf = mfpr(CPUCNF);

	/* Explicitly halt each non-boot CPU */
	for(i = 0; i < 4; i++) {
		if((i != boot_cpu_num) && (cpucnf & (1 << i))) {
			ka9000_spu_request(TXFCT_HALT_AND_KEEP, i, 0, 1);
		}
	}

	/* Finally, halt ourselves... */
	asm("halt");
}

xjainit(nxv,nxp,xminumber,xminode,xmidata)
char *nxv;
char *nxp;
int xminumber,xminode;
register struct xmidata *xmidata;
{
#ifdef lint
	nxv=nxv; nxp=nxp;
	xminumber=xminumber;
	xminode = xminode;
#endif lint


}



ka9000stopcpu(cpu_num)
int cpu_num;
{
	int	timeout;

/*
 * Test for valid cpu.  VAX9000 supports up to 4 cpu's
 */
	if(cpu_num < 0 || cpu_num > 3)
		return(0);

/*
 * Test to see if requested cpu is configured
 */

	if((mfpr(CPUCNF) & (1 << cpu_num)) == 0)
		return(0);

	timeout = 20000000;
	while((mfpr(TXFCT) & TXFCT_RDY) == 0) {
		if(--timeout == 0) {
			panic("ka9000stopcpu: TXFCT not ready");
		}
	}
/*
 * Halt the cpu but keep it in the list of available cpus
 */
	mtpr (TXFCT, TXFCT_HALT_AND_KEEP | (cpu_num << TXFCT_SPARAM_SHIFT));
	return(1);


}

ka9000startcpu(cpu_num)
int cpu_num;
{
	int	timeout;

/*
 * Test for valid cpu.  VAX9000 supports up to 4 cpu's
 */
	if(cpu_num < 0 || cpu_num > 3)
		return(0);

/*
 * Test to see if requested cpu is configured
 */

	if((mfpr(CPUCNF) & (1 << cpu_num)) == 0)
		return(0);

/* Get a cpudata structure */

	get_cpudata(cpu_num);
	timeout = 20000000;
	while((mfpr(TXFCT) & TXFCT_RDY) == 0) {
		if(--timeout == 0) {
			panic("ka9000startcpu: TXFCT not ready");
		}
	}

	if (max_vec_procs > 0) {

		/* allocate memory for the vpdata structure */
		KM_ALLOC (( CPUDATA ( cpu_num)-> cpu_vpdata), 
			struct vpdata *, sizeof(struct vpdata), 
			KM_VECTOR, KM_CLEAR | KM_CONTIG);
	}

	mtpr (TXPRM, CPU_START_ADDR);	/* set the starting address for the secondary cpu */
	mtpr (TXFCT, TXFCT_BOOT_CPU | (cpu_num << TXFCT_SPARAM_SHIFT)); /* start it */

	timeout = 20000000;
	while((mfpr(TXFCT) & TXFCT_RDY) == 0) {
		if(--timeout == 0) {
			panic("ka9000startcpu: TXFCT not ready");
		}
	}

	/* Wait for this processor to declare itself runnable */
	timeout=1500;
	while(timeout-->0) {
		DELAY(10000); /* 1/100 of sec */
		/* if running then success */
		if (CPUDATA(cpu_num)->cpu_state&CPU_RUN) {
			DELAY(1000000);
			return(1);
		}
	}

/* If we get here, then the addition of this cpu failed... */
	return(0);
/*	asm("halt");*/
}




ka9000machcheck (cmcf)
	caddr_t cmcf;
{
	register struct el_rec *elp;
	register struct	el_mc9000frame	*mcp;
	int	 cpu_num;

	mcp = (struct el_mc9000frame *)cmcf;

	/* If this is in fact an EBOX machine check, handle
	 * it differently...
	 */
	if(mcp->mc_type == MCHK_9000_TYPE_EBOX) {
		ka9000eboxmcheck(cmcf);
		return(0);
	}

	/* This is an SPU-reported error.  Find out exactly
	 * which error occurred and deal with it appropriately.
	 */
	cpu_num = CURRENT_CPUDATA->cpu_num;

	if(mcp->mc_err_summ & MCHK_9K_ERR_REG_PE) {
		/* Register parity error.  See if we can kill
		 * the process that was running
		 */
		if(USERMODE(mcp->mc_psl) && (u.u_procp->p_pid != 1)
			&& (cpu_num != boot_cpu_num)) {

			/* Yes we can.  Do so & treat this as a soft
			 * CPU error.
			 */
			swkill(u.u_procp, 
				"Machine check: Register parity error");
			if(!ka9000softerr(&cpu_softerrs[cpu_num])) {
				/* Too many soft errors.  Disable the CPU */
				ka9000discpu(mcp, cpu_num);
			}
		}
		else {
			/* Can't kill the process.  Have to panic */
			ka9000badmchk(mcp, "Register parity error");
		}
	}

	else if(mcp->mc_err_summ & MCHK_9K_ERR_MB_ADDR_FLT) {
		/* This one is unrecoverable */
		ka9000badmchk(mcp, "MBOX address fault");
	}

	else if(mcp->mc_err_summ & MCHK_9K_ERR_MB_DATA_FLT) {
		int	addr_type = CSYS;

		/* MBOX data fault.  If the address is a kernel
		 * address or we can't tell what kind it is, 
		 * then crash the system.
		 */
		addr_type = ka9000mckaddrtype(mcp);

		if((addr_type != CTEXT) && (addr_type != CDATA)
		   && (addr_type != CSTACK) && (addr_type != CSMEM)) {

			ka9000badmchk(mcp, "MBOX data fault");
		}
		else {
			/* If we can't gracefully kill the proc
			 * that's currently running, then crash
			 * the system.
			 */
			if(!ka9000addrkill(mcp, addr_type, "MBOX data fault")) {
				ka9000badmchk(mcp, "MBOX data fault");
			}
			else if(!ka9000softerr(&cpu_softerrs[cpu_num])) {
				/* Too many soft errors.  Disable the CPU */
				ka9000discpu(mcp, cpu_num);
			}
		}
	}
	else if(mcp->mc_err_summ & MCHK_9K_ERR_MB_WRBK_FLT) {
		/* This one is unrecoverable */
		ka9000badmchk(mcp, "MBOX writeback fault");
	}
	else if(mcp->mc_err_summ & MCHK_9K_ERR_IB_PEND_GPR) {
		/* IBOX pending GPR write failed.
		 * See if we can kill the process that was running
		 */
		if(USERMODE(mcp->mc_psl) && (u.u_procp->p_pid != 1)
			&& (cpu_num != boot_cpu_num)) {

			/* Yes we can.  Do so & treat this as a soft
			 * CPU error.
			 */
			swkill(u.u_procp, 
				"Machine check: IBOX pending GPR wrt failed");
			if(!ka9000softerr(&cpu_softerrs[cpu_num])) {
				/* Too many soft errors.  Disable the CPU */
				ka9000discpu(mcp, cpu_num);
			}
		}
		else {
			/* Can't kill the process.  Have to panic */
			ka9000badmchk(mcp, "IBOX pending GPR wrt failed");
		}
	}
	else if(mcp->mc_err_summ & MCHK_9K_ERR_IB_LOST_PC) {
		/* IBOX lost PC.
		 * See if we can kill the process that was running
		 */
		if(USERMODE(mcp->mc_psl) && (u.u_procp->p_pid != 1)
			&& (cpu_num != boot_cpu_num)) {

			/* Yes we can.  Do so & treat this as a soft
			 * CPU error.
			 */
			swkill(u.u_procp, 
				"Machine check: IBOX lost PC");
			if(!ka9000softerr(&cpu_softerrs[cpu_num])) {
				/* Too many soft errors.  Disable the CPU */
				ka9000discpu(mcp, cpu_num);
			}
		}
		else {
			/* Can't kill the process.  Have to panic */
			ka9000badmchk(mcp, "IBOX lost PC");
		}
	}
	else if(mcp->mc_err_summ & MCHK_9K_ERR_RDS) {
		int	addr_type = CSYS;

		/* Double-bit memory error.  If the address is a kernel
		 * address or we can't tell what kind it is, 
		 * then crash the system.
		 */
		addr_type = ka9000mckaddrtype(mcp);

		if((addr_type != CTEXT) && (addr_type != CDATA)
		   && (addr_type != CSTACK) && (addr_type != CSMEM)) {

			ka9000badmchk(mcp, "Dbl-bit memory error");
		}
		else {
			/* If we can't gracefully kill the proc
			 * that's currently running, then crash
			 * the system.
			 */
			if(!ka9000addrkill(mcp, addr_type, "Dbl-bit mem err")) {
				ka9000badmchk(mcp, "Dbl-bit mem err");
			}
			else if(!ka9000softerr(&mem_softerrs)) {
				/* Too many soft errors.  Crash the system*/
				ka9000badmchk(mcp, "Too many mem errs");
			}
		}
	}
	else if(mcp->mc_err_summ & MCHK_9K_ERR_BAD_DATA) {
		int	addr_type = CSYS;

		/* Bad data.  If the address is a kernel
		 * address or we can't tell what kind it is, 
		 * then crash the system.
		 */
		addr_type = ka9000mckaddrtype(mcp);

		if((addr_type != CTEXT) && (addr_type != CDATA)
		   && (addr_type != CSTACK) && (addr_type != CSMEM)) {

			ka9000badmchk(mcp, "Bad data");
		}
		else {
			/* If we can't gracefully kill the proc
			 * that's currently running, then crash
			 * the system.
			 */
			if(!ka9000addrkill(mcp, addr_type, "Mchk: Bad data")) {
				ka9000badmchk(mcp, "Bad data");
			}
			else if(!ka9000softerr(&mem_softerrs)) {
				/* Too many soft errors.  Crash the system*/
				ka9000badmchk(mcp, "Bad data");
			}
		}
	}
	else if(mcp->mc_err_summ & MCHK_9K_ERR_VBOX_ABORT) {
		/* VBOX abort.
		 * See if we can kill the process that was running
		 */
		if(USERMODE(mcp->mc_psl) && (u.u_procp->p_pid != 1)
			&& (cpu_num != boot_cpu_num)) {

			/* Yes we can.  Do so & treat this as a soft
			 * CPU error.
			 */
			swkill(u.u_procp, 
				"Machine check: VBOX abort");
			if(!ka9000softerr(&vbox_softerrs[cpu_num]) ||
			   (mcp->mc_err_summ & MCHK_9K_ERR_SOLID)) {
				/* Hard error, or too many soft errors.  
				 * Disable the VBOX 
				 */
				vp_remove();
			}
		}
		else {
			/* Can't kill the process.  Have to panic */
			ka9000badmchk(mcp, "VBOX abort");
		}
	}
	else if(mcp->mc_err_summ & MCHK_9K_ERR_VBOX_REG_PE) {
		/* VBOX register parity error.
		 * See if we can kill the process that was running
		 */
		if(USERMODE(mcp->mc_psl) && (u.u_procp->p_pid != 1)
			&& (cpu_num != boot_cpu_num)) {

			/* Yes we can.  Do so & treat this as a soft
			 * CPU error.
			 */
			swkill(u.u_procp, 
				"Machine check: VBOX register parity error");
			if(!ka9000softerr(&vbox_softerrs[cpu_num]) ||
			   (mcp->mc_err_summ & MCHK_9K_ERR_SOLID)) {
				/* Hard error, or too many soft errors.  
				 * Disable the VBOX 
				 */
				vp_remove();
			}
		}
		else {
			/* Can't kill the process.  Have to panic */
			ka9000badmchk(mcp, "VBOX register parity error");
		}
	}
	else {
		/* Unknown machine check type */
		ka9000badmchk(mcp, "Unknown machine check type");
	}

		/* The machine check did not cause a panic. */
		/* So, log at priority EL_PRIHIGH, not EL_PRISEVERE */
	if( (elp=ealloc(sizeof(struct el_mck), EL_PRIHIGH)) != EL_FULL ) {
	    LSUBID(elp, ELCT_MCK, ELMCKT_9000,
		EL_UNDEF, EL_UNDEF, EL_UNDEF, EL_UNDEF);
#define ELP_ELMCK elp->el_body.elmck.elmck_frame.el9000mcf
	    ELP_ELMCK.mc_length = mcp->mc_length;
	    ELP_ELMCK.mc_type = mcp->mc_type;
	    ELP_ELMCK.mc_id = mcp->mc_id;
	    ELP_ELMCK.mc_err_summ = mcp->mc_err_summ;
	    ELP_ELMCK.mc_sys_summ = mcp->mc_sys_summ;
	    ELP_ELMCK.mc_vaddr = mcp->mc_vaddr;
	    ELP_ELMCK.mc_paddr = mcp->mc_paddr;
	    ELP_ELMCK.mc_misc_info = mcp->mc_misc_info;
	    ELP_ELMCK.mc_pc = mcp->mc_pc;
	    ELP_ELMCK.mc_psl = mcp->mc_psl;
            if(((mfpr(CPUCNF) >> 8) & 0xf) == (1 << cpu_num)) {
                    /* VBOX present */
                    ELP_ELMCK.mc_vpsr = mfpr(VPSR);
            }
            else {
                    /* VBOX absent */
                    ELP_ELMCK.mc_vpsr = 0;
            }
	    EVALID(elp);
	}

}

/* Routine to handle an EBOX-generated machine check... */
ka9000eboxmcheck(emcp)
caddr_t emcp;
{
	register struct el_rec *elp;
	register struct el_mc9000eboxframe *eemcp =
			(struct el_mc9000eboxframe *) emcp;

	if( (elp=ealloc(sizeof(struct el_mck), EL_PRISEVERE)) != EL_FULL ) {
	    LSUBID(elp, ELCT_MCK, ELMCKT_9000,
		EL_UNDEF, EL_UNDEF, EL_UNDEF, EL_UNDEF);
#define ELP_ELMCK_E elp->el_body.elmck.elmck_frame.el9000eboxmcf
	    ELP_ELMCK_E.mc_length = eemcp->mc_length;
	    ELP_ELMCK_E.mc_type = eemcp->mc_type;
	    ELP_ELMCK_E.mc_pc = eemcp->mc_pc;
	    ELP_ELMCK_E.mc_psl = eemcp->mc_psl;
	    EVALID(elp);
	}

	/* We should never get this! */
	ka9000badmchk((struct el_mc9000eboxframe *)emcp, "EBOX machine check");
}

/* Disable the specified CPU */
ka9000discpu(mcp, cpu_num)
struct el_mc9000frame *mcp;
int cpu_num;
{
	if(cpu_num != CURRENT_CPUDATA->cpu_num) {
		ka9000badmchk(mcp, "ka9000 mchk not on faulting CPU");
	}

	/* Disable ourselves, unless we already stopped
	 * ourselves for a machine check...
	 */
	if(CURRENT_CPUDATA->cpu_stops & IPIMSK_MCHK) {
		ka9000badmchk(mcp, "Multiple fatal machine checks on CPU");
	}
	else if(cpu_num == boot_cpu_num) {
		/* Can't disable the boot CPU... */
		ka9000badmchk(mcp, "Fatal machine check on boot CPU");
	}
	else {
		printf("Machine check: Disabling cpu %d\n", cpu_num);
		hold_cpu(IPI_MCHK);
	}
}


/* On a MBOX or memory machine check, figure out what the memory
 * is being used for (text, data, stack, shared mem...)
 */
ka9000mckaddrtype(mcp)
register struct el_mc9000frame *mcp;
{
	int	addr_type = CSYS;

	if(mcp->mc_misc_info & MCHK_9K_MISC_PA_VALID) {
		/* Physical address is valid.  Go into cmap to
		 * get the type of the page.
		 */
		addr_type = 
		    cmap[pgtocm(clbase(btop(mcp->mc_paddr)))].c_type;
	}
	else if(mcp->mc_misc_info & MCHK_9K_MISC_VA_VALID) {
		int	v;

		/* Virtual address is valid.  Figure out its type. */
		if(mcp->mc_vaddr & 0x80000000) {
			addr_type = CSYS;
		}
		else {
			v = clbase(btop(mcp->mc_vaddr));
			if(isatsv(u.u_procp, v)) {
				addr_type = CTEXT;
			}
			else if (isadsv(u.u_procp, v)) {
				addr_type = CDATA;
				if(vtodp(u.u_procp, v) >=
					u.u_procp->p_dsize) {
					addr_type = CSMEM;
				}
			}
			else if (isassv(u.u_procp, v)) {
				addr_type = CSTACK;
			}
		}
	}
	return(addr_type);
}

/* On MBOX error, see if we can find all processes that reference
 * this particular address and kill them.  Returns 1 if successful,
 * 0 otherwise.
 */
ka9000addrkill(mcp, addr_type, msg)
register struct el_mc9000frame *mcp;
int addr_type;
char * msg;
{
	int	i;
	struct proc *p;
	struct smem *smp;

	if((addr_type == CTEXT) || (addr_type == CDATA) ||
	   (addr_type == CSTACK) || (addr_type == CSMEM)) {

	    /* We can't kill the proc if it's in system space
	     * or if it's the init proc.
	     */
	    if(!USERMODE(mcp->mc_psl) || (u.u_procp->p_pid == 1)) {
		return(0);
	    }

	    /* If it's a text or shared data page, kill everyone
	     * ELSE who's using it first!
	     */
	    if(addr_type == CTEXT) {
		p = u.u_procp->p_xlink;
		while(p != u.u_procp) {
		    swkill(p, msg);
		    p = p->p_xlink;
		}
	    }
	    if(addr_type == CSMEM) {
		if(!(mcp->mc_misc_info & MCHK_9K_MISC_VA_VALID)) {
		    /* We need the virtual address.  If we don't have
		     * it, then we can't do this...
		     */
		    return(0);
		}

		p = (struct proc *)NULL;

		/* Look for the shared memory segment that contains
		 * this virtual address.
		 */
		for(i = 0; i < sminfo.smseg; i++) {
		    if(u.u_procp->p_sm[i].sm_p == NULL) {
			continue;
		    }
		    if(mcp->mc_vaddr >= u.u_procp->p_sm[i].sm_saddr &&
			mcp->mc_vaddr < u.u_procp->p_sm[i].sm_eaddr)

			p = u.u_procp->p_sm[i].sm_link;
			smp = u.u_procp->p_sm[i].sm_p;
			break;
		    }
		}

		/* If we can't find it, we're sunk */
		if(!p) {
			return(0);
		}

		/* Kill everyone who's sharing this segment */
		while(p != u.u_procp) {
			struct proc * nextp;

			nextp = (struct proc *)NULL;
			for(i = 0; i < sminfo.smseg; i++) {
				if(p->p_sm[i].sm_p == smp) {
					nextp = p->p_sm[i].sm_link;
				}
			}
			if(!nextp) {
				/* This shouldn't happen! */
				return(0);
			}
			swkill(p, msg);
			p = nextp;
		}

		/* Killed anyone else that has to be killed...
		 * now kill ourselves!
		 */
		swkill(u.u_procp, msg);
		return(1);
	}
	else {
		/* Not a user address... */
		return(0);
	}
}
				   


/*
 * ka9000badmchk is called when we have an unrecoverable machine check,
 * and we need to log the machine check stack frame to the console
 */
ka9000badmchk(mcp, reason)
register struct el_mc9000frame *mcp;
char * reason;
{
	register struct el_rec *elp;
	int	 cpu_num;

	cpu_num = CURRENT_CPUDATA->cpu_num;
	printf("Unrecoverable machine check: %s\n", reason);
	cprintf("Machine check stack frame:\n");
	cprintf("  Length: 0x%x\n", mcp->mc_length);
	cprintf("  Type: 0x%x\n", mcp->mc_type);
	if(mcp->mc_type == MCHK_9000_TYPE_EBOX) {
		struct el_mc9000eboxframe * emcp =
			(struct el_mc9000eboxframe *)mcp;
		cprintf("  PC: 0x%x\n", emcp->mc_pc);
		cprintf("  PSL: 0x%x\n", emcp->mc_psl);
	}
	else {
		cprintf("  ID: 0x%x\n", mcp->mc_id);
		cprintf("  ERR SUMM: 0x%x\n", mcp->mc_err_summ);
		cprintf("  SYS SUMM: 0x%x\n", mcp->mc_sys_summ);
		cprintf("  Virt addr: 0x%x\n", mcp->mc_vaddr);
		cprintf("  Phys addr: 0x%x\n", mcp->mc_paddr);
		cprintf("  Misc info: 0x%x\n", mcp->mc_misc_info);
		cprintf("  PC: 0x%x\n", mcp->mc_pc);
		cprintf("  PSL: 0x%x\n", mcp->mc_psl);
			/* fatal non-ebox mach chk must log at EL_PRISEVERE */
		if((elp=ealloc(sizeof(struct el_mck),EL_PRISEVERE))!= EL_FULL){
		    LSUBID(elp, ELCT_MCK, ELMCKT_9000,
			EL_UNDEF, EL_UNDEF, EL_UNDEF, EL_UNDEF);
#define ELP_ELMCK elp->el_body.elmck.elmck_frame.el9000mcf
		    ELP_ELMCK.mc_length = mcp->mc_length;
		    ELP_ELMCK.mc_type = mcp->mc_type;
		    ELP_ELMCK.mc_id = mcp->mc_id;
		    ELP_ELMCK.mc_err_summ = mcp->mc_err_summ;
		    ELP_ELMCK.mc_sys_summ = mcp->mc_sys_summ;
		    ELP_ELMCK.mc_vaddr = mcp->mc_vaddr;
		    ELP_ELMCK.mc_paddr = mcp->mc_paddr;
		    ELP_ELMCK.mc_misc_info = mcp->mc_misc_info;
		    ELP_ELMCK.mc_pc = mcp->mc_pc;
		    ELP_ELMCK.mc_psl = mcp->mc_psl;
	            if(((mfpr(CPUCNF) >> 8) & 0xf) == (1 << cpu_num)) {
			    /* VBOX present */
	                    ELP_ELMCK.mc_vpsr = mfpr(VPSR);
	            }
	            else {
	                    /* VBOX absent */
	                    ELP_ELMCK.mc_vpsr = 0;
	            }
		    EVALID(elp);
		}

	}
	panic("Unrecoverable machine check");
}

/* This routine updates the soft error statistics for a particular
 * module.  If the module should be failed, this routine returns
 * 0; otherwise, it returns 1.
 */
ka9000softerr(es)
struct mc9000_softerr_stats *es;
{
	unsigned long	todr = mfpr(TODR);


	/* If we've never had an error or the error occurred a
	 * long time ago, then treat this as the first error ever.
	 */
	if((es->err_cnt == 0) || 
	   ((es->lasterr_time + MCHK_9K_ERR_TIMEOUT) < todr)) {

		es->err_cnt = 1;
		es->lasterr_time = todr;
		return(1);
	}

	/* Bump error count.  If it exceeds the threshold then
	 * recommend failing the module.
	 */
	es->err_cnt++;
	es->lasterr_time = todr;

	if(es->err_cnt > MCHK_9K_MAX_ERRS) {
		return(0);
	}
	else {
		return(1);
	}
}

		

/*
 *  Function:
 *	ka9000memerr()
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


ka9000memerr()
{
	struct el_rec *elrp;
	struct el_mem *mrp;

/* Todo: what to do on these errors is not defined yet */
	return(0);
}

/*
 * this routine sets the cache to the state passed.  enabled/disabled
 */

ka9000setcache(state)
int state;
{
/* Todo: CSWP only has 1 usefull bit: the Initiate sweep bit
 * need to set whatever cache config reg is appropriate for Aquarius
 */
	mtpr (CSWP, state);
	return(0);
}

/*
 * ka9000memenable enables the memory controller corrected data reporting.
 * This runs at regular intervals, turning on the interrupt.
 * The interrupt is turned off, per memory controller, when error
 * reporting occurs.  Thus we report at most once per memintvl.
 */

ka9000memenable ()
{
	register struct mcr *mcr;

/* Todo: what to do for this is not defined yet */
	return(0);
}

ka9000tocons (c)
	register int c;
{
    /* This routine is ONLY used to send Logical Console
     * commands.  Since the use of the Logical Console
     * is discouraged on Aquarius in favor of TXFCT commands,
     * we'll just map the one into the other.
     */
    c &= TXDB_DATA;
    c |= TXDB_ID;
    switch (c) {
	case TXDB_BOOT:		/* Reboot */
	    printf( "ka9000tocons REBOOT: Should use TXFCT instead\n");
	    ka9000_spu_request(TXFCT_REBOOT_SYSTEM, 0, 0, 0);
	    break;

	case TXDB_CWSI:		/* Clear warm start */
	    printf( "ka9000tocons Clear Warmstart: Should use TXFCT instead\n");
	    ka9000_spu_request(TXFCT_CLEAR_WS, boot_cpu_num, 0, 1);
	    break;

	case TXDB_CCSI:		/* Clear cold start */
	    printf( "ka9000tocons Clear Coldstart: Should use TXFCT instead\n");
	    ka9000_spu_request(TXFCT_CLEAR_CS, boot_cpu_num, 0, 1);
	    break;
	default:
	    printf("ka9000tocons: Unrecognized logical console cmd 0x%x\n", c);
	    break;
    }
    return(0);

}

/*
 * Enable cache and enable floating point accelerator
 */

extern	int	cache_state;

ka9000cachenbl()
{
/* Todo: what to do here is not defined yet */
	return(0);
}

ka9000nexaddr(xminumber,xminode) 
 	int xminode,xminumber;
{
	return((XMI_START_PHYS+(xminumber * 0x800000) + (xminode * 0x80000)));		
}


u_short *
ka9000umaddr(ioadpt,ubanumber) 
 	int ioadpt,ubanumber;
{
}

u_short *
ka9000udevaddr(ioadpt,ubanumber) 
 	int ioadpt,ubanumber;
{
}


ka9000logsbi(sbi_num,sbi_type,pc_psl)
long sbi_num;
long sbi_type;
long *pc_psl;
{
extern int cold;	/* Cold start flag */
if (cold) {		/* booting, may not be able to access XMI yet */
	if (head_xmidata) {	/* We can access XMI */
		ka9000_clear_err();
	}
	return(0);
}

}

/* Initialize the state of the Service Processor Unit
 * at boot time
 */
ka9000_init_spu()
{
	/* Enable RXFCT interrupts... */
	mtpr(RXFCT, mfpr(RXFCT) | RXFCT_IE );
}

/* Re-initialize the state of the SPU if an SPU reboot
 * has been detected
 */
ka9000_reinit_spu()
{
	u_long	timo;
	int	i;
	struct dmd *	dmd;
	struct dmd *	dmd_next;
	extern struct tty cons[2];
	long	txcs;
	long	rxcs;


	/* Back out any datagram allocations */

	dmd = spu_dmdlist;
	while(dmd) {
		dmd_next = (struct dmd *)dmd->dmd_link;
		KM_FREE(dmd->dmd_bufaddr, KM_SPU);
		KM_FREE(dmd, KM_SPU);
		dmd = dmd_next;
	}

	spu_dmdlist = (struct dmd *)NULL;

	/* Re-initialize the SPU's state */
	
	/* Set SPU interrupt delivery mode such that all interrupts
	 * are delivered in absolute mode to the boot cpu only
	 */

	/* Wait for TXFCT to become available */
	timo = 20000000;
	while((mfpr(TXFCT) & TXFCT_RDY) == 0) {
		if(--timo == 0) {
			panic("ka9000_reinit_spu: TXFCT not ready");
		}
	}

	mtpr(TXPRM, boot_cpu_mask);		/* Select boot cpu only */
	mtpr(TXFCT, TXFCT_SET_INTMODE |		/* Func: set interrupt mode */
		    (INTMODE_ABSOLUTE 		/* Select absolute mode */
			<< TXFCT_SPARAM_SHIFT));

	/* Initialize the TTY lines */

	txcs = TXCS_IE | TXCS_NO_TTY;
	rxcs = RXCS_IE;
	if(cons[0].t_state & TS_ISOPEN) {
		txcs |= (TXCS_CTY_ENA | TXCS_WM);
	}
	if(cons[1].t_state & TS_ISOPEN) {
		txcs |= (TXCS_RTY_ENA | TXCS_WM);
		rxcs |= RXCS_RDTR;
	}

	mtpr(RXCS,rxcs);
	mtpr(TXCS,txcs);

	/* Enable RXFCT interrupts... */
	mtpr(RXFCT, mfpr(RXFCT) | RXFCT_IE );

	/* If error logging was enabled before the SPU rebooted, then
	 * we have to re-enable it.
	 */
	if(ka9000_errlog_enabled) {
		ka9000_errlog_enabled = 0;
		ka9000_enable_errlog();
	}
}

/*
 * Routine to periodically send a keepalive message
 */
ka9000_keepalive_timeout()
{
	extern int	hz;
	int		ka9000_keepalive_done();

	/* We may get called ONCE even though keepalives are disabled,
	 * if there was a race condition on dequeueing the timeout
	 * request...
	 */
	if(!ka9000_keepalive_enabled) {
		return;
	}

	if(ka9000_keepalive_misses > SPU_MAX_KEEPALIVE_MISS) {
		/* We poked it several times and no response;
		 * reboot the SPU.
		 */
		mtpr(CRBT, 1);

		/* Disable keepalive system; it's up to the SPU to
		 * re-enable it when it comes up...
		 */
		ka9000_keepalive_enabled = 0;
		ka9000_keepalive_misses = 0;
		return;
	}

	/* Record this as a miss (until it hits) */
	ka9000_keepalive_misses++;

	/* Schedule another timeout in 10 seconds */
	timeout(ka9000_keepalive_done, 0, 10 * hz);
	/* Send a keepalive message down to the SPU */
	ka9000_spu_request(TXFCT_KEEPALIVE, 0, 0, 0);
}

/*
 * Routine called on successful acknowledgement of
 * keepalive message.
 */
ka9000_keepalive_done()
{
	ka9000_keepalive_misses = 0;
}




/*
 * Send a request to the SPU
 */
ka9000_spu_request(func, sparam, txprm, wait)
int     func;
int     sparam;
int     txprm;
int     wait;
{
	unsigned long	txfct;
	unsigned long	timeout;

	/* Construct txfct register */
	txfct = func | ((sparam << TXFCT_SPARAM_SHIFT) & TXFCT_SPARAM);

	timeout = 20000000;
	while((mfpr(TXFCT) & TXFCT_RDY) == 0) {
		if(--timeout == 0) {
			panic("ka9000_spu_request: TXFCT not ready");
		}
	}
	mtpr(TXPRM, txprm);		/* Send the request down! */
	mtpr(TXFCT, txfct);

	/* If the caller wants to wait for the request to complete
	 * then wait for it...
	 */
	if(wait) {
		int status;

		timeout = 20000000;
		while(((txfct = mfpr(TXFCT)) & TXFCT_RDY) == 0) {
			if(--timeout == 0) {
				panic("ka9000_spu_request: TXFCT not ready");
			}
			/* Delay 0.1ms and check again */
			DELAY(100);
		}
		status = TXFCT_GET_STATUS(txfct);
		return(status ? 0 : -1);
	}
}


/*
 * RXFCT interrupt service routine.  Services requests from
 * the SPU.
 */
ka9000_rxfct_isr()
{
	u_long	rxfct;
	u_long	rxprm;
	int	func;
	int	sparam;
	int	status;
	struct	dmd * dmd;
	int	s;


	/* Run at spl */
	s = spl5();

	rxprm = mfpr(RXPRM);
	rxfct = mfpr(RXFCT);

	/* Extract fields of interest */
	sparam = RXFCT_GET_SPARAM(rxfct);
	func = rxfct & RXFCT_FUNCT;

	/* Perform the function */
	switch (func) {
		case RXFCT_RM_CPU:
			/* We don't support this since we already have
			 * a command to do it.  Print a warning message
			 * and fail the request.
			 */
			ka9000_rmcpu_reqs++;
			printf("SPU CPU removal not supported.\n");
			printf("Use /etc/stopcpu instead.\n");
			RXFCT_SET_STATUS(rxfct, 0);
			RXFCT_SET_SPARAM(rxfct, 0);
			rxfct |= RXFCT_VALID;
			mtpr(RXFCT, rxfct);
			break;

		case RXFCT_ADD_CPU:
			/* We don't support this since we already have
			 * a command to do it.  Print a warning message
			 * and fail the request.
			 */
			ka9000_addcpu_reqs++;
			printf("SPU CPU addition not supported.\n");
			printf("Use /etc/startcpu instead.\n");
			RXFCT_SET_STATUS(rxfct, 0);
			RXFCT_SET_SPARAM(rxfct, 0);
			rxfct |= RXFCT_VALID;
			mtpr(RXFCT, rxfct);
			break;

		case RXFCT_MEM_BAD:
			/* We only support this request to the extent
			 * that we print a warning message to the
			 * console.  Future implementations may wish
			 * to add a hook to VM to disable a bad page.
			 */
			printf("VM: SPU reported bad page @0x%x\n", rxprm);
			/* No response expected */
			break;
			
		case RXFCT_SEND_ERRLOG:
                        /* Find the virtual address of the
                         * dmd that we got... (rxprm contains the
                         * physical address...)
                         */

                        for(dmd = spu_dmdlist; dmd; 
					dmd = (struct dmd *)dmd->dmd_link) {
                                if(rxprm == dmd->dmd_paddr) {
                                        /* Found it! */
                                        break;
                                }
                        }

                        if(!dmd) {
                            RXFCT_SET_STATUS(rxfct, 0);
                            RXFCT_SET_SPARAM(rxfct, 0);
			    rxfct |= RXFCT_VALID;
                            mtpr(RXFCT, rxfct);
                            break;
			}

                        /* Process the error log message... */
                        v9000err_spu(dmd->dmd_bufaddr, dmd->dmd_buflen);

                        /* Acknowledge the datagram */
			RXFCT_SET_STATUS(rxfct, 1);
			RXFCT_SET_SPARAM(rxfct, 1);
			rxfct |= RXFCT_VALID;
			mtpr(RXFCT, rxfct);
			ka9000_spu_request(TXFCT_RETURN_DG_STATUS,
				SPU_SUCCESS, rxprm, 0);

			/* Free up the datagram */
			ka9000_dgfree(rxprm);
			break;

		case RXFCT_SEND_OPCOM:
			ka9000_opcom_reqs++;
		    {
			char	*tmpbuf;
			struct dmd *dp;

			/* OPCOM is a VMS-ism, but we may have a use for
			 * it anyway.  If we get an OPCOM message, just
			 * print it to the console.  The printf's will
			 * in turn get error-logged...
			 */

			/* Find the dmd */
			for(dp = spu_dmdlist; dp ; 
					dp = (struct dmd *)dp->dmd_link) {
				if(dp->dmd_paddr == rxprm) break;
			}

			if(!dp) {
				RXFCT_SET_STATUS(rxfct, 0);
				RXFCT_SET_SPARAM(rxfct, 0);
				rxfct |= RXFCT_VALID;
				mtpr(RXFCT, rxfct);
			        break;
			}

			/* Allocate a temporary buffer to hold the
			 * message... */

			KM_ALLOC(tmpbuf, char *, dp->dmd_buflen+1, 
				KM_SPU, KM_NOWAIT);

			if(tmpbuf) {

				/* If we got the buffer, then copy the
				 * message over, null-terminate it, and
				 * print it out.  Otherwise, drop it.
				 */
				bcopy(dp->dmd_bufaddr, tmpbuf, dp->dmd_buflen);
				tmpbuf[dp->dmd_buflen] = '\0';
				printf("OPCOM: %s\n", tmpbuf);
				KM_FREE(tmpbuf, KM_SPU);
			}

			/* Acknowledge the datagram */
			RXFCT_SET_STATUS(rxfct, 1);
			RXFCT_SET_SPARAM(rxfct, 1);
			rxfct |= RXFCT_VALID;
			mtpr(RXFCT, rxfct);
			ka9000_spu_request(TXFCT_RETURN_DG_STATUS,
				SPU_SUCCESS, rxprm, 0);

		    }
		    /* In any case, free up the datagram */
	 	    ka9000_dgfree(rxprm);
		    break;

		case RXFCT_GET_DG:
			/* Allocate datagram... */
			status = ka9000_dgalloc(sparam, &rxprm);

			/* and report our results back to the SPU */
			RXFCT_SET_STATUS(rxfct, status);
			RXFCT_SET_SPARAM(rxfct, status);
			mtpr(RXPRM, rxprm);
			rxfct |= RXFCT_VALID;
			mtpr(RXFCT, rxfct);
			break;

		case RXFCT_SEND_DG:
			/* We shouldn't see any of these... just
			 * quietly acknowledge them...
			 */
			ka9000_spu_request(TXFCT_RETURN_DG_STATUS, 
					SPU_SUCCESS, rxprm, 0);
			/* Free up the datagram */
			ka9000_dgfree(rxprm);
			break;
				
		case RXFCT_RETURN_DG_STATUS:
			/* SPU is done with this datagram and so we
			 * may reallocate it.
			 */
			ka9000_dgfree(rxprm);
			break;

		case RXFCT_SET_KEEPALIVE:
			ka9000_keepalive_enabled = sparam;
			break;

		case RXFCT_ABORT_DATALINK:
			/* We should never see this! */
			RXFCT_SET_STATUS(rxfct, 1);
			RXFCT_SET_SPARAM(rxfct, 1);
			rxfct |= RXFCT_VALID;
			mtpr(RXFCT, rxfct);
			break;

		default:
			printf("Unsupported SPU function 0x%x\n", func);
			RXFCT_SET_STATUS(rxfct, 0);
			RXFCT_SET_SPARAM(rxfct, 0);
			rxfct |= RXFCT_VALID;
			mtpr(RXFCT, rxfct);
	}

	splx(s);
}

/*** Local functions used by ka9000_rxfct_isr() ***/

/* Allocate and initialize a datagram.  NOTE:  This routine assumes that 
 * we're using 32-bit physical addresses.  This will have to change if/when
 * we go to Aquarius 34-bit physical addresses...
 */
ka9000_dgalloc(nbytes, dmdaddrp)
int nbytes;
caddr_t *dmdaddrp;
{
	int	allocpages;
	int	allocbytes;
	int	npfn, i;
	caddr_t	buf_vaddr;
	struct dmd * dmd;

	/* Round up the request to a page-sized boundary... */
	allocbytes = roundup(nbytes, NBPG);
	allocpages = allocbytes / NBPG;

	/* Allocate a page of memory for the dmd header */
	KM_ALLOC(dmd, struct dmd *, NBPG, KM_SPU, KM_NOWAIT);

	/* If we fail, return failure status */
	if(dmd == (struct dmd *)NULL) {
		*dmdaddrp = (caddr_t)NULL;
		return(0);
	}

	/* Allocate page(s) of memory for the datagram */
	KM_ALLOC(buf_vaddr, caddr_t, allocbytes, KM_SPU, KM_NOWAIT);

	/* If we fail, then back out the previous allocation and
	 * return failure status.
	 */
	if(buf_vaddr == (caddr_t)NULL) {
		KM_FREE(dmd, KM_SPU);
		*dmdaddrp = (caddr_t)NULL;
		return(0);
	}

	/* Link this dmd onto the dmd list */
	dmd->dmd_link = (long)spu_dmdlist;
	spu_dmdlist = dmd;

	/* Initialize the dmd... */
	dmd->dmd_paddr = svtophy(dmd) >> 2;
	dmd->dmd_buflen = nbytes;
	dmd->dmd_pfn_count = allocpages;
	dmd->dmd_bufaddr = (long)buf_vaddr;

	dmd->dmd_pfn[0] = svtophy(buf_vaddr)/NBPG;
	for(i = 1; i < allocpages; i++) {
		dmd->dmd_pfn[i] = dmd->dmd_pfn[i-1]+1;
	}

	*dmdaddrp = (caddr_t)dmd->dmd_paddr;
	return(1);
}

/* Inverse of the previous routine; deallocates a previously-allocated
 * datagram.
 */
ka9000_dgfree(paddr)
caddr_t paddr;
{
	struct dmd * dmd_prev;
	struct dmd * dmd;

	/* Walk down the list until we find the DMD
	 * we're looking for...
	 */
	dmd_prev = (struct dmd *)NULL;
	dmd = spu_dmdlist;
	while(dmd) {
		if((caddr_t)dmd->dmd_paddr == paddr) {
			break;
		}
		dmd_prev = dmd;
		dmd = (struct dmd *)dmd->dmd_link;
	}

	if(!dmd) {
		return;
	}


	/* Unlink this DG from the system list */
	if(dmd_prev) {
		dmd_prev->dmd_link = dmd->dmd_link;
	}
	else {
		/* This is the first (or only) DG on the list */
		spu_dmdlist = (struct dmd *)dmd->dmd_link;
	}

	/* Free up the memory. */
	KM_FREE(dmd->dmd_bufaddr, KM_SPU);
	KM_FREE(dmd, KM_SPU);
}


/*** Utility routines for systems programmers ***/


/* Clear warmstart flag */
ka9000_clear_warmstart()
{
	ka9000_spu_request(TXFCT_CLEAR_WS, boot_cpu_num, 0, 1);
}

/* Clear coldstart flag */
ka9000_clear_coldstart()
{
	ka9000_spu_request(TXFCT_CLEAR_CS, boot_cpu_num, 0, 1);
}


/* Reset XJA adapter(s) */
ka9000_io_reset(xjamask)
int xjamask;
{
	/* Do we need to wait for this function to complete? */
	ka9000_spu_request(TXFCT_IO_RESET, xjamask, 0, 1);
}

/* Disable VBOX */
ka9000_disable_vbox(vbmask)
int vbmask;
{
	/* Do we need to wait for this function to complete? */
	ka9000_spu_request(TXFCT_DISABLE_VBOX, vbmask, 0, 1);
}

/* Error logging function */
#define OFFSET_SID 0
#define OFFSET_SYSTYPE 6
#define OFFSET_ENTRYCODE 36
#define SIZE_SID 4
#define SIZE_SYSTYPE 4
#define SIZE_ENTRYCODE 2

#define SIZE_HEADER 48

	/* returns 1 if ealloc() returns full, 0 otherwise */
v9000err_spu(ptr,len)
caddr_t ptr;
int len;
{
	long sid, systype, entrycode;
	int len_b;
	struct el_rec *elp;

	if( len < SIZE_HEADER ) {
		return(0);
	}
	len_b = len - SIZE_HEADER;

	sid = *(long *)(ptr+OFFSET_SID);
	systype = *(long *)(ptr+OFFSET_SYSTYPE);
	entrycode = *(short *)(ptr+OFFSET_ENTRYCODE);

		/* timestamp messages from SPU risk filling up
		   error log buffer in cases where host has been
		   down for extended period of time, and then host
		   reboots and messages arrive from SPU before elcsd
		   is started and can empty error log buffer.
		   Besides, timestamps don't really belong in error
		   log buffer anyway. */
#define SPU_TIMESTAMP_MSG 38
	if( entrycode == SPU_TIMESTAMP_MSG )
		return(0);
	if( (elp=ealloc(len_b, EL_PRIHIGH)) == EL_FULL )
		return(1);

	bcopy(ptr+SIZE_HEADER, &elp->el_body, len_b);
	elp->elrhdr.rhdr_sid = sid;
	elp->elrhdr.rhdr_systype = systype;

	switch(entrycode) {
		case ELSPU_MC:
			LSUBID(elp, ELCT_MCK, ELMCKT_9000SPU,
				EL_UNDEF, EL_UNDEF, EL_UNDEF, EL_UNDEF);
			break;

		case ELSPU_SYNDROME:
			LSUBID(elp, ELCT_9000_SYNDROME, EL_UNDEF,
				EL_UNDEF, EL_UNDEF, EL_UNDEF, EL_UNDEF);
					break;

		case ELSPU_SE:
			LSUBID(elp, ELCT_MEM, EL_UNDEF, 
				ELMCNTR_9000_SE, EL_UNDEF, EL_UNDEF, EL_UNDEF);
					break;

		case ELSPU_HE:
			LSUBID(elp, ELCT_MEM, EL_UNDEF, 
				ELMCNTR_9000_HE, EL_UNDEF, EL_UNDEF, EL_UNDEF);
					break;

		case ELSPU_EMM:
			LSUBID(elp, ELCT_NMIEMM, EL_UNDEF,
				EL_UNDEF, EL_UNDEF, EL_UNDEF, EL_UNDEF);
					break;

		case ELSPU_HLT:
			LSUBID(elp, ELCT_9000_KAF, ELMCKT_9000SPU,
				EL_UNDEF, EL_UNDEF, EL_UNDEF, EL_UNDEF);
					break;

		case ELSPU_BIADPERR:
			LSUBID(elp, ELCT_ADPTR, ELADP_SJASCM,
				EL_UNDEF, EL_UNDEF, EL_UNDEF, EL_UNDEF);
					break;

		case ELSPU_CLKERR:
			LSUBID(elp, ELCT_9000_CLK, EL_UNDEF,
				EL_UNDEF, EL_UNDEF, EL_UNDEF, EL_UNDEF);
					break;

		case ELSPU_SCAN:
			LSUBID(elp, ELCT_9000_SCAN, EL_UNDEF,
				EL_UNDEF, EL_UNDEF, EL_UNDEF, EL_UNDEF);
					break;

		case ELSPU_CONFIG:
			LSUBID(elp, ELCT_9000_CONFIG, EL_UNDEF,
				EL_UNDEF, EL_UNDEF, EL_UNDEF, EL_UNDEF);
					break;
		default:
			LSUBID(elp, EL_UNDEF, entrycode,
				EL_UNDEF, EL_UNDEF, EL_UNDEF, EL_UNDEF);
					break;
	}
	EVALID(elp);
	return(0);
}

ka9000_enable_errlog()
{
	/* Only do this once!!*/

	if(!ka9000_errlog_enabled) {
		ka9000_errlog_enabled = 1;
		/* Enable error log transmission */
		ka9000_spu_request(TXFCT_ENABLE_ERRLOG, 1, 0, 0);
	}
}

/* handles error from XJA (XMI to Jbox adapter) */

ka9000xja_err(xjanum)
int xjanum;
{
	register struct el_rec *elp;
	register struct xmi_reg  *nxv;   /* virtual pointer to XMI node */
	register struct xja_node_regs  *nodep;   /* pointer to XJA node */
	register struct xja_regs *xja;	/* pointer to xja private space */
	register int xja_node, pri, len_b;

		/* compute address of xja private space */
	xja =(struct xja_regs *) (((char *)xja_mem) + xjanum*NBPG);

		/* compute address of xja nodespace */
	xja_node = (xja->xja_cnf >> 12) & 0xf;
	nxv = xmi_start + (xjanum * 16) + xja_node;
	nodep = (struct xja_node_regs *)nxv;

	/* for all errors except the three parity errors, we will panic. */

		/* For ERRS, check all errors other than:(XJA_ERRS_JXDI_PE) */
	pri = EL_PRIHIGH;
	if( xja->xja_errs &
		(XJA_ERRS_JXDI_MPE| XJA_ERRS_XCE_TE| XJA_ERRS_ICU_BC|
		 XJA_ERRS_CPU_RO| XJA_ERRS_RBO| XJA_ERRS_CLE|
		 XJA_ERRS_CBI_PE| XJA_ERRS_XMI_ATO| XJA_ERRS_XMI_PF) )
	{
	    pri = EL_PRISEVERE;
	}

		/* For BER, check all errors other than:(XMI_CC|XMI_PE) */
	if( nodep->xja_node_ber &
		(XMI_NRST | XMI_NHALT | XMI_XFAULT | XMI_WEI | XMI_WSE | 
		 XMI_RIDNAK | XMI_WDNAK | XMI_CRD | XMI_NRR | XMI_RSE | 
		 XMI_RER | XMI_CNAK | XMI_TTO) )
	{
	    pri = EL_PRISEVERE;
	}

	if( (elp=ealloc(sizeof(struct el_xja), pri)) != EL_FULL ) {
	    LSUBID(elp, ELCT_ADPTR, ELADP_XJA,
		EL_UNDEF, xjanum, EL_UNDEF, EL_UNDEF);

		/* Struct xmi_reg has registers present in all nodes on xmi.
		   It does not have registers specific to xja nodes on xmi.
		   Struct xja_node_regs is used to reference these regs. */

	    elp->el_body.el_xja.el_xja_xdev = nodep->xja_node_dev;
	    elp->el_body.el_xja.el_xja_xber = nodep->xja_node_ber;
	    elp->el_body.el_xja.el_xja_xfadra = nodep->xja_node_xfadra;
	    elp->el_body.el_xja.el_xja_xfadrb = nodep->xja_node_xfadrb;
	    elp->el_body.el_xja.el_xja_aosts = nodep->xja_node_aosts;
	    elp->el_body.el_xja.el_xja_sernum = nodep->xja_node_sernum;

	    elp->el_body.el_xja.el_xja_errs = xja->xja_errs;
	    elp->el_body.el_xja.el_xja_fcmd = xja->xja_fcmd;
	    elp->el_body.el_xja.el_xja_ipintrsrc = xja->xja_ipint;
	    elp->el_body.el_xja.el_xja_diag = xja->xja_diag;
	    elp->el_body.el_xja.el_xja_dmafaddr = xja->xja_dmafaddr;
	    elp->el_body.el_xja.el_xja_dmafcmd = xja->xja_dmafcmd;
	    elp->el_body.el_xja.el_xja_errintr = xja->xja_errintr;
	    elp->el_body.el_xja.el_xja_cnf = xja->xja_cnf;
	    elp->el_body.el_xja.el_xja_xbiida = xja->xja_xbiida;
	    elp->el_body.el_xja.el_xja_xbiidb = xja->xja_xbiidb;
	    elp->el_body.el_xja.el_xja_errscb = xja->xja_errscb;
		
	    EVALID(elp);
	}

	if(pri == EL_PRISEVERE)
		panic("xja error");

		/* We panic on all errors except for:
		   XJA_ERRS_JXDI_PE, XMI_CC, XMI_PE.
	   	   So, we need to clear only these specific errors.  */
	xja->xja_errs = XJA_ERRS_JXDI_PE;
	nodep->xja_node_ber = XMI_CC|XMI_PE;
}
