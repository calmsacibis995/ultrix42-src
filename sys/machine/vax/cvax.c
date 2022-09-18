#ifndef lint
static char *sccsid = "@(#)cvax.c	4.5	ULTRIX	4/11/91";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1986,87,88 by			*
 *		Digital Equipment Corporation, Maynard, MA		*
 *			All rights reserved.				*
 *									*
 *   This software is furnished under a license and may be used and	*
 *   copied  only  in accordance with the terms of such license and	*
 *   with the  inclusion  of  the  above  copyright  notice.   This	*
 *   software  or  any	other copies thereof may not be provided or	*
 *   otherwise made available to any other person.  No title to and	*
 *   ownership of the software is hereby transferred.			*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or	reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

/***********************************************************************
 *
 * Modification History:	cvax.c
 *
 * 11-Apr-91	dlh
 *	- (6000 only)
 *	- do not call vp_reset in cca_startcpu
 *	- do write 0 to ACCS (to clear the enable bit) if VP is set disabled in
 *	  the cca - this write is done via cca_send()
 *	  note: question write of 0 to ACCS when CPU is not in vpmask.  
 *		this may not be necessary.
 *
 * 20-dec-90	dlh
 *	added parameter to vp_reset()
 *
 * 10-Oct-90	dlh
 *	add vector support for the Mariah (VAX6000-5xx)
 *
 * 4-Sep-90	dlh
 *	check cca to see if this is a vector-capable machine and if it 
 *	is then allocate the cpu vpdata structure and set up global 
 *	variables
 *
 * 03-Jun-90    jas
 *	Modified cca_startcpu() to not do INIT command if cpu is
 *	an xmp.  Modified cca_send() such that delay is 100000 for RXRDY
 *	clearing if cpu is xmp.
 * 
 * 10-Nov-89	jaw
 *	move machdep KMALLOC to ccastartcpu for ka6200 because 
 * 	structure cannot be kmalloced on the IS.
 *
 * 30-Oct-89    Szczypek
 *      Added check for disabled processor in cca_startcpu().  If
 *      cpu disabled, fact is noted in error log.  Check range is
 *      from node 0->31.
 *      
 * 17-Jul-89	Darrell A. Dunnuck (darrell)
 *	Removed the printf for VAX_60 that printed the stkpaddr in
 *	cca_startcpu().
 *
 * 30-May-89	darrell
 *	Replaced include of cpu.h with include of cpuconf.h.
 *
 * 10-Jan-89	Kong
 *	Added Rigel (VAX6400) support.  Although Rigel is different
 *	from CVAX, it uses the same CCA routines.
 *
 *  26-Jan-89	jaw
 *	fix up start/stop cpu.
 *
 * 16-Aug-88	robin
 *	Removed \n from print statements (as per Darrell's request);
 *	removed the ifdiff'ed code that was never used; and marked 
 *	"dk" to -1 so iostat and vmstat did not think the "ln" device
 *	was a disk.
 *
 * 06-Mar-88	darrell
 *	creation of this file to contain cvax specific code that
 *	can be common to all machines with a CVAX processor.
 *
 **********************************************************************/

#include "../h/types.h"
#include "../h/param.h"
#include "../h/buf.h"
#include "../h/config.h"
#include "../h/dk.h"
#include "../machine/vectors.h"
#include "../h/cpudata.h"
#include "../h/time.h"
#include "../h/errlog.h"
#include "../h/vmmac.h"
#include "../h/kmalloc.h"
#include "../machine/pte.h"
#include "../machine/scb.h"
#include "../machine/rpb.h"
#include "../io/uba/ubareg.h"
#include "../io/uba/ubavar.h"
#include "../../machine/common/cpuconf.h"
#include "../machine/cvax.h"
#include "../h/cpudata.h"
#include "../machine/ka6200.h"
#include "../machine/mtpr.h"
#include "../machine/ka6400.h"

/* timers & counters for errors:    used with following error bit... */

struct cfpa_errcnt cfpa_errcnt;		/* machine checks 1 thru 4 */
struct cdal_errcnt cdal_errcnt;		/* MSER_MCD */
struct cache_errcnt cache_errcnt;	/* MSER_MCC */
struct qnxm_errcnt qnxm_errcnt;		/* DSER_QNXM */
struct qngr_errcnt qngr_errcnt;		/* DSER_NOGRANT */
struct qpe_errcnt qpe_errcnt;		/* DSER_QPE */
struct dnxm_errcnt dnxm_errcnt;		/* DSER_DNXM */
struct crd_errcnt crd_errcnt;		/* MEM_CRD */
struct cdalW_errcnt cdalW_errcnt;	/* MEM_CDAL */
struct rdsW_errcnt rdsW_errcnt;		/* MEM_RDS */
struct tag_errcnt tag_errcnt;		/* CACR_CPE */

/*
 * Machine Check codes for CVAX CPUs.
 */
char *mcCVAX[] = {
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

extern int dkn;         /* number of iostat dk numbers assigned so far */
extern int nNUBA;
extern int cpu;
extern int cpu_sub_subtype;


/*
 * Name:	ib_config_cont (nxv, slot, name);
 *
 * Args:	nxv	- The virtual address of the "controller"
 *
 *		nxp	- The physicall address of the "controller"
 *
 *		slot	- The mbus slot number containing the "controller"
 *
 *		name	- The name of the "controller" to match with in the
 *			  ubminit and ubdinit structures
 *
 *		scb_vec_addr - The offset from the begining of scb block
 *				 zero that you want the address of the 
 *				 interrupt routine specified in the um
 *				 structure inserted.  If the value equals
 *				 zero, do not insert the the address of the
 *				 interrupt routine into the scb.
 *
 * Returns:	1 - if the "controller" was found
 *		0 - if the "controller" wasn't found
 */
ib_config_cont(nxv, nxp, slot, name, scb_vec_addr)
char *nxv;
char *nxp;
u_long slot;
char *name;
int scb_vec_addr;
{
	register struct uba_device *ui;
	register struct uba_ctlr *um;
	register struct uba_driver *udp;
	register struct config_adpt *p_adpt;
	extern struct config_adpt config_adpt[];
	int (**ivec)();
	int i;
	int found = 0;

	um = ubminit;
	while (found == 0 && (um->um_driver)) {
	    if ((um->um_adpt == slot) && (strcmp(um->um_ctlrname, name) == 0) &&
		(um->um_alive == 0) && (um->um_ubanum == -1)) {
		    found = 1;
	    }
	    else {
	    	if ((um->um_adpt == '?') && (strcmp(um->um_ctlrname, name) == 0) &&
		    (um->um_alive == 0) && (um->um_ubanum == -1)) {
			found = 1;
		}
		else {
		    um++;
		}
	    }
	}

	if (found == 0) {
	    return(0);
	}
	 

	udp = um->um_driver;
	for( p_adpt = &config_adpt[0]; p_adpt->p_name; p_adpt++) {

	/* first look for iobus entry for this board then
	 * check if the driver is correct for this board then
 	 * check that the controller number is the same as the
	 * iobus entry then
	 * check that this iobus entry has not been used
	 */
			
	    if (strcmp("ibus", p_adpt->p_name)==0 && 
		(char *)udp == p_adpt->c_name &&
		p_adpt->c_num == um->um_ctlr  &&
		p_adpt->c_ptr == 0) {
		    found = 1;
	    }
	}

	if (found = 0) {
	    return(0);
	}
/* DAD - do this here for now.  Need to set up the vector since
 * sz_siiprobe goes and looks for devices
 */
	if (scb_vec_addr)
	    ibcon_vec(scb_vec_addr, um);

	i = (*udp->ud_probe)(nxv);
	if (i == 0)
	    return(0);
	um->um_adpt = slot;
	um->um_alive = (i ? 1 : 0);
	um->um_addr = (char *)nxv;
	um->um_physaddr = (char *)svtophy(um->um_addr);
	udp->ud_minfo[um->um_ctlr] = um;
	config_fillin(um);
	printf("\n");


	for (ui = ubdinit; ui->ui_driver; ui++) {
	    if (ui->ui_driver != udp || ui->ui_alive ||
		ui->ui_ctlr != um->um_ctlr && ui->ui_ctlr != '?') {
		    continue;
	    }
	    if ((*udp->ud_slave)(ui, nxv)) {
		ui->ui_alive = 1;
		ui->ui_ctlr = um->um_ctlr;
		ui->ui_addr = (char *)nxv;
		ui->ui_ubanum = um->um_ubanum;
		ui->ui_hd = um->um_hd;
		ui->ui_physaddr = nxp;
		ui->ui_adpt = slot;

		if (ui->ui_dk && dkn < DK_NDRIVE)
		    ui->ui_dk = dkn++;
		else
		    ui->ui_dk = -1;
		ui->ui_mi = um;
		/* ui_type comes from driver */
		udp->ud_dinfo[ui->ui_unit] = ui;
		if(ui->ui_slave >= 0) {
		    printf("%s%d at %s%d slave %d",
			ui->ui_devname, ui->ui_unit,
			udp->ud_mname, um->um_ctlr, ui->ui_slave);
		}
		else {
		    printf("%s%d at %s%d",
			ui->ui_devname, ui->ui_unit,
			udp->ud_mname, um->um_ctlr);
		}
		(*udp->ud_attach)(ui);
		printf("\n");
	    }
	}
	return (found);
}

/*
 * Name:	ib_config_dev (nxv, nxp, slot, name);
 *
 * Args:	nxv	- The virtual address of the "device"
 *
 *		nxp	- The physical address of the "device"
 *
 *		slot	- The ibus slot number containing the "device"
 *
 *		name	- The name of the "device" to match with in the
 *			  ubminit and ubdinit structures
 *
 *		scb_vec_addr - The offset from the begining of scb block
 *				 zero that you want the address of the 
 *				 interrupt routine specified in the um
 *				 structure inserted.  If the value equals
 *				 zero, do not insert the the address of the
 *				 interrupt routine into the scb.
 *
 * Returns:	1 - if the "device" was found
 *		0 - if the "device" wasn't found
 */
ib_config_dev(nxv, nxp, slot, name, scb_vec_addr)
char *nxv;
char *nxp;
u_long slot;
char *name;
int scb_vec_addr;
{
	register struct uba_device *ui;
	register struct uba_ctlr *um;
	register struct uba_driver *udp;
	register struct config_adpt *p_adpt;
	extern struct config_adpt config_adpt[];
	int (**ivec)();
	int i;
	int found = 0;
	
	ui = ubdinit;
	while (found == 0 && (ui->ui_driver)) {
	    if ((ui->ui_adpt == slot) && (strcmp(ui->ui_devname, name) == 0) &&
		(ui->ui_alive == 0) && (ui->ui_slave == -1)) {
		        found = 1;
	    }
	    else {
		if ((ui->ui_adpt == '?') && (strcmp(ui->ui_devname, name) == 0) &&
		    (ui->ui_alive == 0) && (ui->ui_slave == -1)) {
		        found = 1;
	    	}
		else {
		    ui++;
		}
	    }
	}

	if (found == 0)
	    return(0);
	udp = ui->ui_driver;
	i = (*udp->ud_probe)(nxv);
	if (i == 0) {
	    return(0);
	}
	ui->ui_adpt = slot;
	config_fillin(ui);
	printf("\n");
	if (scb_vec_addr)
	    ibdev_vec(scb_vec_addr, ui);
	ui->ui_alive = (i ? 1 : 0);
	ui->ui_addr = (char *)nxv;
	ui->ui_dk = -1;
	udp->ud_dinfo[ui->ui_unit] = ui;
	(*udp->ud_attach)(ui);
	return (found);
}

/*
 * ibdev_vec(): To set up Mbus device interrupt vectors.
 * It is called with 3 parameters:
 *	slot	   - The ibus slot number
 *	scb_vec_addr - The offset from the start of slot specific
		     vector space
 *	ui:	   - the device structure (for names of interrupt routines)
 */

ibdev_vec(scb_vec_addr, ui)
int scb_vec_addr;
struct uba_device *ui;
{
    register int (**ivec)();
    register int (**addr)();	/* double indirection neccessary to keep
			   	   the C compiler happy */

    ivec = ui->ui_intr;
    addr = (int (**)())scb_vec_addr;
    *addr = scbentry(*ivec,SCB_ISTACK);
}

ibcon_vec(scb_vec_addr, um)
int scb_vec_addr;
struct uba_ctlr *um;
{
    register int (**ivec)();
    register int (**addr)();	/* double indirection neccessary to keep
			   	   the C compiler happy */

    ivec = um->um_intr;
    addr = (int (**)())scb_vec_addr;
    *addr = scbentry(*ivec,SCB_ISTACK);
}

cca_setup()
{
/* return the number of configured CPUs which are still in console mode */
	register int i, j;
	register int cpustarted;
	register int num_cpu = 0;
	register int stkpaddr;
	u_char *cca_phys;
	int timeout;

	cca_phys = (u_char *)rpb.cca_addr;  /* get CCA addr from RPB */

#ifdef VAX6200 || VAX6400
	if ((cpu == VAX_6200) || (cpu == VAX_6400))
		nxaccess(cca_phys,CCAmap,4096);  
#endif VAX6200
#ifdef VAX60
	if (cpu == VAX_60)
		nxaccess(cca_phys,CCAmap, (12*512));  
#endif VAX60


	/* sanity check CCA */
	if (cca_phys != (u_char *)ccabase.cca_base ||
	    (ccabase.cca_indent0 != 'C') ||
	    (ccabase.cca_indent1 != 'C')) {
		panic("no CCA");
	}


	/* the only CPU which is running at this point is the boot
	 * CPU.  All other CPUs are still in console mode.  Therefore to
	 * determine how many CPUs are configured, count the CPUs in
	 * console mode, and add 1.  (note: the 1 is added by the caller
	 * of this routine.
	 */
	for ( cpustarted=0, i=0; i < ccabase.cca_nproc; i++ ) {
		if (ccabase.cca_console & (1 << i)) {
			num_cpu++;
		}
	}

	return(num_cpu);
}

extern	int	max_vec_procs;

char *hextochar = "0123456789ABCDEF";
char *init_slave_stack = "d sp XXXXXXXX\r";
char *init_cpu = "I X\r";

cca_startcpu(cpunum)
int cpunum;
{
	register int j;
	register int stkpaddr;
	int timeout;

	if (ccabase.cca_console & (1 << cpunum)) {

	    /* Check for enabled CPU (assume no more than 32 CPUs) */
  	    if (ccabase.cca_enabled & (1 << cpunum)) {
	      get_cpudata(cpunum);

	      if (cpu == VAX_6200) {
		if (!CPUDATA(cpunum)->cpu_machdep) 
			/*Allocate memory to store machine check information */
			KM_ALLOC((CPUDATA(cpunum)->cpu_machdep), char *,
		      		sizeof(struct xcp_machdep_data), 
		       		KM_MBUF,KM_CLEAR | KM_CONTIG);
	      }

 	      if (cpu == VAX_60) {
	    	cca_send("I\r", cpunum);
		/* 
		 * Since the VAX_60 console code requires us to 
		 * point it to a valid stack before sending the
		 * start command, (which appears to be a violation
		 * of the VAX SRM) we will set the stack pointer 
		 * to the address of the RX buffer in the CCA for
		 * this processor.  This will cause the first push
		 * to start at the end of the TX buffer in the CCA
		 * for this processor.
		 */
		stkpaddr = svtophy(ccabase.cca_buf[cpunum].rx);
		for(j = 12; init_slave_stack[j] == 'X'; j--) {
		    init_slave_stack[j] = hextochar[stkpaddr & 0xf];
		    stkpaddr = stkpaddr >> 4;
			}
		cca_send(init_slave_stack, cpunum);
	      } else { 
	    	  /*
	    	   * Init the slave
	    	   */
	    	  init_cpu[2] = hextochar[cpunum];
		  if((cpu != VAX_6400) || ((cpu == VAX_6400) && (cpu_sub_subtype != MARIAH_VARIANT)))
	    	  	cca_send(init_cpu, cpunum);  /* Don't INIT if XMP */
	      }

              /*
               * cca revision determines whether or not there might be an
	       * attached vector processor.  vector processors are attached 
	       * to scalar cpu's only * after cca rev #3 
               */
              if ((cpu == VAX_6400) && (ccabase.cca_b_revision > 3)) {

		 if (max_vec_procs > 0) {

		      /* if there is a scalar in this slot ... */
                      if ( CPUDATA(cpunum) ) {

			      /* allocate memory for the vpdata structure */
                              KM_ALLOC (( CPUDATA ( cpunum)-> cpu_vpdata), 
                                      struct vpdata *, sizeof(struct vpdata), 
                                      KM_VECTOR , KM_CLEAR | KM_CONTIG);

			      /* if this scalar has an attached vector ...  */

			      /* NOTE: I am commenting out the call to 
			       * vp_reset.  Since we are running on the boot 
			       * processor, and starting a secondary, 
			       * vp_reset would be accessing the wrong 
			       * registers if called from here.  the call to 
			       * vp_reset has been moved to _slavestart in 
			       * locore.s
			       */

			      /* test to see if there is an enabled VP on 
			       * cpunum.  If there is not an enabled VP, then
			       * clear the vector enabled bit
			       */

                              if (! (vpmask & CPUDATA(cpunum)->cpu_mask) ) {
				/* disable the vector processor by writing a 
				 * 0 to the vector present bit in the ACCS 
				 * register of cpunum.  ACCS is IPR #40 
			         * (0x28).
				 */
				 cca_send ("D/I 28 0\r", cpunum);
			      }
                      }
		 } else {
		      /*
		       * disable the vector processor by writing a 0 to
		       * the vector present bit in the ACCS register of
		       * cpunum.  ACCS is IPR #40 (0x28).
		       */
		      cca_send ("D/I 28 0\r", cpunum);
		 }
              }

	      /* Start slave cpu at address 100 */
	      cca_send("S 100\r", cpunum);
	      timeout=1000;
	      while ( (ccabase.cca_console & (1<<cpunum)) ) {
	    	  DELAY(10000)
		  if (--timeout < 0) break;
	      }


	      if(!(ccabase.cca_console & (1 << cpunum))) {
	 	  return(1);
	      }
	    } else {
		  /* note disabled cpu in error log */
		  mprintf("CPU %x disabled\n",cpunum);
	    }
	}
	return(0);
}

cca_send(str,cpunum) 
register char *str;
register int cpunum;
{

	register int index;

	int timeout;

	timeout=1000;
	while ( (ccabase.cca_buf[cpunum].flags & RXRDY) != 0) {
		DELAY(10000);
		if (--timeout < 0) {
			printf("processor %x not ready\n",cpunum);
			return;
		}
	}

	index = 0;
	while (*str != '\0') {
		ccabase.cca_buf[cpunum].rx[index] = *str;
		str++;
		index++;
	}
	ccabase.cca_buf[cpunum].rxlen = index;
	ccabase.cca_buf[cpunum].flags |= RXRDY;

	timeout=1000;
	while ( (ccabase.cca_buf[cpunum].flags & RXRDY) != 0) {
	        if((cpu == VAX_6400) && (cpu_sub_subtype == MARIAH_VARIANT)) {
			DELAY(100000);    /* Extra delay for XMP */
		}
		else { 
			DELAY(10000);
		}
		if (--timeout < 0) {
			printf("processor %x not ready\n",cpunum);
			return;
		}
	

	}

}		

/*
 * Prints the console message from a slave processor on the console
 */
cca_print(cpunum)
register int cpunum;
{

	int index;
	char *cp;

	if (ccabase.cca_ready & (1 << cpunum)) {
		cp = ccabase.cca_buf[cpunum].tx;
		printf("cpunum = 0x%x, flag = 0x%x, rxlen = 0x%x, txlen = 0x%x\n",
			cpunum,
			ccabase.cca_buf[cpunum].flags,
			ccabase.cca_buf[cpunum].rxlen,
			ccabase.cca_buf[cpunum].txlen);

		for (index = 0; index <= ccabase.cca_buf[cpunum].txlen; index++) {
			putchar((struct el_msg *)0, cp++, 0);
		}
		ccabase.cca_ready |= (ccabase.cca_ready & ~( 1 << cpunum));
	}
}

/*
 * Stop the processor that is currently running.
 *
 */

cca_stopcpu() {

	spl7();
	CURRENT_CPUDATA->cpu_state &= ~CPU_RUN;
	bbssi((CURRENT_CPUDATA->cpu_num),&ccabase.cca_secstart);
	bbssi((CURRENT_CPUDATA->cpu_num),&ccabase.cca_restartip);
	asm("halt");
}


cca_check_input() {
	register int i,j;

	if(ccabase.cca_ready) {
		for (i = 0 ; i< 32; i++) {
		    if (ccabase.cca_ready & (1<<i)){ 		
		    	/* decide what to do with the message */
			cca_decode_message(i);
			bbcci(i,&ccabase.cca_ready);
		    }
		}
	}
}
/*
 *	look for halt code in the message.  If it is a ?06 and the 
 * 	percpu structure says this processor was halting then the processor
 *	was stopped in a "nice" manner.  If it is a ?02 (ctrl-P) then assume
 *	debugging is taking place.  Otherwise an error halt took
 *	place and it is time to bring down the system.
 *
 */

cca_decode_message(i)
int i;
{
	int j;
/*
ccabase.cca_buf[i].tx[ccabase.cca_buf[i].txlen] = 0;
printf("CPU %x:  %s\n",i,ccabase.cca_buf[i].tx);
*/

	for(j = 0; j <ccabase.cca_buf[i].txlen; j++) {
		/* find start of halt code */
		if (ccabase.cca_buf[i].tx[j] == '?') {

		    if ((ccabase.cca_buf[i].tx[j+1] == '0')&&
		    	(ccabase.cca_buf[i].tx[j+2] == '6') &&
			(CPUDATA(i)->cpu_state & CPU_STOP)) {
			/* Halt instruction when stopping cpu */
			uprintf("CPU %d was halted \n",i);

		    } else if ((ccabase.cca_buf[i].tx[j+1] == '0')&&
		    	(ccabase.cca_buf[i].tx[j+2] == '2')) {
			/* stopped by a control-P.  Assume that someone
			   is debugging using the console */
			printf("CPU %d was stopped by ctrl-P\n",i);

		   } else {
		    	/* ERROR HALT */
		    	ccabase.cca_buf[i].tx[j+3]=0; /* clear string end */
		    	printf("CPU %d halted with code %s\n",
				i,&ccabase.cca_buf[i].tx[j]);
			panic("secondary halted");
		    }

		}
		
	}

}




