#ifndef lint
static char *sccsid = "@(#)gq.c	4.5      (ULTRIX)  10/9/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1990 by				*
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

/************************************************************************
 * Modification History
 *
 *   13-Sep-90 Joe Szczypek
 *	Added new TURBOchannel console ROM support.  osconsole environment
 *	variable now returns 1 slot number is serial line, else 2 slot numbers
 *	if graphics.  Use this to determine how to do setup.  Note that new
 *	ROMs do not support multiple outputs...
 *
 *   21-Jun-90 Sam Hsu
 *	Add sampling of cpu idle time, and a incr count after each sample.
 *	Used for PEX EB/RB tuning.
 *
 *   17-May-90 Sam Hsu
 *	It'd be nice if we put cmap[0] back for video_on, wouldn't it?
 *	Propogate page count for N10 pagein requests to server.  Doesn't
 *	seem like R3K reading intr_coproc using intr addr is harmful, as
 *	is the case when the N10 does this to the R3K, but we'll use the
 *	SRAM addr anyway to be clean.
 *
 *   07-May-90 Sam Hsu
 *	Catch bad addr xlation requests from N10 and let server try to
 *	pagein which should result in a segv at an informative point.
 *	Allow dynamic setting of behavior when N10 perceived dead.
 *
 *   19-Apr-90 Sam Hsu
 *	HWDEBUG no longer used in here.  PROTO_3DA controls whether a counter
 *	board is mapped to the server, and whether the N10/server is killed
 *	when the N10 doesn't respond to our interrupt.  Entire board is mapped
 *	to server, instead of just SRAM (no additional pte resources used).
 *
 *   04-Apr-90 Sam Hsu
 *	Change cprintf -> printf so it gets logged.  Enable console/N10
 *	sync.
 *
 *   28-Mar-90 Sam Hsu
 *	Change gxPriv to K0.  gq_ptpt stays K1.  Fix intr_coproc retry
 *	loop (never timed out, but might someday).  Add counts for poll
 *	STIC pint timeout, and dropped packets (poll dma -> stamp_busy).
 *	Will reset STIC and retry (once) on a dropped packet.
 *	Add _gx_modtype linkage.
 *
 *   07-Mar-90 Sam Hsu
 *	Add vdac reset linkage.  getPacket checks for active server, and
 *	either syncs with N10 or returns no buffer.
 *	
 *   26-Feb-90 Sam Hsu
 *	Add sync with N10 on console output when xcons not active.  gq_ioctl
 *	now called by gx_ioctl.  intr_coproc checks for 2 possible response
 *	values from N10 (0/1).  Sync code cannot be activated until ucode can
 *	handle intr from R3K.
 *
 *   29-Jan-90 Sam Hsu
 *	Fix vm_hook to check if device closed since N10 would be halted
 *	already.  Only really works in !HWDEBUG mode, and when server close
 *	coincides with process death.  Dynamic debug levels.  gq_close hook
 *	to halt N10 and reset process' vm_hook vector.
 *
 *   18-Jan-90 Sam Hsu
 *	Fix gq_cons_init() - don't PHYS_TO_K1 the kn02where_option()
 *	until after the test == 0!  Zero out gq_ptpt on each server
 *	open.  Add debugging output on N10 interrupts.
 *	
 *   18-Dec-89 Sam Hsu
 *	Add some "heartbeat" counters to interrupt code.  Clear
 *	interrupt before signal'ing server to service pagein.
 *	Add _gx_init <- gq_init() to halt N10 and clear interrupt.
 *	Update interrupt code to use most recent board map struct.
 *	Add gq_getPixBuff() => readspans buffer.
 *
 *   15-Dec-89 Alan Frechette
 *	Changes to "ws_display_type" and "ws_display_units".
 *
 *   17-Dec-89 Sam Hsu
 *	Move gq_init_stic() to gx.c and use linkage var _gx_stic.
 *	Use gx_decode_option() and gx_devtype() for configuration.
 *
 *   30-Nov-89 Sam Hsu
 *	Merge into 4.0 source pool.  Breakup into gx/ga/gq modules.
 *	Compiles and links, but I'd be amazed if it still runs.
 *
 *   19-Oct-89 Sam Hsu
 *	Allow non-exclusive open of the graphics device.  Only
 *	the 1st process will have get the events/tcs mapped into
 *	its address space.  This is DEBUGGING code and could be
 *	removed before SDC.  It will allow the DMA simulator to
 *	access GRAM while the server has /dev/mouse open...
 *
 *   21-Sep-89 Sam Hsu
 *	Modifications for KMAX 3D.  Graphics console supported.
 *	Extraneous PMAX code removed.  No server transport yet.
 *	Consistent(?) naming: pm_<> internal, pm<> external.
 *
 *   21-Aug-89 Sam Hsu
 *	Modifications for KMAX w/ 3D graphics board, keyboard, mouse,
 *	but no bitmap console and something in SLU 3 port for console
 *	output.  Look for "KMAX3D" to find changes...
 *
 *   $Header: /nfs/kodak/local/4.0-12/io/tc/RCS/gq.c,v 1.19 90/05/08 20:41:47 fhsu Exp Locker: fhsu $
 *
 *   The dark ages: see gx.c
 *
 ************************************************************************/
#define _GQ_C_

#include "../data/gx_data.c"
#include "../io/tc/gq.h"

/*
 * Definition of the driver for the auto-configuration program.
 */
int	gq_probe(), gq_attach(), gqintr();
int	gq_ioctl();

struct	uba_driver gqdriver = 
        { gq_probe, 0, gq_attach, 0, gxstd, "gq", gxinfo };

/*
 * Internal routines.
 */
int  gq_config(), gq_init(), gq_init_cons(), gq_cons_init();
int *gq_getPacket(), gq_sendPacket();
int  gq_intr_noop(), gq_intr_pagein(), gq_intr_xlate(), gq_intr_vblank(),
     gq_intr_vrfy(), gq_vm_hook(), gq_intr_coproc(), gq_intr_stic();
void gq_cpu_idleSample();

/*
 * These are really for internal use, but declared global so nkvar can
 * access and change them.
 */
gqMap	       *gqo;

int	       *gq_ptpt;
int		gq_N1OK = 0;		/* whether N10 supposed to be alive */
					/* also controls whether cpu_idle   */
					/* is being sampled		    */
int		gq_debug = 0;		/* flags controlling exceptional    */
					/* behavior 			    */
#define		GQ_KILL_N10	0x1

#ifndef GX_NODEBUG
int		gq_intrPending  = 0;
int		gq_invalALL     = 0;
int		gq_invalOK      = 0;
int		gq_thrash	= 0;
#endif

/*
 * wait for stic to be ready to accept next packet.  note that we never
 * wait forever.  we'll time out and go ahead and see if the stic will
 * accept a packet anyway.  if not, _then_ we consider complaining...
 */
#define _POLL_STIC_PINT(I)					\
    for ((I) = 0; (I) < STAMP_RETRIES; (I)++) {			\
	if (_gx_stic->ipdvint & STIC_INT_P)			\
	    break;						\
	DELAY(STAMP_DELAY);					\
    }								\
    if ((I) == STAMP_RETRIES) gx_pint_timeout++;



/******************************************************************
 **                                                              **
 ** Routine to return a one(1).				         **
 **                                                              **
 ******************************************************************/
gq_probe(reg)
{
    GX_DEBUG(GX_BLAB,
	     gx_puts("gqprobe()\n");
	     );
    return(1);
}
/* end gq_probe. */


/******************************************************************
 **                                                              **
 ** Routine to attach to the graphic device.                     **
 **                                                              **
 ******************************************************************/
gq_attach(reg)
{
/*  Moved to gq_cons_init() where it occurs much earlier.
    gq_init_cons(reg);
 */
    printf("gq0 ( %d+%d+%dZ+%dX plane %dx%d stamp )\n",
	   gxp->nplanes,
	   gxp->dplanes,
	   gxp->zplanes,
	   gxp->zzplanes,
	   gxp->stamp_width,
	   gxp->stamp_height);
    /*
     * init the "latest mouse report" structure
     */
    gx_last_rep.state = 0;
    gx_last_rep.dx = 0;
    gx_last_rep.dy = 0;
    gx_last_rep.bytcnt = 0;

    gx_keyboard.hold = 0;	/* "Hold Screen" key is pressed if 1 */

    if(!gx_inkbdreset)		/* init the keyboard */
	gx_kbdreset();

    return 0;
}
/* end gq_attach. */


/******************************************************************
 **                                                              **
 ** Graphic device ioctl routine.                                **
 **                                                              **
 ******************************************************************/

/*ARGSUSED*/
gq_ioctl(dev, cmd, data, flag)
    dev_t dev;
    register caddr_t data;
{
    int unit;
    int smid;
    int tmp1;
    int errcode;
    gxInfo *infop;
    gxPriv *gqp;
    
    switch( cmd ) 
    {
     case QIOCGINFO:		     /* return screen info */

	/* check for server already done... */
	errcode = GX_ERR_PRIV;
	unit = minor(dev);

	if ((smid = vm_system_smget(DWN_(gx_priv),
				    GQ_PRIV_PGBYTES,
				    0600)) < 0)
	    goto bad;

	if ((tmp1 = (int)smat(smid, 0, 0)) < 0)
	    goto bad;

	if (smctl(smid, IPC_RMID, 0))
	    goto bad;

	gqp = (gxPriv *)(tmp1 | ((int)gx_priv & (CLBYTES-1)));
	infop = &gx_infos[unit].info;
	infop->qe.events = gqp->events;
	infop->qe.tcs = gqp->tcs;
	/*infop->curs_bits = gqp->cursor;*/
	/*infop->colormap = gqp->colormap;*/
	infop->rb_addr = (int *)gqp + gx_rboffset;
	infop->rb_phys = svtophy(gx_ringbuffer);
	infop->rb_size = GQ_RBUF_SIZE;
	infop->ptpt_phys = svtophy(gq_ptpt);
	infop->ptpt_size = GQ_PTPT_ENTRIES;

	/*
	 * 3D option board -- the whole thing
	 */
	errcode = GX_ERR_GQO;
	if((smid=vm_system_smget(DWN_(gqo),
				 RND_(sizeof(gqMap)),
				 0600)) < 0)
	    goto bad;
	if((tmp1 = (int)smat(smid,0, 0)) < 0)
	    goto bad;
	if (smctl(smid, IPC_RMID, 0))
	    goto bad;
	tmp1 |= (int) gqo & (CLBYTES-1);
	infop->gxo = (char *) tmp1;
	infop->gram = (int *) GQ_RAM(tmp1);

#       ifdef PROTO_3DA
	/*
	 * unID'd board in slot 2 of low/mid-3D is a smorg-as-board counter
	 * board, so map its 2 registers in the unused 2D fields
	 */
	if (infop->stamp_height == 1)
	{
	    extern unsigned gx_slots;
	    u_long slot_addr;

	    GX_DEBUG(GX_GAB,
	    	     gx_printf("gq_ioctl: looking for smorg-as-board...\n");
		     );

	    if ((gx_slots & (1<<2)) == 0) {
	        GX_DEBUG(GX_GAB,
			 gx_printf("gq_ioctl: slot 2 empty\n");
			 );
		goto NoTimer;
	    }

	    slot_addr = PHYS_TO_K1(tc_slotaddr[2]);

	    if ((smid = vm_system_smget(DWN_(slot_addr),
					RND_(0x4),
					0600)) < 0)
	    {
		printf("gq_ioctl: failed to map counter = %d\n",
			u.u_error);
	    	GX_DEBUG(GX_GAB,
			 gx_printf("gq_ioctl: 0x%x 0x%x 0600\n",
			 	   DWN_(slot_addr), RND_(0x80004));
			 );
		goto NoTimer;
	    }
	    if ((tmp1 = (int) smat(smid, 0, 0)) < 0) {
	    	GX_DEBUG(GX_GAB,
			 gx_printf("gq_ioctl: failed to attach slot 2 = %d\n",
			 	   u.u_error);
			 );
		goto NoTimer;
	    }
	    if (smctl(smid, IPC_RMID, 0)) {
	    	GX_DEBUG(GX_GAB,
			 gx_printf("gq_ioctl: failed to ctl slot 2\n");
			 );
		goto NoTimer;
	    }

	    infop->stic_dma_rb = (int *) tmp1;
	    infop->stic_reg = (int *) tmp1;

	    GX_DEBUG(GX_GAB,
	    	     gx_printf("gq_ioctl: smorg-as-board mapped 0x%x\n",
		               tmp1);
		    );

	 NoTimer:
	    ;
	}
#	endif proto_3da

#	ifdef notdef
	errcode = GX_ERR_SRAM;
	if((smid =
	    vm_system_smget(DWN_(GQ_RAM(gqo)),
			    RND_(sizeof(gqRAM)),
			    0600)) < 0)
	    goto bad;
	if((tmp1 = (int)smat(smid,0, 0)) < 0)
	    goto bad;
	if (smctl(smid, IPC_RMID, 0))
	    goto bad;
	tmp1 |= (int) GQ_RAM(gqo) & (CLBYTES-1);
	infop->gram = (int *) tmp1;
#	endif notdef

	*(gxInfo **)data
	    = gx_infos[unit].shmat
		= &(gqp->infos[unit].info);
	break;

     bad:	
	*(gxInfo **)data = 0;
	return (errcode);

     case QIO_N10RESET:
#       ifdef p_dev_VM_maint
	if (GX_IAMSERVER)
	{
	    gx_serverp->p_dev_VM_maint = 0;
	    GX_DEBUG(GX_YAK,
		     gx_printf("gq_ioctl: i860 stop %d\n", gx_serverp->p_pid);
		     );
	}
#       endif
	*GQ_RESET(gqo) = gq_N1OK = 0;
	wbflush();
	break;

     case QIO_N10START:
	bzero(gq_ptpt, GQ_PTPT_SIZE);
	*GQ_START(gqo) = gq_N1OK = 1;
	wbflush();
	timeout(gq_cpu_idleSample, (char *)0, GQ_CPU_IDLESAMPLE);
#       ifdef p_dev_VM_maint
	if (GX_IAMSERVER)
	{
	    /* express interest in server's vm activity... */
	    gx_serverp->p_dev_VM_maint = gq_vm_hook;
	    GX_DEBUG(GX_YAK,
		     gx_printf("gq_ioctl: i860 go %d\n", gx_serverp->p_pid);
		     );
	}
#       endif
	break;

     case QIO_N10INTR:
	GX_DEBUG(GX_CONSOLE,
		 tmp1 = gq_intr_coproc(*(int *)data);
		 /*gx_printf("gq_ioctl: n10intr 0x%x -> 0x%x\n",
			   *(int *)data, tmp1);*/
		 *(int *)data = tmp1;
		 );
	break;

     default:				/* not ours ??  */
	return GX_ERR_NOOP;
    }
    return GX_ERR_NONE;
}
/* end gqioctl. */


gq_init_cons()
{
    GX_DEBUG(GX_BLAB,
	     gx_puts("gq_init_cons()\n");
	     );

    ws_display_type = GQ_DTYPE;
    ws_display_units = 1;

    return gx_setup();
}
/* end gq_init_cons. */


gq_init()
{
    GX_DEBUG(GX_BLAB,
	     gx_puts("gq_init()\n");
	     );

    *GQ_RESET(gqo) = gq_N1OK = 0;	/* halt the N10 */
    *GQ_INTRH(gqo) = GQ_INTR_ACK;	/* clear intr word */
    GX_DEBUG(GX_SILENT, gq_invalALL = gq_invalOK = 0; );
    wbflush();

#   ifdef p_dev_VM_maint
    if (GX_HAVESERVER) {
	gx_serverp->p_dev_VM_maint = 0;
    }
#   endif

    return 0;
}
/* end gq_init. */


int (*gq_intr_vec[])() = {
    gq_intr_noop,			/* 0 */
    gq_intr_pagein,			/* 1 */
    gq_intr_xlate,			/* 2 */
    gq_intr_vblank,			/* 3 */
    gq_intr_vrfy,			/* 4 */
    gq_intr_noop,			/* 5 */
    gq_intr_noop,			/* 6 */
    gq_intr_noop,			/* 7 */
    gq_intr_noop,			/* 8 */
    gq_intr_noop,			/* 9 */
    gq_intr_noop,			/* 10 */
    gq_intr_noop,			/* 11 */
    gq_intr_noop,			/* 12 */
    gq_intr_noop,			/* 13 */
    gq_intr_noop,			/* 14 */
    gq_intr_noop,			/* 15 */
};

int gq_intr_send[] = { 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0 };
int gq_intr_recv[] = { 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0 };


gq_intr_stic()
{
    if (_gx_stic->ipdvint & STIC_INT_E) {
	gx_init_stic();
	printf("gq_intr_stic: 0x%x CSR 0x%x B:CSR 0x%x addr 0x%x dat 0x%x\n",
		_gx_stic->ipdvint,
		_gx_stic->sticsr, _gx_stic->buscsr,
		_gx_stic->busadr, _gx_stic->busdat);
    }
    return 0;
}
/* end gq_intr_stic. */


gq_intr_noop(va)
    int va;
{
    *GQ_INTRH(gqo) = GQ_INTR_ACK;
    wbflush();

    GX_DEBUG(GX_GAB,
	     gx_printf("gq_intr_noop: 0x%x (%d)\n", va, gq_intr_recv[0]);
	     );

    return gq_intr_stic();
}
/* end gq_intr_noop. */


static
_gq_intr_pagein(va)
    int va;
{
    *GQ_INTRH(gqo) = va;		/* clear the R3K interrupt line */
    wbflush();

    gx_info.ptpt_pgin = (int *)(va & (HST_INTR_MASK|HST_INTR_PMSK));

    /* need to do a pagein */
    GX_DEBUG(GX_TERSE,
	     static int lastVA = 0;
	     if (lastVA == (va & HST_INTR_MASK)) {
	         gx_putchar('\007');
		 gq_thrash++;
	     }
	     else
	         lastVA = (va & HST_INTR_MASK);
	     );
    GX_DEBUG(GX_TALK,
    	     gx_printf("PI(0x%x)\n", va & HST_INTR_MASK);
	     );

    psignal(gx_serverp, SIGUSR1);

    return 0;
}
/* end _gq_intr_pagein. */


gq_intr_pagein(va)
    int va;
{
    register int tmpVA;
    register struct pte *pte;

    tmpVA = (va & HST_INTR_MASK);

#   ifndef notdef
    /*
     * Although some test runs (heavy imaging) shows this to be true, and
     * in some cases very true, it shouldn't be.  There must be a race
     * condition we're not handling somewhere... I do know of 1 created by
     * the pre-pagein code.
     */
    pte = vtopte(gx_serverp, btop(tmpVA));

    if (pte && pte->pg_v) {
	*GQ_INTRH(gqo) = 0;		/* clr intr */
	wbflush();			/* n10 will retry */
	if ((va & HST_INTR_DRTY) == 0)
	    pte->pg_m = 1;		/* mark dirty */
	return 0;
    }
#   endif

    return _gq_intr_pagein(va);
}
/* end gq_intr_pagein. */


gq_intr_xlate(va)
    int va;
{
    register int tmpVA, vsn;
    register struct pte *pte;

    tmpVA = (va & HST_INTR_MASK);
    vsn = vtovsn(tmpVA);		/* virt seg num */

    GX_DEBUG(GX_BLAB,
	     gx_printf("xlate: vpn 0x%x vsn 0x%x\n", btop(tmpVA), vsn);
	     );

    pte = vtopte(gx_serverp, btop(tmpVA));

    GX_DEBUG(GX_GAB,
	     gx_printf("xlate: sp 0x%x (0x%x+0x%x) pte 0x%x\n",
		       gx_serverp,
		       gx_serverp->p_datastart,
		       gx_serverp->p_dsize,
		       pte);
	     );

    if (pte == 0) {
	/*GQ_INTRH(gqo) = va;		/* clear intr line */
	/*wbflush();*/
	printf("gq_intr_xlate: bad VA 0x%x\n", tmpVA);
	/* let server segv while handling signal */
	return _gq_intr_pagein(va);
    }

    gq_ptpt[vsn] = (int) svtophy((int)pte & ~(NBPG-1)) | vsn;

    GX_DEBUG(GX_GAB,
	     gx_printf("gq_intr_xlate(va=0x%x) -> ptpt[0x%x]=0x%x\n",
		       tmpVA, vsn, gq_ptpt[vsn]);
	     );

    if (pte->pg_v)
    {
	/* page is already valid */
	*GQ_INTRH(gqo) = (pte->pg_pfnum << GQ_INTR_SHFT);
	wbflush();
	return 0;
    }
    else
    {
	/* request pagein */
	return _gq_intr_pagein(va);
    }
}
/* end gq_intr_xlate. */


gq_intr_vrfy(va)
    int va;
{
    register int tmpVA;
    register struct pte *pte;

    tmpVA = (va & HST_INTR_MASK);

    pte = vtopte(gx_serverp, btop(tmpVA));

    if (pte == 0) {
	/*GQ_INTRH(gqo) = va;		/* clear intr line */
	/*wbflush();*/
	printf("gq_intr_vrfy: bad VA 0x%x\n", tmpVA);
	/* let server segv while handling signal */
	return _gq_intr_pagein(va);
    }

    if (pte->pg_v)
    {
	/* page is already valid */
	*GQ_INTRH(gqo) = (pte->pg_pfnum << GQ_INTR_SHFT);
	wbflush();
	return 0;
    }
    else
    {
	*GQ_INTRH(gqo) = GQ_INTR_ACK;	/* N10 will choke on this */
	wbflush();
	GX_DEBUG(GX_CONSOLE,
		 gq_thrash++;
		 gx_printf("gq_intr_vrfy: bad pte 0x%x (VA 0x%x)\n",
			   pte, tmpVA);
		 );
	gx_serverp->p_dev_VM_maint = 0;
	psignal(gx_serverp, SIGSEGV); /* not valid - SEGV the server */
    }
}
/* end gq_intr_vrfy. */


/******************************************************************
 **                                                              **
 ** Graphics device end-of-frame interrupt service routine.      **
 **                                                              **
 ******************************************************************
 *
 * For the HE3D 3MAX board, N10 receives VINT.  VINT is normally off.
 * When an N10 packet with the sync bit arrives at the head of the
 * packet queue, N10 enables VINT and interrupts R3 when STIC interrupts
 * N10 for VINT.  We load the colormap during this time.  Unused.
 */
gq_intr_vblank(va)
    int va;
{
    *GQ_INTRH(gqo) = GQ_INTR_ACK;
    wbflush();

    GX_DEBUG(GX_BLAB,
	     gx_puts("gq_intr_vblank()\n");
	     );

    gx_load_colormap();
    gxp->qe.timestamp_ms = TOY;

    return 0;
}
/* end gq_intr_vblank. */


int gq_intr_total = 0;

gqintr(unit)
    register int unit;
{
    gq_intr_total++;

    if (GX_HAVESERVER)
    {
	int intr_cause = GQ_RAM(gqo)->intr_host;

	/* XXX byte-0 assumed. */
	(gq_intr_recv[intr_cause & HST_INTR_WHAT])++;

	return (*gq_intr_vec[intr_cause & HST_INTR_WHAT])(intr_cause);
    }
    else
    {
	/* could also be a STIC error intr... */
	return gq_intr_stic();
    }
}
/* end gqintr. */


gq_intr_coproc(cmd)
    int cmd;
{
    register int i, ans;
    volatile int *intr_coproc = GQ_INTRC(gqo);
    volatile int *read_coproc = &(GQ_RAM(gqo)->intr_coproc);
    int rval = 4;
    int timo = gq_N1OK ? GQ_INTR_TIMO : (GQ_INTR_TIMO >> 2);

    GX_DEBUG(GX_SILENT,
	     if (gq_intrPending) {
		 /* nested interrupts invalid */
		 printf("gq_intr_coproc: new 0x%x (old 0x%x)\n",
			cmd, *read_coproc);
		 panic("gq_intr_coproc: nested\n");
	     }
	     gq_intrPending = 1;
	     );

    GX_DEBUG(GX_NEVER,
	     gx_printf("gq_intr_coproc(cmd=0x%x)\n", cmd);
	     );

    *intr_coproc = cmd;
    wbflush();

    gq_intr_send[(cmd & GQ_INTR_WHAT) >> GQ_INTR_WSHF]++;

    for ( i = timo; i > 0; i-- )
    {
	DELAY(2);			/* 150 cycles for N10 intr roundtrip */
	switch (ans = *read_coproc)	/* N10 running at 25-30ns clock */
	{
	 case 0xdeadbabe:
	 case 0x00000000:
	    GX_DEBUG(GX_SILENT, gq_intrPending = 0;);
	    return -1;
	 case GQ_INTR_BUF0:		/* 1 */
	    GX_DEBUG(GX_SILENT, gq_intrPending = 0;);
	    return 0;
	 case GQ_INTR_BUF1:		/* 2 */
	    GX_DEBUG(GX_SILENT, gq_intrPending = 0;);
	    return 1;
	 default:
	    if (ans != cmd) {
		GX_DEBUG(GX_CONSOLE,
			 gx_printf("gq_intr_coproc: 0x%x bad repl=0x%x\n",
				   cmd, ans);
			 );
		rval = 2;		/* > 1 */
		goto BadReply;
	    }
	}
    }
    GX_DEBUG(GX_CONSOLE,
	     gx_printf("gq_intr_coproc: 0x%x dead=0x%x spl %d hst 0x%x\n",
		       cmd, ans, whatspl(), GQ_RAM(gqo)->intr_host);
	     );
 BadReply:

    if (gq_debug & GQ_KILL_N10) {
	if (GX_HAVESERVER)
#	    ifdef p_dev_VM_maint
	    gx_serverp->p_dev_VM_maint = 0;
#	    endif
	    psignal(gx_serverp,SIGILL); /* kill the server */
	*GQ_RESET(gqo) = 0;		/* kill the N10 */
	wbflush();
    }
    else
    {
# 	ifdef p_dev_VM_maint
	if (GX_HAVESERVER)
	    gx_serverp->p_dev_VM_maint = 0;
#	endif
    }
    GX_DEBUG(GX_SILENT, gq_intrPending = 0;);

    gq_N1OK = 0;

    return rval;
}
/* end gq_intr_coproc. */


gq_vm_hook(cmd, vpn)			/* alias p_dev_VM_maint */
    int cmd, vpn;
{
    int vsn;

    if (GX_HAVESERVER)
    {
	int reply;

	vpn = (int)ptob(vpn) & GQ_INTR_MASK; /* now va, page aligned */
	GX_DEBUG(GX_SILENT, gq_invalALL++;);

	switch (cmd)
	{
	 case PDEVCMD_ONE:
	    GX_DEBUG(GX_NEVER,
		     gx_printf("gq_vm_hook(cmd=1,vpn=0x%x)\n", vpn);
		     );
	    reply = gq_intr_coproc(vpn | GQ_INTR_INV1);
	    GX_DEBUG(GX_CONSOLE,
		     char *msg;
		     int *pTLB;
		     switch (reply) {
		      case 2: msg = "REP"; break;
		      case 3: msg = "STO"; break;
		      case 4: msg = "LTO"; break;
		      default:msg = "OK";
		     }
		     pTLB = (int *)((int)GQ_RAM(gqo) + N10_VTLB_OFF);
		     if (pTLB[GQ_VTLB_INDEX(vpn)] == 0 ||
			 (pTLB[GQ_VTLB_INDEX(vpn)] & 0xfffff) == 0xbabed) {
			 gq_invalOK++;
		     } else {
			 gx_printf("gq_vm_hook: 0x%x[0x%x] -> 0x%x %s 0x%x\n",
				   vpn, GQ_VTLB_INDEX(vpn),
				   pTLB[GQ_VTLB_INDEX(vpn)],
				   msg, GQ_RAM(gqo)->intr_coproc);
		     }
		     );
	    break;

	 case PDEVCMD_ALL:
	    GX_DEBUG(GX_NEVER,
		     gx_printf("gq_vm_hook(cmd=a,vpn=0x%x)\n", vpn);
		     );
	    bzero(gq_ptpt, GQ_PTPT_SIZE);
	    if (gq_intr_coproc(GQ_INTR_INVA) >= 0)
		printf("gq_vm_hook: INVA\n");
	    break;

	 case PDEVCMD_TOP:
	    vsn = vtovsn(vpn);
	    GX_DEBUG(GX_NEVER,
		     gx_printf("gq_vm_hook(cmd=t,vpn=0x%x:vsn=0x%x)\n",
			       vpn, vsn);
		     );
	    /* I'm assuming that we dont get called here (brk(2) with *
	     * change < 0) except when a page table page is freed.    */
	    bzero(gq_ptpt + vsn, GQ_PTPT_SIZE - vsn);
	    if (gq_intr_coproc(vpn | GQ_INTR_CEIL) >= 0)
		printf("gq_vm_hook: CEIL\n");
	    break;

	 default:
	    GX_DEBUG(GX_GAB,
		     gx_printf("gq_vm_hook(cmd=bad 0x%x)\n", cmd);
		     );
	    printf("gq_vm_hook: bad cmd 0x%x\n", cmd);
	    panic("gq_vm_hook");
	}
    }
    else
	printf("gq_vm_hook: no server?\n");

    return 0;
}
/* end gq_vm_hook. */


void
gq_cpu_idleSample(arg)
    char *arg;
{
    if (gq_N1OK) {
	register struct cpudata *pcpu=CURRENT_CPUDATA;
	gx_info.host_idle = pcpu->cpu_cptime[CP_IDLE];
	gx_info.host_idleCount++;
	timeout(gq_cpu_idleSample, (char *)0, GQ_CPU_IDLESAMPLE);
    }
}
/* end gq_cpu_idleSample. */


gq_close(dev, flag)
{
#   ifdef p_dev_VM_maint
    /* unexpress interest in server's vm activity... */
    if (GX_HAVESERVER)
    {
	gx_serverp->p_dev_VM_maint = 0;
	*GQ_RESET(gqo) = gq_N1OK = 0;	/* halt N10 */
	wbflush();
	GX_DEBUG(GX_GAB,
		 gx_printf("gq_close: pid=%d\n", gx_serverp->p_pid);
		 );
    }
#   endif

    return 0;
}
/* end gq_close. */


gq_config(qp, module_type)
    register gxInfo *qp;
    int module_type;
{
    qp->gxo = (char *)gqo;

    qp->gram = (int *)GQ_RAM(gqo);

    if (module_type == STIC_OPT_2DA) {
	printf("gq_config: not 3DA, STIC modtype = %d\n", module_type);
	panic("gq_config");
    }

    return 0;
}
/* end gq_config. */


gq_getPixBuff(p_sva, p_stic)
    int **p_sva;			/* sys virt addr */
    int *p_stic;			/* stic phys addr */
{
    u_long sram_addr;

    *p_sva = GQ_RAM(gqo)->pixbuf;	/* K1 addr */
    sram_addr = GQ_SYS_TO_PHYS(*p_sva);
    *p_stic = GX_SYS_TO_STIC(sram_addr);
}


int *
gq_getPacket()
{
    static int whichBuffer = 0;		/* 0 || 1 */
    register int *buf;

    /*
     * don't collide with N10 over SRAM when xcons not enabled!
     * should not be the common case.
     */
    if (gq_N1OK) {
	register int i;			/* ask N10 which packet buffer */
					/* we may use. */
	whichBuffer = gq_intr_coproc(GQ_INTR_PAUS);
	if (whichBuffer < 0 || whichBuffer > 1) {
	    GX_DEBUG(GX_TERSE,
		     gx_printf("gq_getPacket: bad reply from i860 %d\n",
			       whichBuffer);
		     );
	    return ((int *)(whichBuffer = 0));
	}
	GX_DEBUG(GX_GAB,
		 gx_printf("gq_getPacket: i860->%d\n", whichBuffer);
		 );
	_POLL_STIC_PINT(i);		/* then wait for stic to be idle */
    }					/* since this buffer may be currently */
					/* executing */
    buf = GQ_REQBUF(gqo, whichBuffer);
    whichBuffer ^= 0x1;

    return (buf);
}
/* end gq_getPacket. */


gq_sendPacket(buf)
    char *buf;				/* virtual addr */
{
    int i, addr;
    volatile int *poll;

    /* ... to sram phys addr ... */
    addr = GQ_SYS_TO_PHYS(buf);

    poll = (int *)((char *)GQ_POLL(gqo) + GX_SYS_TO_DMA(addr));

    _POLL_STIC_PINT(i);
    _gx_stic->ipdvint = STIC_INT_P_WE;	/* clear pkt done intr bit */

    wbflush();				/* make sure all writes completed */

    if (*poll != STAMP_GOOD)
    {
	gx_init_stic();
	gx_unwedge_stic++;
	if (*poll != STAMP_GOOD) {
	    gx_dropped_packet++;
	    GX_DEBUG(GX_TERSE,
		     gx_printf("STIC:CSR 0x%x B:SR 0x%x A 0x%x D 0x%x I 0x%x\n",
			       _gx_stic->sticsr, _gx_stic->buscsr,
			       _gx_stic->busadr, _gx_stic->busdat,
			       _gx_stic->ipdvint);
		     );
	    return -1;
	}
    }
					/* if N10 running, then must have */
    if (gq_N1OK) {			/* asked permission to do console */
	register int k;			/* output, so N10 must be waiting */
	_POLL_STIC_PINT(k);		/* for OK to proceed... */
	GQ_RAM(gqo)->intr_coproc = 0;
	wbflush();
    }
    return (i);
}
/* end gq_sendPacket. */


/*
 * THE BEGINNING.
 */
gq_cons_init()
{
    register int reg;
    int tmp1;
    extern int cpu, console_magic;

    if (cpu == DS_3100) return (0);
    reg = tc_where_option("gq");
    if (reg == 0)
        return (0);

    reg = PHYS_TO_K1(reg);

    /*
     * 3max console ROM changes for enhanced TURBOchannel support
     * have eliminated the ability to have multiple outputs.  If
     * new ROM is in place, use the output device specifiec by ROM.
     */

    if (console_magic != 0x30464354) {
      tmp1 = atoi(prom_getenv("osconsole"));
      if (tmp1 & 0x1) gx_console |= GRAPHIC_DEV;
      if (tmp1 & 0x8) gx_console |= CONS_DEV;
    }
    else {
      tmp1 = rex_getenv("osconsole");
      if (strlen(tmp1) > 1) {
	if (tmp1 & 0x1) gx_console = GRAPHIC_DEV;
      }
      else {
	if (tmp1 & 0x8) gx_console = CONS_DEV;
      }
    }

    GX_DEBUG(GX_GAB,
	     gx_printf("gq_cons_init: reg=0x%x\n", reg);
	     );

    gqo = (gqMap *)reg;			/* K1 physaddr: base of option board */

    /*
     * KM_ALLOC storage area for gxPriv struct.  This has to be done
     * here, since the console code assumes gx_priv exists already!
     */
    KM_ALLOC(gx_priv, gxPriv *, GQ_PRIV_SIZE, KM_DEVBUF, KM_CLEAR|KM_CONTIG);
    if (!gx_priv) return 0;
    gx_priv = (gxPriv *)PHYS_TO_K0(svtophy(gx_priv));
    /*
     * ditto for the ((page table page) table) ...
     */
    KM_ALLOC(gq_ptpt, int *, GQ_PTPT_SIZE, KM_DEVBUF, KM_CLEAR|KM_CONTIG);
    if (!gq_ptpt) return 0;
    gq_ptpt = (int *)PHYS_TO_K1(svtophy(gq_ptpt));

    gxp = &(gx_info);

    gx_rboffset = sizeof(gxPriv)/sizeof(int);

    /*
     * Required linkage...
     */
    _gx_vdac       = GQ_VDAC(gqo);
    _gx_vdacReset  = GQ_VDACRESET(gqo);
    _gx_stamp	   = GQ_STAMP(gqo);	/* stic stamp space    @ 0x..0c0000 */
    _gx_stic       = GQ_STIC(gqo);	/* stic register space @ 0x..180000 */
    _gx_modtype	   = ((_gx_stic->modcl & ~STIC_CF_CONFIG_OPTION)
		      | STIC_OPT_3DA_SH);

    _gx_config     = gq_config;
    _gx_ioctl	   = gq_ioctl;
    _gx_getPacket  = gq_getPacket;
    _gx_sendPacket = gq_sendPacket;
    _gx_getPixBuff = gq_getPixBuff;

    /*
     * Optional linkage...
     */
    _gx_init       = gq_init;
    _gx_close	   = gq_close;

    gq_init_cons();

    return (1);
}
/* end gq_cons_init. */
