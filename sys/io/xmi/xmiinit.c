#ifndef lint
static char *sccsid = "@(#)xmiinit.c	4.7    ULTRIX  12/6/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1988,1990 by			*
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
 * Revision History
 *
 * 03-Dec-90	Joe Szczypek
 *	Added return() to get_xmi() to silence LINT.
 *
 * 29-Aug-90	stuarth (Stuart Hollander)
 *	Fixed bug in xmisst when determining which xmi a node is on.
 *	Added comments explaining xmiconf structure.
 *
 * 17-Aug-90	rafiey (Ali Rafieymehr)
 *	Made the following changes for Stuart Hollander.
 *	Split xmiconf() into xmiconf_reset() and xmiconf_conf().
 *	Allows, per xmi bus, to reset all nodes, then wait as they
 *	all perform self-test in parallel, instead of resetting and
 *	waiting for each device, one-by-one.
 *	Also, handles multiple xmi s.
 *
 * 03-Aug-90	rafiey (Ali Rafieymehr)
 *	Changed xmi_io_space for VAX9000. Defined numxmi for multiple
 *	XMI support.
 *
 * 02-May-90    Joe Szczypek
 *      Modified error routines for xbi+ support.
 *
 * 13-Mar-90	rafiey (Ali Rafieymehr)
 *	Removed unnecessary check for invalid slot of xmi node from xmi_io_space().
 *
 * 11-Dec-89    Paul Grist
 *      Modified xmierrors to call panic using status returned from
 *      xbi_check_errs(). Added new routine log_xmi_bierrors() to 
 *      to log pending VAXBI errors and log_xmierrors to just log pending
 *      xbi errors. They will be used by exception handlers looking for
 *      more error information.
 *
 * 08-Dec-89 	Pete Keilty
 *	Modified nxaccess() in xmiconf() routine to use XMINODE_SIZE
 *	as node space size define in xmireg.h for VAX.
 *
 * 08-Dec-89 	jaw
 *	make printf use decimal for printing node and bus numbers.  
 *
 * 13-Nov-89	burns
 *	Made xmi_io_space consistent over vax/mips platforms. Trickery
 *      now performed in nxaccess().
 *
 * 09-Nov-89	jaw
 *	fix bug where hooking xna to wrong bus.
 *
 * 20-Jul-89	rafiey (Ali Rafieymehr)
 *	Added support for XMI devices.
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
 *************************************************************************/
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
#include "../h/kmalloc.h"
#include "../h/vmmac.h"

#include "../machine/cpu.h"
#ifdef vax
#include "../machine/mtpr.h"
#endif vax
#include "../machine/nexus.h"
#include "../machine/scb.h"

#include "../io/xmi/xmireg.h"
#include "../io/uba/ubavar.h"

#ifdef vax
#define XMI_START_PHYS 0x21800000
#endif vax
#ifdef mips
#define XMI_START_PHYS 0x11800000
#endif mips
extern int dkn;         /* number of iostat dk numbers assigned so far */
extern int nNUBA;
#ifdef vax
extern int catcher[256];
#endif vax
#ifdef mips
extern int	stray();
#endif mips
extern int nNXMI;
extern int vecbi;
extern int cpu;			/* Ultrix internal System type */

extern struct bus_dispatch xmierr_dispatch[];
extern struct cpusw *cpup;	/* pointer to cpusw entry */
extern struct config_adpt  *ni_port_adpt;
extern struct bidata bidata[];

int	numxmi;

xmisetvec(xminumber)
int xminumber;
{
	int i;
	struct xmidata *xmidata;
	xmidata = get_xmi(xminumber);
	for(i=0; xmierr_dispatch[i].bus_num != xminumber ; i++) {
		if (xmierr_dispatch[i].bus_num == -1) panic("no vector");

	}
	
	*(xmidata->xmivec_page+(XMIEINT_XMIVEC/4))=
				scbentry(xmierr_dispatch[i].bus_vec,SCB_ISTACK);

}

struct xmidata *get_xmi(xminumber) 
int xminumber;
{
	register struct xmidata *xmidata;
	
	xmidata = head_xmidata;

	while(xmidata) {
		if(xmidata->xminum == xminumber)
			return(xmidata);
		xmidata = xmidata->next;
	}

	panic("no bus data");
	/*NOTREACHED*/
}

extern struct xmi_reg xmi_start[];

	/* Instead of being simply one large function,
	   xmiconf is structured as three subfunctions so that
	   one could easily write a function to initialize multiple
	   xmis in parallel by calling xmiconf_reset for each xmi,
	   then calling xmiconf_wait for each, and then calling
	   xmiconf_conf for each.

	   We keep xmiconf as a function that initializes
	   only one xmi so that existing code can remain unchanged.
 	*/
xmiconf(xminumber)
int xminumber;
{
	int s;

    	s = spl5();
	xmiconf_reset(xminumber); /* Reset all nodes on the xmi */
	xmiconf_wait(xminumber); /* Wait for all initializations to complete */
	splx(s);
	xmiconf_conf(xminumber); /* Configure each node */
}

xmiconf_reset(xminumber)
int xminumber;
{
	struct xmi_reg	*nxv;	/* virtual pointer to XMI node */
	struct xmi_reg	*nxp;	/* physical pointer to XMI node */
	register int xminode;
	register struct xmisw *pxmisw;
	register struct xmidata *xmidata;
	register struct xmi_reg *cpunode=0;
	register int i;

	xmidata = get_xmi(xminumber);

	/* set xmi alive in adpter struct */
	config_set_alive("xmi",xminumber);

	nxp = xmidata->xmiphys;
	
	/* see if we need to allocate pte's */
	/*
	 * Set up initial virtual address for xmi node space.
	 * This is a bit of a misnomer. On Vaxes XMI node space is mapped
	 * and thus accesses via real virtual addresses. On mips XMI node
	 * is accessible vis KSEG0 and KSEG1, so we use a KSEG1 "virtual"
	 * address which really is a direct translation of the physical.
	 */
#ifdef vax
	nxv = xmi_start;
	nxv += (xminumber * 16);
#endif vax
#ifdef mips
	nxv = (struct xmi_reg *)PHYS_TO_K1(XMI_START_PHYS);
#endif mips
	xmidata->xmivirt = nxv;

	printf("xmi %d at address 0x%x\n",xminumber,nxp);
	xmidata->xminodes_alive =0;

	/* figure out cpu node from xmi interrupt dst. reg */
	xmidata->cpu_xmi_addr = nxv + (ffs(xmidata->xmiintr_dst) -1);

		/* If on first page of scb, do not change the first 64
		 * vectors, as these are the standard arch defined vectors.
		 * On other pages, initialize all vectors.
		 */
	if (xmidata->xmivec_page == &scb.scb_stray) i=64;
	else i=0;

		/* initialize SCB to catcher */
	for ( ; i < 128; i++)
		*(SCB_XMI_ADDR(xmidata) + i) =
#ifdef vax
		    scbentry(&catcher[i*2], SCB_ISTACK);
#endif vax
#ifdef mips
		    scbentry(stray, 0);
#endif mips
	/* set error interrupt vector */
	xmisetvec(xminumber);

	for(xminode = 0; xminode < MAX_XMI_NODE; xminode++,nxv++) {

	    /* get physical address to map */
	    nxp = (struct xmi_reg *) cpup->nexaddr(xminumber,xminode);

#ifdef vax
	    /* map xmi node space */
	    nxaccess(nxp,&Sysmap[btop((int)(nxv) & ~VA_SYS)],XMINODE_SIZE);
#endif vax
#ifdef mips
	    /* 
	     * XMI node space is not mapped on mips.
	     */
#endif mips

	    /* xmi node alive ??? */
	    if (BADADDR((caddr_t) nxv,sizeof(long))) continue;

	    for (pxmisw = xmisw ; pxmisw->xmi_type ; pxmisw++) {	
		if (pxmisw->xmi_type == (short)(nxv->xmi_dtype)) {

	    	    xmidata->xmierr[xminode].pxmisw= pxmisw;
		    xmidata->xminodes_alive |= (1 << xminode);
                    if (pxmisw->xmi_flags&XMIF_SST) {
			nxv->xmi_xbe = (nxv->xmi_xbe & ~XMI_XBAD) | XMI_NRST;
                    }
                    break;
		}
	    }
	    if (pxmisw->xmi_type ==0)
		printf ("xmi %d node %d, unsupported device type 0x%x\n",
			xminumber,xminode, (unsigned short) nxv->xmi_dtype);
	}
	DELAY(10000);	/* need to give time for XMI bad line to be set */
}


	/* Wait for reset of xmi nodes to take effect. */

xmiconf_wait(xminumber)
int xminumber;
{
	struct xmi_reg	*nxv;	/* virtual pointer to XMI node */
	struct xmi_reg	*nxp;	/* physical pointer to XMI node */
	register int xminode;
	register struct xmisw *pxmisw;
	register struct xmidata *xmidata;
	register struct xmi_reg *cpunode=0;
	register int i;
	int broke;
	int alive;
	int totaldelay;

	/* Wait cumulative up to 20 seconds.
	   For extra safety, this is double the spec value. */
	totaldelay = 2000;
	xmidata = get_xmi(xminumber);
	nxv = xmidata->xmivirt;
	for(xminode = 0; xminode < MAX_XMI_NODE; xminode++,nxv++) {
	    if( !(xmidata->xminodes_alive & (1 << xminode)))
		continue;
	    pxmisw = xmidata->xmierr[xminode].pxmisw;
		/* wait here for up to remaining count time
		   or until device is reset.  */
            if (pxmisw->xmi_flags&XMIF_SST) {
	    	cpunode = xmidata->cpu_xmi_addr;
		while((cpunode->xmi_xbe & XMI_XBAD) && (totaldelay-- > 0))
			DELAY(10000);
		while((nxv->xmi_xbe&(XMI_ETF|XMI_STF)) && (totaldelay-- > 0))
			DELAY(10000);
		nxv->xmi_xbe = nxv->xmi_xbe & ~(XMI_XBAD | XMI_NRST);
	    }
	}

}

	/* Do config of nodes. */

xmiconf_conf(xminumber)
int xminumber;
{
	struct xmi_reg	*nxv;	/* virtual pointer to XMI node */
	struct xmi_reg	*nxp;	/* physical pointer to XMI node */
	register int xminode;
	register struct xmisw *pxmisw;
	register struct xmidata *xmidata;
	register struct xmi_reg *cpunode=0;
	register int i;
	int broke;
	int alive;
	int totaldelay;

	xmidata = get_xmi(xminumber);

	/* do config of devices, adapters, controllers, etc. */
	nxv = xmidata->xmivirt;
	for(xminode = 0; xminode < MAX_XMI_NODE; xminode++,nxv++) {
	    if( !(xmidata->xminodes_alive & (1 << xminode)))
		continue;
	    pxmisw = xmidata->xmierr[xminode].pxmisw;
	    nxp = (struct xmi_reg *) cpup->nexaddr(xminumber,xminode);

	    broke = (nxv->xmi_xbe & XMI_ETF) ||
			(nxv->xmi_xbe & XMI_STF);
	    alive = 0;
	    if (pxmisw->xmi_flags&XMIF_DEVICE)
		alive |= xmi_config_dev(nxv,nxp,xminumber,xminode,xmidata);
	    if (pxmisw->xmi_flags&XMIF_CONTROLLER);
		alive |= xmi_config_con(nxv,nxp,xminumber,xminode,xmidata);
	    if (pxmisw->xmi_flags&XMIF_ADAPTER){
	    	(**pxmisw->probes)(nxv,nxp,xminumber,xminode,xmidata);
		alive = 1;
	    }
            if (pxmisw->xmi_flags&XMIF_NOCONF) {
		printf ("%s at xmi%d node %d",
                        pxmisw->xmi_name,xminumber,xminode);
                if (broke == 0)
                        printf("\n");
                else
                        printf(" is broken, continuing!\n");
	 	(**pxmisw->probes)(nxv,nxp,xminumber,xminode,xmidata);
                alive=1;
            }
            if (alive==0) xminotconf(nxv,nxp,xminumber,xminode,xmidata);
	}


	/* The following loop cleans up any errors that might
	   have gotten set when we probed the XMI */

	nxv = xmidata->xmivirt;
	for(xminode = 0; xminode < MAX_XMI_NODE; xminode++,nxv++) {
		if (xmidata->xminodes_alive & (1<<xminode)) {
			nxv->xmi_xbe = nxv->xmi_xbe & ~XMI_XBAD;
		}	

	}
}


struct uba_ctlr *xmifindum();

xmi_config_con(nxv,nxp,xminumber,xminode,xmidata)
struct	xmi_reg *nxv;
char	*nxp;
register int    xminumber;
register int    xminode;
register struct xmidata *xmidata;
{

	register struct uba_device *ui;
	register struct uba_ctlr *um;
	register int (**xmiprobe)();
	register struct uba_driver *udp;
	register struct xmisw *pxmisw = xmidata->xmierr[xminode].pxmisw;
	int level;
	int (**ivec)();
	int found = 0;

	
	for ( xmiprobe =  pxmisw->probes; *xmiprobe; xmiprobe++) {
		if ((um = xmifindum(xminumber,xminode,xmiprobe,pxmisw->xmi_name)) == 0)
			if ((um = xmifindum(xminumber,'?',xmiprobe,pxmisw->xmi_name))==0) 
				if ((um = xmifindum('?','?',xmiprobe,pxmisw->xmi_name))==0) 

					continue;
		found =1;
                if (((*xmiprobe)(nxv,nxp,xminumber,xminode,um))
			== 0){
			continue;
		}
		um->um_adpt = xminumber;
		um->um_nexus = xminode;
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
		xmicon_vec(xminumber, xminode, level, um);
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
				ui->ui_adpt = xminumber;
				ui->ui_nexus = xminode;

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




struct uba_ctlr *xmifindum(xminumber,xminode,xmiprobe,xmiconn)
register int xminumber;
register int xminode;
register int (**xmiprobe)();
register char *xmiconn;
{
	struct uba_driver *udp;
	register struct uba_ctlr *um;
        register struct config_adpt *p_adpt;
        struct config_adpt *p_aie;
        extern struct config_adpt config_adpt[];



	for (um=ubminit; udp =um->um_driver; um++) {
		/* first check that the drivers probe routine equals the 
		 * xmi's probe routine then 
		 * crosscheck xminumber with um's xmi number then
		 * crosscheck xminode number with um's node number then
		 * make sure not alive already then
		 * make sure its not a valid unibus number.
		 */
	
		if ((udp->ud_probe != *xmiprobe) ||
		    (um->um_adpt != xminumber) ||
		    (um->um_nexus != xminode) ||
		    (um->um_alive) ||
		    ((um->um_ubanum >=0) && ((um->um_ubanum < nNUBA)
		        		 || (um->um_ubanum == '?'))))continue;

                for(p_adpt = &config_adpt[0];p_adpt->p_name; p_adpt++) {

			/* first look for iobus entry for this board 
			   then check if the driver is correct for 
			   this board then check that the controller 
			   number is the same as the iobus 
			*/
                	if (strcmp(xmiconn, p_adpt->p_name)==0 && 
                       		(char *)udp == p_adpt->c_name &&
                       		p_adpt->c_num == um->um_ctlr) {

		       		/* this case checks that the adapter 
				 structure has not been used. */
		 		if (p_adpt->c_ptr == 0)
                               		return(um);
			}
		}

	}
	return(0);
}		


struct uba_device *xmifindui();

xmi_config_dev(nxv,nxp,xminumber,xminode,xmidata)
struct	xmi_reg *nxv;
char *nxp;
register int    xminumber;
register int    xminode;
register struct xmidata *xmidata;
{

	register struct uba_device *ui;
	register int found = 0;
	register int (**xmiprobe)();
	register struct xmisw *pxmisw = xmidata->xmierr[xminode].pxmisw;

	
	for ( xmiprobe = pxmisw->probes; *xmiprobe; xmiprobe++) {
		
		if ((ui = xmifindui(xminumber,xminode,xmiprobe)) == 0)
			if ((ui = xmifindui(xminumber,'?',xmiprobe))==0) 
				if ((ui = xmifindui('?','?',xmiprobe))==0) 
					continue;
		found =1;
		if (((*xmiprobe)(nxv,nxp,xminumber,xminode,ui))
			== 0){
			continue;
		}

 		ui->ui_adpt = xminumber;
		ui->ui_nexus = xminode;
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



struct uba_device *xmifindui(xminumber,xminode,xmiprobe) 
register int xminumber;
register int xminode;
register int (**xmiprobe)();
{
	struct uba_driver *udp;
	register struct uba_device *ui;
        register struct config_adpt *p_adpt;


	for (ui=ubdinit; udp =ui->ui_driver; ui++) {
	
		if ((udp->ud_probe != *xmiprobe) ||
		    (ui->ui_adpt != xminumber) ||
		    (ui->ui_nexus != xminode) ||
		    (ui->ui_alive) ||
		    (ui->ui_slave != -1)) continue;

		 /* check that the adapter structure that is associated
		    with this device has not been used.  It could have been
		    used in the case of a DEBNK/DEBNT. */
		 if (is_adapt_alive(ui)) continue;

		 /* next we verify that this "ui" entry is hooked to an
		    XMI and not a BI in the case of the xna */
		 for(p_adpt = &config_adpt[0];p_adpt->p_name; p_adpt++) {
		 	if (((char *) ui->ui_driver == p_adpt->c_name) &&  
			    (p_adpt->c_type == 'D') &&
			    (ui->ui_unit == p_adpt->c_num) &&
			    (strcmp(p_adpt->p_name,"xmi")==0) &&
			    ((p_adpt->p_num == xminumber) ||
			     (p_adpt->p_num == '?')))
				return(ui);
		}


	}
	return(0);

}		



/* TODO: This routine should use xminumber .  Ali */
xmi_io_space(xminumber,xminode) 
 	int xminode,xminumber;
{

	if(cpu == VAX_9000)
		return(((int)0x22000000) + (0x02000000 * vecbi)); 
	else
		return(((int)0x20000000) + (0x02000000 * xminode)); 
}


/* 
	xmi errors -- currently the only device that will interrupt
	here are the XBI and XBI+.

*/

xmierrors(xminumber,pcpsl)
int xminumber;
int *pcpsl;
{
	int xminode;

	struct xmi_reg	*nxv;	/* virtual pointer to XMI node */
	register struct xmidata *xmidata;

	xmidata = get_xmi(xminumber); 
			
	nxv = xmidata->xmivirt;
	for(xminode = 0; xminode < MAX_XMI_NODE; xminode++,nxv++) {
		if (xmidata->xminodes_alive & (1<<xminode)) {
		   if ((short) (nxv->xmi_dtype) == XMI_XBI ||
		       (short) (nxv->xmi_dtype) == XMI_XBIPLUS) {
		     if ( (xbi_check_errs(xminumber,xminode,nxv)==0))
		           panic("xbi error");
		   }

		}	

	}
}


log_xmi_bierrors(xminumber,pcpsl)
int xminumber;
int *pcpsl;

/*
 *
 * function: scan xmi and log any pending vaxbi errors found.      
 *
 */

{
	int xminode;
	register struct bidata *bid;
	register int binumber;


	struct xmi_reg	*nxv;	/* virtual pointer to XMI node */
	register struct xmidata *xmidata;

	xmidata = get_xmi(xminumber); 
			
	nxv = xmidata->xmivirt;
	for(xminode = 0; xminode < MAX_XMI_NODE; xminode++,nxv++) {
		if (xmidata->xminodes_alive & (1<<xminode)) {
			if ((short) (nxv->xmi_dtype) == XMI_XBI ||
			    (short) (nxv->xmi_dtype) == XMI_XBIPLUS) {
  	                   binumber = xminode + (xminumber<<4);
    	                   bid = &bidata[binumber];
 	                   if (bid->binodes_alive){
 	                     log_bierrors(binumber,pcpsl);
                           }
			}
		}	

	}

}


log_xmierrors(xminumber,pcpsl)
int xminumber;
int *pcpsl;

/*
 *
 * function: scan xmi and log any pending errors found.      
 *
 */

{
	int xminode;

	struct xmi_reg	*nxv;	/* virtual pointer to XMI node */
	register struct xmidata *xmidata;

	xmidata = get_xmi(xminumber); 
			
	nxv = xmidata->xmivirt;
	for(xminode = 0; xminode < MAX_XMI_NODE; xminode++,nxv++) {
		if (xmidata->xminodes_alive & (1<<xminode)) {
			if ((short) (nxv->xmi_dtype) == XMI_XBI ||
			    (short) (nxv->xmi_dtype) == XMI_XBIPLUS) {
			  xbi_check_errs(xminumber,xminode,nxv);
			}
		}	

	}
}


/*
 * xmidev_vec(): To set up XMI device interrupt vectors.
 * It is called with 4 parameters:
 *	xminum:	the XMI number that the device is on
 *	xminode: the XMI node number of the device
 *	level:  the offset corresponding to the interrupt priority level
 *		to start at.  See xmireg.h: LEVEL{14,15,16,17}.
 *	ui:	the device structure (for names of interrupt routines)
 */

xmidev_vec(xminum, xminode, level, ui)
	int xminum, xminode, level;
	struct uba_device *ui;
{
	register int (**ivec)();
	register int (**addr)();	/* double indirection neccessary to keep
				   	   the C compiler happy */
	register struct xmidata *xmidata;

	xmidata = get_xmi(xminum); 
	for (ivec = ui->ui_intr; *ivec; ivec++) {
		addr = (int (**)())(SCB_XMI_VEC_ADDR(xmidata, xminum,xminode,level));
		*addr = scbentry(*ivec,SCB_ISTACK);
		level += XMIVECSIZE;
	}
}

xmisst(nxv)
struct	xmi_reg *nxv;
{
	int totaldelay;
	int ret = 0;
	int s;
	register struct xmidata *xmidata;
	register struct xmi_reg *cpunode=0;

	s= spl5();
	nxv->xmi_xbe = (nxv->xmi_xbe & ~XMI_XBAD) | XMI_NRST;

	/* need to give time for XMI bad line to be set */
	DELAY(10000);
	
	xmidata = head_xmidata;

	while(xmidata) {
		if ((xmidata->xminum != -1) &&
		    (nxv >= xmidata->xmivirt) && 
		    (nxv < (xmidata->xmivirt +16)))
		    	cpunode = xmidata->cpu_xmi_addr;
		xmidata = xmidata->next;
	}

	if (!cpunode) panic("invalid xmi address");
	
	/* wait for XMI_XBAD line to be deasserted.  or 10 seconds.*/
	totaldelay = 1000;
	while((cpunode->xmi_xbe & XMI_XBAD) && (totaldelay-- > 0)) {
		DELAY(10000);
	}
/*
 * Wait for the self tests to finish (10 seconds)
 */
	totaldelay = 1000;
	while (((nxv->xmi_xbe & XMI_ETF) || (nxv->xmi_xbe & XMI_STF))
	     &&(totaldelay > 0)) {
		--totaldelay;
		DELAY(10000);
	}
	nxv->xmi_xbe = (nxv->xmi_xbe & ~(XMI_XBAD | XMI_NRST));
	if (totaldelay > 0)
		ret = 1;

	splx(s);
	return (ret);
}
