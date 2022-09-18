#ifndef lint
static char *sccsid = "@(#)biinit.c	4.2	ULTRIX	10/10/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1984,86,87,88 by			*
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
 *
 * 10-Oct-90    Stuart Hollander
 *	Added 9000 specific code to bierrors()
 *
 * 30-Nov-89    Paul Grist
 *      Modified vaxbierrors() to additional error checking to get a more
 *      complete picture of the machine before it panics.
 *
 * 06-Nov-89    Paul Grist
 *      Created log_bierrors() routine by extracting and modifying code
 *      from vaxbierrors(). It can now be used by all exception handlers
 *      which need to check a VAXBI for errors, and to log them.
 *
 * 09-Nov-89	jaw
 *	fix bug where xna was being hooked to wrong bus.
 *
 * 20-Jul-89    rafiey (Ali Rafieymehr)
 *	Moved is_adapt_alive() routine to autoconf.c to be shared by
 *	xmi.
 *
 * 24-May-89	darrell
 *	Removed the v_ prefix from all cpusw fields, removed cpup from any
 *	arguments being passed in function args.  cpup is now defined
 *	globally -- as part of the new cpusw.
 *
 * 24-May-89	darrell
 *	Changed the #include for cpuconf.h to find it in it's new home --
 *	sys/machine/common/cpuconf.h
 *
 * 19-Jan-88 -- jaw
 *	add support for config generating error vectors and xmierror logging.
 *
 * 08-Jan-1988		Todd M. Katz
 *	The configuration adapter structure has been notified to include
 *	fields for bus and nexus numbers, and parameters have been added
 *	to the routine config_set_alive() so that it may set these fields.
 *	Appropriately modify all invocations of this function within this
 *	module.
 *
 * 6-Aug-86   -- jaw	bi device autoconf wasn't setting ui_dk to -1.  This
 *			caused iostat and vmstat programs to have bad headers.
 *			Added routinues to prevent errors while sizer is 
 *			running.
 *
 * 10-Jul-86   -- jaw	added adpt/nexus to ioctl
 *
 * 5-Jun-86   -- jaw 	changes to config.
 *
 * 14-May-86 -- pmk	Changed where binumber gets logged, now in LSUBID
 *
 * 23-Apr-86 -- pmk 	Changed logbua() to logbigen()
 *
 * 18-Apr-86 -- jaw	hooks for nmi faults and fixes to bierrors.
 *
 * 16-Apr-86 -- darrell
 *	badaddr is now called via the macro BADADDR.
 *
 * 09-Apr-86 -- lp
 *	Couple of changes in config_cont for bvp nodes.
 *
 * 04-Apr-86 -- afd
 *	Added bidev_vec routine for setting up BI device interrupt vectors.
 *
 * 02-Apr-86 -- jaw  Fix bi autoconf for mutilple bi's.
 *
 * 09-Mar-86  darrell -- pointer to cpusw structure now passed in.
 *	No longer use a global reference.
 *
 * 05-Mar-86 -- jaw  VAXBI device and controller config code added.
 *		     todr code put in cpusw.
 *
 * 19-Feb-86 -- jrs
 *	Removed ref. to TODR as 8800 doesn't have one.  Also fix check
 *	for multiple bi errs to handle correctly before clock is set.
 *
 * 18-Mar-86 -- jaw  add routines to cpu switch for nexus/unibus addreses
 *		     also got rid of some globals like nexnum.
 *		     ka8800 cleanup.
 *
 *	12-Feb-86 -- pmk   Added errlogging of bi & bua errors
 *	
 * 	04-feb-86 -- jaw   get rid of biic.h.
 *
 *	06-Dec-85 -- jaw   infinite retry loop fix.
 *
 *	11-Nov-85 -- jaw   fix so we won't try to recover twice from a 
 *                         from a bi error.
 *
 *	16-Oct-85 -- jaw   bug fixes for BI error handling.
 *
 *	27-Sep-85 -- jaw   more print message fixes.
 *
 *	11-Sep-85 -- jaw   put in offical names of BI devices.
 *
 *	04-Sep-85 -- jaw   Add bi error interrupt code.
 *
 *	08-Aug-85 -- lp	   Add rev number to found nodes.
 *
 *	26-Jul-85 -- jaw   fixup BI vector allocation.
 *
 *	27-Jun-85 -- jaw   memory nodes always come-up broke.
 *
 * 	19-Jun-85 -- jaw   VAX8200 name change.
 *
 *	05 Jun 85 -- jaw   code clean-up and replace badwrite with bisst.
 *
 * 	20 Mar 85 -- jaw   add support for VAX 8200
 *
 *
 * ------------------------------------------------------------------------
 */


#include "../machine/pte.h"
#include "../h/param.h"
#include "../h/systm.h"
#include "../h/map.h"
#include "../h/buf.h"
#include "../h/time.h"
#include "../h/kernel.h"
#include "../h/errlog.h"
#include "../../machine/common/cpuconf.h"
#include "../h/dk.h"
#include "../h/config.h"
#include "../h/vmmac.h"

#include "../machine/cpu.h"
#ifdef vax
#include "../machine/ka8800.h"
#include "../machine/ka9000.h"
#include "../machine/mtpr.h"
#endif vax
#include "../machine/nexus.h"
#include "../machine/scb.h"
#include "../io/uba/ubareg.h"
#include "../io/uba/ubavar.h"
#include "../io/bi/bireg.h"
#include "../io/bi/buareg.h"
#include "../io/bi/bimemreg.h"
extern int numuba;
extern int nbicpus;
extern int nbitypes;
extern int dkn;		/* number of iostat dk numbers assigned so far */
extern int ignorebi;
extern struct bisw bisw[];
extern struct bidata bidata[];
#ifdef vax
extern int catcher[256];
#endif vax
#ifdef mips
extern int	stray();
#endif mips
extern int nNUBA;

#define BIMEMTEST 0x00007f00

extern struct bus_dispatch vaxbierr_dispatch[];
extern struct config_adpt  *ni_port_adpt;

bisetvec(binumber)
int binumber;
{
	int i;

	for(i=0; vaxbierr_dispatch[i].bus_num != binumber ; i++) {
		if (vaxbierr_dispatch[i].bus_num == -1) panic("no vector");

	}
	
	*(bidata[binumber].bivec_page+(BIEINT_BIVEC/4))=
				scbentry(vaxbierr_dispatch[i].bus_vec,SCB_ISTACK);

}

probebi(binumber)
int 	binumber;
{
	register struct bi_nodespace *nxv;
	register struct bi_nodespace *nxp;
	register struct bisw *pbisw;
	int foo;
	register int i,vec;
	int broke;
	int binode;
	int alive;

	/* set VAXBI alive in adpter struct */
	config_set_alive("vaxbi",binumber,binumber,-1);

	/* ingnore BI errors when doing autoconf */
	ignorebi = 1;

	/* bit mask of active nodes */
	bidata[binumber].binodes_alive = 0;
	nxv =  bidata[binumber].bivirt;
	nxp =  bidata[binumber].biphys;

	/* don't initialize first half of page on 8200 */
#ifdef vax
	if (cpu == VAX_8200)
		i=64;
	else
#endif vax
		i=0;

	/* initialize SCB to catcher */
	for ( ; i < 128; i++)
			*(SCB_BI_ADDR(binumber) + i) =
#ifdef vax
			    scbentry(&catcher[i*2], SCB_ISTACK);
#endif vax
#ifdef mips
			    scbentry(stray, 0);
#endif mips

	bisetvec(binumber);

	for(binode=0; binode < NBINODES; binode++, nxv++ ,nxp++) {
			
	    /* map bi node space */
#ifdef vax
	    nxaccess(nxp,&Sysmap[btop((int)(nxv) & ~VA_SYS)], BINODE_SIZE);
#endif vax
#ifdef mips
	    nxaccess(nxp,&Sysmap[btop((int)(nxv) & ~K2BASE)], BINODE_SIZE);
#endif mips
		
	    /* bi node alive ??? */
	    if (BADADDR((caddr_t) nxv,sizeof(long))) continue;

	    /* find active device in bi list */
	    for (pbisw = bisw ; pbisw->bi_type ; pbisw++) {	
		if (pbisw->bi_type == (short)(nxv->biic.biic_typ)) {

		    bidata[binumber].binodes_alive |= (1 << binode);
	    	    bidata[binumber].bierr[binode].pbisw= pbisw;
				
		    if (pbisw->bi_flags&BIF_SST) 
				foo=bisst(&nxv->biic.biic_ctrl);
				
		    if ((nxv->biic.biic_typ & BIMEMTEST) == 0)
			broke=((struct bimem *)nxv)->bimem_csr1&BICTRL_BROKE;
		    else broke = nxv->biic.biic_ctrl & BICTRL_BROKE;
					

		    /* set BI interupt dest to be cpu */
		    if (pbisw->bi_flags & BIF_SET_HEIE) 
			   nxv->biic.biic_int_dst=bidata[binumber].biintr_dst;

		    alive=0;	
		    /* this is code special to the DEBNK that allows the NI
		    	port to only be used on a LYNX (8200) system. */
		    ni_port_adpt = 0;
		    if((pbisw->bi_type == BI_AIE_TK) ||
		    	(pbisw->bi_type == BI_AIE_TK70)) {
#ifdef vax
		    	if (cpu == VAX_8200)
			    alive|=bi_config_dev(nxv,nxp,binumber,binode);
#endif vax
#ifdef mips
			/* Don't call config_dev on ISIS for the network side
			 * of an aie card, we don't support it.
			 */;
#endif mips

		    } else if (pbisw->bi_flags&BIF_DEVICE) 
			alive |= bi_config_dev(nxv,nxp,binumber,binode);

		    if (pbisw->bi_flags&BIF_CONTROLLER) 
			alive |= bi_config_cont(nxv,nxp,binumber,binode);

		    if (pbisw->bi_flags&BIF_ADAPTER){ 
		    	(**pbisw->probes)(nxv,nxp,binumber,binode);
			alive=1;
		    }
		    if (pbisw->bi_flags&BIF_NOCONF) {
			printf ("%s at vaxbi%d node %d",
				pbisw->bi_name,binumber,binode);
	    		if (broke == 0)
				printf("\n");
	    		else
				printf(" is broken, continuing!\n");
		    	(**pbisw->probes)(nxv,nxp,binumber,binode);
			alive=1;
		    }
		    if (alive==0) binotconf(nxv,nxp,binumber,binode);

		    break;
		} 
	    }
	    if (pbisw->bi_type == 0) 
		printf ("vaxbi%d node %d, unsupported device type 0x%x\n",
			binumber,binode, (short) nxv->biic.biic_typ);
	}
	/* set up error vec offset */
	vec = (SCB_BI_OFFSET(binumber))+BIEINT_BIVEC;

	/* set up BI hard error interrupt for proper nodes */	
	nxv =  bidata[binumber].bivirt;
	for (binode = 0; binode < NBINODES; binode++,nxv++) {
		pbisw = bidata[binumber].bierr[binode].pbisw;

		if ((bidata[binumber].binodes_alive & (1 << binode)) &&
			(pbisw->bi_flags & BIF_SET_HEIE)) {

			/*clear out any remaining errors in cpu BIIC*/
	
			nxv->biic.biic_err = nxv->biic.biic_err; 
		 	nxv->biic.biic_err_int = (nxv->biic.biic_err_int &
				~(BIEINT_VECTOR|BIEINT_LEVEL|BIEINT_FORCE))
	 		 	|(BIEINT_5LEVEL|vec);

		    	nxv->biic.biic_ctrl = (BICTRL_HEIE) | 
					(nxv->biic.biic_ctrl&~(BICTRL_BROKE));
		}
	}
	ignorebi = 0;
}

struct uba_ctlr *bifindum(); 

bi_config_cont(nxv,nxp,binumber,binode)
struct bi_nodespace *nxv;
char *nxp;
int 	binumber;
int 	binode;
{
	register struct uba_device *ui;
	register struct uba_ctlr *um;
	register int (**biprobe)();
	register struct uba_driver *udp;
	register struct bisw *pbisw = bidata[binumber].bierr[binode].pbisw;
	int level;
	int (**ivec)();
	int found = 0;

	
	for ( biprobe =  pbisw->probes; *biprobe; biprobe++) {
		if ((um = bifindum(binumber,binode,biprobe,pbisw->bi_name)) == 0)
			if ((um = bifindum(binumber,'?',biprobe,pbisw->bi_name))==0) 
				if ((um = bifindum('?','?',biprobe,pbisw->bi_name))==0) 

					continue;
		found =1;
                if (((*biprobe)(nxv,nxp,binumber,binode,um))
			== 0){
			continue;
		}
		um->um_adpt = binumber;
		um->um_nexus = binode;
		um->um_alive = 1;
		um->um_addr = (char *)nxv;
		um->um_physaddr = (char *)svtophy(um->um_addr);
		udp = um->um_driver;
		udp->ud_minfo[um->um_ctlr] = um;
		config_fillin(um);
		printf("\n");

#define V2.4
#ifndef V2.4
		level = LEVEL15;
		bicon_vec(binumber, binode, level, um);
#endif V2.4
		for (ui = ubdinit; ui->ui_driver; ui++) {
			if (ui->ui_driver != udp || ui->ui_alive ||
			    ui->ui_ctlr != um->um_ctlr && ui->ui_ctlr != '?')
				continue;
			if ((*udp->ud_slave)(ui, nxv)) {
				ui->ui_alive = 1;
				ui->ui_ctlr = um->um_ctlr;
				ui->ui_addr = (char *)nxv;
				ui->ui_ubanum = um->um_ubanum;
				ui->ui_hd = um->um_hd;
				ui->ui_physaddr = nxp;
				ui->ui_adpt = binumber;
				ui->ui_nexus = binode;

				if (ui->ui_dk && dkn < DK_NDRIVE)
					ui->ui_dk = dkn++;
				else
					ui->ui_dk = -1;
				ui->ui_mi = um;
				/* ui_type comes from driver */
				udp->ud_dinfo[ui->ui_unit] = ui;
				if(ui->ui_slave >= 0)
				printf("%s%d at %s%d slave %d\n",
				    ui->ui_devname, ui->ui_unit,
				    udp->ud_mname, um->um_ctlr, ui->ui_slave);
				else
				printf("%s%d at %s%d\n",
				    ui->ui_devname, ui->ui_unit,
				    udp->ud_mname, um->um_ctlr);

				(*udp->ud_attach)(ui);
			}
	    }


	}
	return(found);
}


struct uba_ctlr *bifindum(binumber,binode,biprobe,biconn)
register int binumber;
register int binode;
register int (**biprobe)();
register char *biconn;
{
	struct uba_driver *udp;
	register struct uba_ctlr *um;
        register struct config_adpt *p_adpt;
        struct config_adpt *p_aie;
        extern struct config_adpt config_adpt[];



	for (um=ubminit; udp =um->um_driver; um++) {
		/* first check that the drivers probe routine equals the 
		 * bi's probe routine then 
		 * crosscheck binumber with um's bi number then
		 * crosscheck binode number with um's node number then
		 * make sure not alive already then
		 * make sure its not a valid unibus number.
		 */
	
		if ((udp->ud_probe != *biprobe) ||
		    (um->um_adpt != binumber) ||
		    (um->um_nexus != binode) ||
		    (um->um_alive) ||
		    ((um->um_ubanum >=0) && ((um->um_ubanum < nNUBA)
		        		 || (um->um_ubanum == '?'))))continue;

                for(p_adpt = &config_adpt[0];p_adpt->p_name; p_adpt++) {

			/* first look for iobus entry for this board 
			   then check if the driver is correct for 
			   this board then check that the controller 
			   number is the same as the iobus 
			*/
                	if (strcmp(biconn, p_adpt->p_name)==0 && 
                       		(char *)udp == p_adpt->c_name &&
                       		p_adpt->c_num == um->um_ctlr) {

				/* now if it is an LYNX (8200) then we 
				   need to make sure that if the device 
				   is an DEBNK (aie with tape) we hook 
				   it up to the same AIE that NI was found 
				   at.  The variable ni_port_adpt will 
				   point to the adapter structure that the 
				   NI was configured at.  Corralate that 
			 	   number with the adapter structure we 
				   are looking at.   If they equal, we 
				   found the right structure.
			 	*/
				if (strcmp(biconn, "aie") == 0) {
#ifdef vax
					if ((cpu==VAX_8200) && ni_port_adpt) {
			        		if (ni_port_adpt->p_num == 
								p_adpt->p_num)
			    				return(um);
					} else {
						if (!is_adapt_alive(um)) 
							return(um); 
						else
							break;
					}
#endif vax
#ifdef mips
					if (!is_adapt_alive(um)) 
							return(um); 
						else
							break;
#endif mips
				} else {
		       			/* this case checks that the adapter 
					 structure has not been used. */
		 			if (p_adpt->c_ptr == 0)
                               			return(um);
				}
			}
		}


	}
	return(0);

}		

struct uba_device *bifindui(); 

bi_config_dev(nxv,nxp,binumber,binode) 
struct bi_nodespace *nxv;
char *nxp;
register int 	binumber;
register int 	binode;
{
	register struct uba_device *ui;
	register int found = 0;
	register int (**biprobe)();
	register struct bisw *pbisw = bidata[binumber].bierr[binode].pbisw;

	
	for ( biprobe = pbisw->probes; *biprobe; biprobe++) {
		
		if ((ui = bifindui(binumber,binode,biprobe)) == 0)
			if ((ui = bifindui(binumber,'?',biprobe))==0) 
				if ((ui = bifindui('?','?',biprobe))==0) 
					continue;
		found =1;
		if (((*biprobe)(nxv,nxp,binumber,binode,ui))
			== 0){
			continue;
		}

 		ui->ui_adpt = binumber;
		ui->ui_nexus = binode;
		config_fillin(ui);
		printf("\n");
		ui->ui_dk = -1;
		ui->ui_alive = 1;
		ui->ui_addr=(char *)nxv;
		ui->ui_driver->ud_dinfo[ui->ui_unit] = ui;
		(*ui->ui_driver->ud_attach)(ui);

	}
	return(found);

}

struct uba_device *bifindui(binumber,binode,biprobe) 
register int binumber;
register int binode;
register int (**biprobe)();
{
	struct uba_driver *udp;
	register struct uba_device *ui;
        register struct config_adpt *p_adpt;


	for (ui=ubdinit; udp =ui->ui_driver; ui++) {
	
		if ((udp->ud_probe != *biprobe) ||
		    (ui->ui_adpt != binumber) ||
		    (ui->ui_nexus != binode) ||
		    (ui->ui_alive) ||
		    (ui->ui_slave != -1)) continue;

		 /* check that the adapter structure that is associated
		    with this device has not been used.  It could have been
		    used in the case of a DEBNK/DEBNT. */
		 if (is_adapt_alive(ui)) continue;

		 if (strcmp(ui->ui_devname,"bvpni")==0) return(ui);

		 for(p_adpt = &config_adpt[0];p_adpt->p_name; p_adpt++) {
		 	if (((char *) ui->ui_driver == p_adpt->c_name) &&  
			    (p_adpt->c_type == 'D') &&
			    (ui->ui_unit == p_adpt->c_num) &&
			    (strcmp(p_adpt->p_name,"vaxbi")==0) &&
			    ((p_adpt->p_num == binumber) ||
			     (p_adpt->p_num == '?')))
				return(ui);
		}
	}
	return(0);

}		


#define BI_HARDERR 0x7fff0000
#define BI_THRESHOLD 2 	/* 2 sec */
int nignorebi = 0;

unsigned lastbierr = 0;

vaxbierrors(binumber,pcpsl)
int binumber;
int *pcpsl;
{
  extern int cpu; /* global cpu value from machine/common/cpuconf.c */
  int nbi_num;
  int i;
  extern int nNXMI;

        /* provide escape exit if excessive errors during autoconf */

	if (ignorebi) {
		if (nignorebi++ > 0x10000) panic("Too many VAXBI errors");
		return;
	}


	/* log_bierrors routine will scan and log any VAXBI errors.
           It will return zero if no recovery should be attempted */

	if ( log_bierrors(binumber,pcpsl) == 0 ) {

#ifdef vax

 	  /* ISIS needs a call here .... TODO */

	  /*
	   * log more info to get a complete picture of the state
           * of the machine before we panic.
           *
	   */

	  switch (cpu) {

 	    case VAX_8200: memerr();
	                   break;

	    case VAX_8800: for (nbi_num=0;nbi_num<MAX_NNBIA;nbi_num++) 
		  	        nbia_log_err(nbi_num);
 			   log_ka8800memerrs();
	                   break;

  	    case VAX_6200: log_ka6200memerrs();
 		 	   log_xmierrors(0,pcpsl); 
	                   break;

 	    case VAX_6400: rxma_check_errors(EL_PRILOW);
	                   log_xmierrors(0,pcpsl);
	                   break;

 	    case VAX_9000:    /* 9000 has no memories on busses to check. */
	                      /* Just check adapters on xmi for errors. */
	                   for(i = 0; i<nNXMI; i++){
	                     if(mfpr(CPUCNF) & (1 << (CPUCNF_XJA_PRESENT+i)))
	                       log_xmierrors(i,pcpsl);
	                   }
	                   break;

 	          default: break;

	  }

#endif vax
	  	panic("VAXBI error");
	}


}


log_bierrors(binumber,pcpsl)

int binumber;  /* VAXBI number */
int *pcpsl;    /* pointer to error pc and psl */

/*---------------------------------------------------------------*
 * function: scan VAXBI nodes for hard errors and log BI error   *
 *           info if found. If no errors found, get out.         *
 *           return zero if no recovery should be attempted      *
 *           by the calling error handler routine.               *
 *---------------------------------------------------------------*/

{
	register struct bi_regs *biregp;
	register struct bidata *bid;
	register struct el_rec *elrp;
	register struct el_bier *elbierp;
	register struct bi_nodespace *nxv;
	int node;
	int nodecnt = 0;
	int *intp;
	struct bisw *pbisw;
	int priority;
	int recover_status = 1;
	

	bid = &bidata[binumber];

	/* need to loop through BI nodes to determine if  there is
	   a hard error on a node with "BIF_SET_HEIE" enabled. It
	   is assumed that all other boards will handle the error 
	   in there own way. */

	nxv =  bid->bivirt;
	for(node=0; node < NBINODES; node++,nxv++) {

	    if (bid->binodes_alive & (1 << node)) {
	    	/*count number of active BI nodes */
		nodecnt++;
	    	pbisw = bid->bierr[node].pbisw;
		if ((pbisw->bi_flags & BIF_SET_HEIE) &&
		    (nxv->biic.biic_err & BI_HARDERR)) {
			priority = EL_PRISEVERE;
			recover_status =0;
		}
		if (  (pbisw->bi_type == BI_BUA)
		   || (pbisw->bi_type == BI_BLA)) {
			/* hard error on bua csr if an error bit is set */
			if (((struct bua_regs *)nxv)->bua_ctrl & BUACR_MASK) {
			    priority = EL_PRISEVERE;
			    recover_status =0;
			}
		}
	    }
	}

	/* if recover_status still 1, no hard errors were found. 
           Assume error was handled elsewhere and log no error. */

	if (recover_status == 1) {
	   return(recover_status); 
	}

	/* provide time-out escape for repeat offenders */

	if ((time.tv_sec - bid->bilast_err_time) < BI_THRESHOLD){
		priority = EL_PRISEVERE;
		recover_status =0;
	} else 
		priority = EL_PRIHIGH;

	bid->bilast_err_time = time.tv_sec;
	bid->bi_err_cnt++;

	/* get error log packet and fill in header. */

	elrp = ealloc(sizeof(struct bi_regs) * nodecnt + 12, priority);
	if (elrp != NULL) {
	    LSUBID(elrp,ELCT_BUS,ELBUS_BIER,EL_UNDEF,binumber,EL_UNDEF,EL_UNDEF);
	    elbierp = &elrp->el_body.elbier;
	    biregp = elbierp->biregs;
	} 

	if (recover_status == 0) cprintf("hard error VAXBI%d\n",binumber);

	nxv =  bid->bivirt;
	for(node=0; node < NBINODES; node++,nxv++) {

	    if (bid->binodes_alive & (1<<node)) { 
	    		pbisw = bid->bierr[node].pbisw;
			bid->bierr[node].bierr1 = bid->bierr[node].bierr;
			bid->bierr[node].bierr=nxv->biic.biic_err&(~BIERR_UPEN);
			if (recover_status == 0) {
				cprintf("%s at node %d error %b ",
				pbisw->bi_name, node, bid->bierr[node].bierr,
				BIERR_BITS);
			}
			/* reset error bits on proper nodes */
			if (pbisw->bi_flags & BIF_SET_HEIE)
				nxv->biic.biic_err = nxv->biic.biic_err;
			
			if (  (pbisw->bi_type == BI_BUA)
			   || (pbisw->bi_type == BI_BLA)) {
				/* log bua csr if an error bit is set */
				if (((struct bua_regs *)nxv)->bua_ctrl &
				   BUACR_MASK) {
				   	bid->bierr[node].bierr = 
						((struct bua_regs *)nxv)->bua_ctrl;
					logbigen(binumber,pcpsl,nxv);

				if (recover_status == 0) cprintf(" cr %b ",
				   (((struct bua_regs *)nxv)->bua_ctrl),
				   BUAERR_BITS);
				}
			}
		        if (recover_status == 0) cprintf("\n");

			if (elrp != NULL) {
				/* log the node status */
				biregp->bi_typ = nxv->biic.biic_typ;
				biregp->bi_ctrl = nxv->biic.biic_ctrl;
				biregp->bi_err = bid->bierr[node].bierr;
				biregp->bi_err_int = nxv->biic.biic_err_int;
				biregp->bi_int_dst = nxv->biic.biic_int_dst;
				biregp++;
			}
		}
	}

	/* finish logging error */
	if (elrp != NULL) {
		intp = (int *)biregp;
		elbierp->bier_nument = (short)nodecnt;
		*intp++ = *pcpsl++;
		*intp = *pcpsl; 
		EVALID(elrp);
	}

	/* loop back through nodes and call reset routines if preset */
	nxv =  bid->bivirt;
	for(node=0; node < NBINODES; node++,nxv++) {
		if (bid->binodes_alive & (1<<node)) { 
			pbisw= bid->bierr[node].pbisw;
			(*(pbisw->bi_reset))(binumber,node,nxv);
		}
	}

	return(recover_status);
}

logbigen(binum,pcpsl,nxv)
int binum;
int *pcpsl;
register struct bi_nodespace *nxv;
{
	register int class;
	register int type;
	register struct el_rec *elrp;
	register struct el_bigen *elbp;
	
	elrp = ealloc(sizeof(struct el_bigen), EL_PRIHIGH);
	if (elrp != NULL) {
	    elbp = &elrp->el_body.elbigen;
	    elbp->bigen_dev = nxv->biic.biic_typ;
	    elbp->bigen_bicsr = nxv->biic.biic_ctrl;
	    elbp->bigen_ber = nxv->biic.biic_err;
	    elbp->bigen_csr = ((struct bua_regs *)nxv)->bua_ctrl;
	    switch (elbp->bigen_dev & BITYP_TYPE) {
		case BI_BUA:
		    class = ELCT_ADPTR;
		    type = ELADP_BUA;
		    break;
		case BI_BLA:
		    class = ELCT_DCNTL;
		    type = ELBI_BLA;
		    break;
		default:
		    class = EL_UNDEF;
		    type = EL_UNDEF;
		    break;
	    }
	    if (type == ELADP_BUA)
	        elbp->bigen_fubar = 
			((struct bua_regs *)nxv)->bua_fubar & BUAFUBAR;
	    else
		elbp->bigen_fubar = 0;
	    elbp->bigen_pc = *pcpsl++;
	    elbp->bigen_psl = *pcpsl;
	    LSUBID(elrp,class,type,EL_UNDEF,binum,EL_UNDEF,EL_UNDEF);
	    EVALID(elrp);
	}
}

extern int nNVAXBI;

biclrint(){

int binumber,binode;
struct bisw *pbisw;
struct bi_nodespace *nxv;

    	for (binumber= 0; binumber<nNVAXBI; binumber++) {
		nxv = bidata[binumber].bivirt;
		if (bidata[binumber].binodes_alive) 
		    for(binode=0; binode < 16; binode++,nxv++){

			pbisw = bidata[binumber].bierr[binode].pbisw;

			if ((bidata[binumber].binodes_alive & 1<<binode) &&
				(pbisw->bi_flags & BIF_SET_HEIE)){
				nxv->biic.biic_ctrl &= ~(BICTRL_HEIE); 
				if ((pbisw->bi_type == BI_BUA) ||
				   (pbisw->bi_type == BI_BLA)) 
					((struct bua_regs *)nxv)->bua_ctrl &=
						~(BUACR_BUAEIE);
	
			}

		    }
	}
}
bisetint(){

int binumber,binode;
struct bisw *pbisw;
struct bi_nodespace *nxv;

    	for (binumber= 0; binumber<nNVAXBI; binumber++) {
	
		nxv = bidata[binumber].bivirt;
		if (bidata[binumber].binodes_alive) 
		    for(binode=0; binode < 16; binode++,nxv++){

			pbisw = bidata[binumber].bierr[binode].pbisw;

			if ((bidata[binumber].binodes_alive & 1<<binode) &&
				(pbisw->bi_flags & BIF_SET_HEIE)){
				nxv->biic.biic_err = nxv->biic.biic_err;
				nxv->biic.biic_ctrl |= (BICTRL_HEIE); 
				
				if ((pbisw->bi_type == BI_BUA) ||
				   (pbisw->bi_type == BI_BLA)){ 
					((struct bua_regs *)nxv)->bua_ctrl =
					   ((struct bua_regs *)nxv)->bua_ctrl;
					((struct bua_regs *)nxv)->bua_ctrl |=
						(BUACR_BUAEIE);
				}
			}

		    }
	}
}

/*
 * bidev_vec(): To set up BI device interrupt vectors.
 * It is called with 4 parameters:
 *	binum:	the BI number that the device is on
 *	binode: the BI node number of the device
 *	level:  the offset corresponding to the interrupt priority level
 *		to start at.  See ../vaxbi/bireg.h: LEVEL{14,15,16,17}.
 *	ui:	the device structure (for names of interrupt routines)
 */

bidev_vec(binum, binode, level, ui)
	int binum, binode, level;
	struct uba_device *ui;
{
	register int (**ivec)();
	register int (**addr)();	/* double indirection neccessary to keep
				   	   the C compiler happy */
	for (ivec = ui->ui_intr; *ivec; ivec++) {
		addr = (int (**)())(SCB_BI_VEC_ADDR(binum,binode,level));
		*addr = scbentry(*ivec,SCB_ISTACK);
		level += BIVECSIZE;
	}
}

bicon_vec(binum, binode, level, um)
	int binum, binode, level;
	struct uba_ctlr *um;
{
	register int (**ivec)();
	register int (**addr)();	/* double indirection neccessary to keep
				   	   the C compiler happy */
	for (ivec = um->um_intr; *ivec; ivec++) {
		addr = (int (**)())(SCB_BI_VEC_ADDR(binum,binode,level));
		*addr = scbentry(*ivec,SCB_ISTACK);
		level += BIVECSIZE;
	}
}

#ifdef mips

/*
 * bisst -- BI Start Self Test
 *
 * locore routine for vax. Done for mips in C since we don't have to deal with
 * funny BI stuff from VAX 8200s.
 */
int	ignorebi;
bisst(address)
volatile int *address;
{
	int save, retval, i;

	retval = 1;
	save = ignorebi;
	ignorebi = 1;
	*address |= BICTRL_NOARB;
	wbflush();
	DELAY (10);
	*address |= BICTRL_STS|BICTRL_SST;
	wbflush();
	DELAY (100);
	/*
	 * Pound sand for quite a while (10 seconds).
	 */
	for (i = 0; i < 100; i++) {
		DELAY(100000);
		if (!(*address & BICTRL_BROKE)) {
			retval = 0;
			break;
		}
	}
	ignorebi = save;
	return (retval);	
}

#endif mips
