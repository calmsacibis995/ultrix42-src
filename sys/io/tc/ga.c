#ifndef lint
static char *sccsid = "@(#)ga.c	4.5      (ULTRIX)  11/9/90";
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
 *
 * Modification History
 *
 *   28-Aug-90 Joe Szczypek
 *      Modified use of osconsole environment variable to comply with new
 *      DS5000 TURBOchannel.  Multiple output no longer supported by
 *      console.
 *
 *   19-Apr-90 Sam Hsu
 *	Since pte resource usage is the same whether you map 1 word or
 *	1024 words, then might as well do the latter and map entire option
 *	board, so can debug at will, thus rendering HWDEBUG irrelevant to
 *	this module.  Also saves 1 sms.  Disable watch-paging hooks.  Add
 *	some wbflush()s in sendPacket when dealing with stic ipdvint reg.
 *
 *    5-Apr-90 Paul Jensen
 *	1) Verify the #else side of HWDEBUG works.  2) Allow the console
 *	to write to the screen without hanging the server. 3) Map the
 *	ring buffer into K1.  Cacheing doesn't appear to help getimage
 *	at all.  This should be revisited if and when the cacheflush()
 *	syscall works, as it has the ability to invalidate cache entries
 *	without generating memory requests. ??? XXX
 *
 *   19-Mar-90 Sam Hsu
 *      Add code to watch paging activity (vm_hook and open).  Linkage for
 *      VDAC reset.  Merge in Paul's changes since BL11 submit.  Add
 *	_gx_modtype linkage.
 *
 *   22-Feb-90 Sam Hsu
 *	Use kernel text for packet buffers b/c of 2MB STIC addr limit.
 *	#ifdef'd on GA_ALLOC_{TX|DT|KM} for text, data, km_alloc, resp'ly.
 *
 *   12-Feb-90 Paul Jensen
 *	Use Packet Done interrupts to dispatch STIC packets.
 *
 *   05-Feb-90 Sam Hsu
 *	Poll packetdone intr bit instead of STIC dma register.
 *
 *   29-Jan-90 Sam Hsu
 *	Dynamic debug levels.  gaintr() checks for stic error.  vblank
 *	and pktdone intrs not used.
 *
 *   18-Jan-90 Sam Hsu
 *	Fix ga_cons_init() - don't PHYS_TO_K1 when looking for whether
 *	option is configured (kn02where_option()) until after test == 0.
 *	Use GA_DTYPE instead of GA_DISPLAY.
 *
 *   17-Dec-89 Sam Hsu
 *	Move ga_init_stic() to gx.c.  Ifdef'd HWDEBUG gxInfo->gxo.
 *	Update call to gx_decode_option().  Add ws_display_* sizer var's.
 *
 *   15-Dec-89 Alan Frechette
 *	Changes to "ws_display_type" and "ws_display_units".
 *
 *   15-Dec-89 Paul Jensen
 *	Merge in Sam Hsu's fixes.  Make sure stic_dma_rb returns the
 *	correct value.
 *
 *   06-Dec-89 Paul Jensen
 *	Use km_alloc() for getting contiguous memory for the ring buffer.
 *
 *   04-Dec-89 Sam Hsu
 *	Conversion to gx/ga/gq driver modules and Ultrix 4.0-8.
 *
 *   21-Sep-89 Paul Jensen
 *	This is the driver for the 3Max Accelerated 2D graphics
 *	option.  It is derived from Sam Hsu's 3D driver, pm.c
 *	Version 1.4.
 *
 * $Header: /nfs/kodak/local/4.0-12/io/tc/RCS/ga.c,v 1.27 90/05/07 11:24:53 fhsu Exp $
 ************************************************************************/
#define _GA_C_

#include "../data/gx_data.c"
#include "../data/ga_data.c"

#undef HWDEBUG

/*
 * Definition of the driver for the auto-configuration program.
 */
int	ga_probe(), ga_attach(), gaintr();
int	ga_close(), ga_open(), ga_ioctl();

struct	uba_driver gadriver = 
        { ga_probe, 0, ga_attach, 0, gxstd, "ga", gxinfo };

/*
 * Internal routines.
 */
int ga_config(), ga_init_cons(), ga_cons_init(), ga_vm_hook();
int *ga_getPacket();

static gaMap	*gao /*= GA_ADDR*/;
static int 	gx_priv_size;


/******************************************************************
 ** ga_probe():                                                  **
 ** Routine to see if the graphic device will interrupt.         **
 **                                                              **
 ******************************************************************/
ga_probe(reg)
{
    GX_DEBUG(GX_BLAB,
	     gx_puts("gaprobe()\n");
	     );

    return(1);
} /* end gaprobe() */


/******************************************************************
 ** ga_attach():                                                 **
 ** Routine to attach to the graphic device.                     **
 **                                                              **
 ******************************************************************/
ga_attach(reg, unit)
{
    GX_DEBUG(GX_GAB,
	     gx_printf("gaattach: gx_priv=0x%x,gxp=0x%x\n",gx_priv,gxp);
	     );

    /*  
     *  ga_init_cons() is now called from ga_cons_init()...
     */
    printf("ga0 ( %d plane %dx%d stamp )\n",
	   gxp->nplanes,
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

} /* end gaattach() */


/******************************************************************
 **                                                              **
 ** Routine to close the graphic device.                         **
 **                                                              **
 ******************************************************************/

/*ARGSUSED*/
ga_close(dev, flag)
    dev_t dev;
    int flag;
{
    register struct tty *tp;
    register int unit = minor(dev);

    _gx_stic->ipdvint = (_gx_stic->ipdvint | STIC_INT_P_WE) &
			     ~(STIC_INT_E_WE | STIC_INT_V_WE |
			       STIC_INT_P_EN | STIC_INT_P);

    gaVintr_count[unit] = 0;
    gaSintr_count[unit] = 0;
    gaPintr_count[unit] = 0;
    gaEintr_count[unit] = 0;

#   if 0
#   ifdef p_dev_VM_maint
    /* unexpress interest in server's vm activity... */
    if (GX_HAVESERVER)
    {
	gx_serverp->p_dev_VM_maint = 0;
	GX_DEBUG(GX_GAB,
		 gx_printf("ga_close: pid=%d\n", gx_serverp->p_pid);
		 );
    }
#   endif
#   endif 0
    return 0;
} /* end ga_close() */


/******************************************************************
 ** ga_ioctl():                                                  **
 ** 2DA-specific ioctl routine.                                  **
 **                                                              **
 ******************************************************************/
#define DWN_(X)	(((int)(X)) & ~(CLBYTES-1))
#define RND_(X)	DWN_(((int)(X)) + CLBYTES-1)

/*ARGSUSED*/
ga_ioctl(dev, cmd, data, flag)
    dev_t dev;
    register caddr_t data;
{
    int smid, unit;
    int tmp1;
    register int i;
    int poll_addr;	/* "virtual" address of base of STIC poll space */
    int rb_phys_addr;	/* phys addr of the ring buffer */
    int rb_poll_base;	/* phys address of entry mapping ring buffer */
    int rb_poll_off;	/* offset from start of poll space to rb entry */
    int serr;
    gxInfo *infop;
    gxPriv *gap;

    switch( cmd ) 
    {
     case QIOCGINFO:		     /* return screen info */

	/* check for server already done... */
	serr = GX_ERR_PRIV;
	unit = minor(dev);
	/*
	 * We can assume that gx_priv is page-aligned, since it must
	 * be physically contiguous on the 2DA.
	 */
	tmp1 = RND_( ((int)gx_priv & (CLBYTES-1)) + gx_priv_size);
	GX_DEBUG(GX_GAB,
		 gx_printf("gaioctl: gx_priv=0x%x RND=0x%x siz=%d\n",
			   gx_priv, RND_(gx_priv), tmp1);
		 );
	if ((smid = vm_system_smget(DWN_(gx_priv), tmp1, 0600)) < 0) {
	    cprintf("gaioctl: smget failed\n");
	    goto bad;
	}
	if ((tmp1 = (int)smat(smid, 0, 0)) < 0) {
	    cprintf("gaioctl: smat failed\n");
	    goto bad;
	}
	if (smctl(smid, IPC_RMID, 0))
	    goto bad;
	gap = (gxPriv *)(tmp1 | ((int)gx_priv & (CLBYTES-1)));
	infop = &gx_infos[unit].info;
	infop->qe.events = gap->events;
	infop->qe.tcs = gap->tcs;
	/*infop->curs_bits = gap->cursor;*/
	/*infop->colormap = gap->colormap;*/
	infop->ptpt_phys = 0;
	infop->ptpt_size = 0;

	/*
	 * The server uses the "physical" address to figure out
	 * which address in STIC polling space to read in order to
	 * dispatch a packet.  However, the STIC's physical addresses
	 * do not map directly onto host physical addresses.  Furthermore,
	 * we are not interested in the address of the ucode packet,
	 * but rather the embedded STIC packet within.  This is the origin
	 * of the mysterious fudge factor of 12.  The physical address
	 * points to the start of the ring buffer, but the STIC address
	 * points to the first STIC packet in the ring buffer.
	 */
	rb_phys_addr = svtophy(gx_ringbuffer);
	rb_poll_off = GX_SYS_TO_DMA(rb_phys_addr + 12);
	infop->rb_phys = GX_SYS_TO_STIC(rb_phys_addr + 12);
	/*
	 * The address we return to the user in rb_addr
	 * must be a kuseg address; however, to derive the physical
	 * address, we must use the kseg0 address.
	 */
	infop->rb_addr = (int *)gap + gx_rboffset;
	infop->rb_size = GA_RBUF_SIZE;

	GX_DEBUG(GX_YAK,
		 gx_printf("gaioctl: Prb=0x%x,Vrb=0x%x,siz=%d,phyadr=0x%x\n",
			   infop->rb_phys, infop->rb_addr, infop->rb_size,
			   rb_phys_addr);
		 );

	/*
	 * 2DA option board addresses - whole shebang
	 */
	serr = GX_ERR_GAO;
	if((smid=vm_system_smget(DWN_(gao), RND_(sizeof(gaMap)), 0600)) < 0)
	    goto bad;
	if((tmp1 = (int)smat(smid,0, 0)) < 0)
	    goto bad;
	if (smctl(smid, IPC_RMID, 0))
	    goto bad;
	tmp1 |= (int) gao & (CLBYTES-1);
	infop->gxo = (char *) tmp1;
	infop->gram = (int *)0;		/* meaningless on 2DA */
	poll_addr = (int) GA_POLL(tmp1);
	infop->stic_dma_rb = (int *)(poll_addr + rb_poll_off);

	GX_DEBUG(GX_YAK,
		 gx_printf("gaioctl: gao=0x%x,tmp1=0x%x,poll=0x%x\n",
			   gao, tmp1, infop->stic_dma_rb);
		 );
	infop->stic_reg = (int *)GA_STIC(tmp1);

#	ifdef notdef
	/*
	 * Map STIC DMA address space into kuseg.  We only map a single
	 * cluster, since we need to span only 96K of main memory
	 * (== 48 words of STIC address space);
	 */
	serr = GX_ERR_POLL;
	/* rb_poll_base = kseg address which polls start of ring buffer */
	rb_poll_base = (int)(GA_POLL(gao)) + rb_poll_off;
	if((smid=vm_system_smget(DWN_(rb_poll_base), CLBYTES, 0600)) < 0)
	    goto bad;
	if((tmp1 = (int)smat(smid,0, 0)) < 0)
	    goto bad;
	if (smctl(smid, IPC_RMID, 0))
	    goto bad;
	tmp1 |= rb_poll_base & (CLBYTES-1);
	infop->stic_dma_rb = (int *) tmp1;
	/*
	 * This is a "fake address" which subtracts the poll offset, to
	 * compensate for not mapping the start of poll space, but rather
	 * the first page which maps the ring buffer.  This allows the
	 * loop cimputing the 'srv_qpoll' addresses to move outside the
	 * ifdef.
	 */
	poll_addr = tmp1 - rb_poll_off;

	/*
	 * Map STIC control registers into kuseg
	 */
	serr = GX_ERR_STIC;
	if((smid=vm_system_smget(DWN_(GA_STIC(gao)), CLBYTES, 0600)) < 0)
	    goto bad;
	if((tmp1 = (int)smat(smid,0, 0)) < 0)
	    goto bad;
	if (smctl(smid, IPC_RMID, 0))
	    goto bad;
	tmp1 |= (int) GA_STIC(gao) & (CLBYTES-1);
	infop->stic_reg = (int *) tmp1;

#	endif notdef

	/*
	 * These addresses are used by the server when it is directly
	 * submitting packets to the STIC.
	 */
	for (i = 0; i < GA_QUEUE_PACKETS; i++) {
	    gaComArea->SRV2DCom.srv_qpoll[i] = (int *)((char *)poll_addr +
		GX_SYS_TO_DMA(&gaComArea->IntrBuf[i][3]));
	}

	*(gxInfo **)data =
	    gx_infos[unit].shmat =
		&(gap->infos[unit].info);
	break;

     bad:	
	*(gxInfo **)data = 0;
	return (serr);

     case QIOWCURSOR:		     /* old calls -> noop */
     case QIOSETCMAP:
     case QIOWCURSORCOLOR:
      {
	/* rude hack to invalidate image buffer cache entries */
	int *imagebuf = gx_ringbuffer + 0x803 - (1 << 16);
	int i;
	int sum = 0;

	/* line size is 4, so don't read every word */
	for (i=0; i<=0x800; i+=4) sum += imagebuf[i];
	*(int *)data = 0x6660000 | (sum & 0xfff);
      }
	break;

     default:			     /* not ours ??  */
	return GX_ERR_NOOP;
    }
    return GX_ERR_NONE;

} /* end gaioctl() */


ga_init_cons()
{
    gx_setup();
 
    ws_display_type = GA_DTYPE;
    ws_display_units = 1;

    return;
}/* end ga_init_cons() */


/******************************************************************
 ** gaintr()                                                     **
 ** Interrupt from STIC						 **
 **                                                              **
 ******************************************************************/
#ifdef notdef
#define SUBMIT_PACKET(i) \
	while (*pCom2d->intr_qpoll[i] == STAMP_BUSY) {}
#else
#define SUBMIT_PACKET(i) \
	status = *pCom2d->intr_qpoll[i]
#endif notdef

gaintr(unit)
    register int unit;
{
    GX_DEBUG(GX_NEVER,
	     gx_puts("gaintr()\n");
	     );

    if (_gx_stic->ipdvint & STIC_INT_P) {		/* Packet Int */
	register Com2dPtr pCom2d = (Com2dPtr) &gaComArea->SRV2DCom;
	register int cp;
	register ga_PacketPtr pPkt;
	register int status;

#ifdef notdef
	gaPintr_count[unit]++;
#endif notdef
	/* clear *only* packet done interrupt */
	_gx_stic->ipdvint = (_gx_stic->ipdvint | STIC_INT_P_WE) &
			     ~(STIC_INT_E_WE | STIC_INT_V_WE | STIC_INT_P);
	/* if were idle, dismiss this as a spurious interrupt */
	if (pCom2d->lastRead == pCom2d->lastWritten ||
	    !(pCom2d->intr_status & GA_INTR_ACTIVE)) return 1;
	/*
	 * If we are in the process of sending a packet through a cliplist,
	 * fixup packet with next cliprect and resubmit.  If the last
	 * cliprect just completed, clear the clipping status.
	 */
	if (pCom2d->intr_status & GA_INTR_CLIP) {
	    if (pCom2d->numCliprect-- > 0) {
		*pCom2d->fixCliprect = *pCom2d->pCliprect++;
		SUBMIT_PACKET(pCom2d->lastRead);
		return 1;
	    } else {
		pCom2d->intr_status &= ~GA_INTR_CLIP;
	    }
	}
loop:
	/* point past packet just completed */
	cp = pCom2d->lastRead = NEXT_BUF(pCom2d, pCom2d->lastRead);
	/* if no more packets to process, set idle status */
	if (cp == pCom2d->lastWritten) {
	    pCom2d->intr_status &= ~GA_INTR_ACTIVE;
	    return 1;
	}

	pPkt = (ga_PacketPtr) gaComArea->IntrBuf[cp];

	if (pPkt->un.un.opcode == N_PASSPACKET) {
	    int clip_idx = (int) (unsigned short)
		pPkt->un.PassPacket.cliplist_sync;
	    if (clip_idx != N_NO_CLIPLIST) {
		int nrects = gaComArea->ClipL[clip_idx].numClipRects;
		int fix_off = DECODE_CLIP_INDEX(pPkt->un.PassPacket.data);
		gaStampClipRect *pFixup =
			(gaStampClipRect *) &pPkt->un.PassPacket.data[fix_off];
		*pFixup = gaComArea->ClipL[clip_idx].clipRects[0];
		if (nrects > 1) {
		    pCom2d->pCliprect =
				&gaComArea->ClipL[clip_idx].clipRects[1];
		    pCom2d->numCliprect = nrects-1;
		    pCom2d->fixCliprect = pFixup;
		    pCom2d->intr_status |= GA_INTR_CLIP;
		}
	    }
	    SUBMIT_PACKET(cp);
	    return 1;
	} else {			/* not a pass packet, skip it */
	    goto loop;
	} /* end if */
    } else if (_gx_stic->ipdvint & STIC_INT_V) {	/* Vert Int */
	gaVintr_count[unit]++;
	_gx_stic->ipdvint = (_gx_stic->ipdvint | STIC_INT_V_WE) &
			     ~(STIC_INT_E_WE | STIC_INT_P_WE | STIC_INT_V);
    } else if (_gx_stic->ipdvint & STIC_INT_E) {	/* Error Int */
	gaEintr_count[unit]++;
	gx_init_stic();
	cprintf("STIC intr 0x%x CSR 0x%x B:CSR 0x%x addr 0x%x dat 0x%x\n",
		_gx_stic->ipdvint,
		_gx_stic->sticsr, _gx_stic->buscsr,
		_gx_stic->busadr, _gx_stic->busdat);
    } else {						/* stray ??? */
	gaSintr_count[unit]++;
    }
} /* end gaintr() */


ga_config(qp, module_type)
    register gxInfo *qp;
    int module_type;
{
    qp->gxo = (char *)gao;

    qp->stic_dma_rb = GA_POLL(gao);

    if (module_type != STIC_OPT_2DA)
	cprintf("ga_config: not 2DA, STIC modtype = %d\n", module_type);

} /* end ga_config() */


/******************************
 * ga_getPacket():
 *	We need to fix this so the console doesn't step on top of data
 *	structures used by the server. ??? XXX
 ******************************/
int
*ga_getPacket()
{
    static u_int i = 1;
    register int *buf;

    /* use nK buffers for now */
    buf = gx_ringbuffer + ((GA_CONSIZ >> 2) * i) + 3;
    i ^= 1;			/* toggle between buffers */

    return (buf);
} /* end ga_getPacket() */


/******************************
 * ga_sendPacket():
 ******************************/
int
ga_sendPacket(buf)                      /* -> # polls; 0 == timeout */
    char *buf;                          /* virtual addr */
{
    register u_long i;
    volatile u_long *poll;
    register int save_ipdvint;

    poll = (u_long *)((char *)GA_POLL(gao) + GX_SYS_TO_DMA((int)buf));

    /* disable packet-done interrupts */
    _gx_stic->ipdvint = ((save_ipdvint =_gx_stic->ipdvint) | STIC_INT_P_WE) &
		       ~(STIC_INT_E_WE | STIC_INT_V_WE | STIC_INT_P_EN);

    wbflush();                          /* make sure all writes completed */

    /*
     * wait for stic to be ready to accept next packet.  note that we never
     * wait forever.  we'll time out and go ahead and see if the stic will
     * accept a packet anyway.  if not, _then_ we complain...
     */
    for (i = 0; i < STAMP_RETRIES; i++) {
	if (_gx_stic->ipdvint & STIC_INT_P)
	    break;
	DELAY(STAMP_DELAY);
    }

    /*
     * Restore ipdvint state, clearing the STIC_INT_P bit.  If interrupts
     * were enabled before, they will be re-enabled.  gaintr() is capable
     * of dealing with spurious interrupts generated while the server is
     * running.
     */
    _gx_stic->ipdvint = (save_ipdvint | STIC_INT_P_WE) &
		       ~(STIC_INT_E_WE | STIC_INT_V_WE | STIC_INT_P);
    wbflush();

    if (*poll != STAMP_GOOD) {
	printf("STIC dead?!\n ");
	printf("CSR 0x%x Bus CSR 0x%x addr 0x%x dat 0x%x int 0x%x\n",
		_gx_stic->sticsr, _gx_stic->buscsr,
		_gx_stic->busadr, _gx_stic->busdat, _gx_stic->ipdvint);
	_gx_stic->ipdvint = save_ipdvint | STIC_INT_WE;
	wbflush();
	return(-1);
    }

    return (i);
} /* end ga_sendPacket() */


#if 0

#define GA_INTR_MASK	0xfffff000

int ga_vm_cnt[3] = { 0 };

ga_vm_hook(cmd, vpn)			/* alias p_dev_VM_maint */
    int cmd, vpn;
{
    if (GX_HAVESERVER)
    {
	int va = (int)ptob(vpn) & GA_INTR_MASK; /* now va, page aligned */

	switch (cmd)
	{
	 case PDEVCMD_ONE:
	    GX_DEBUG(GX_GAB,
		     gx_printf("ga_vm_hook(cmd=1,va=0x%x)\n", cmd, va);
		     );
	    (ga_vm_cnt[0])++;
	    break;

	 case PDEVCMD_ALL:
	    GX_DEBUG(GX_GAB,
		     gx_printf("ga_vm_hook(cmd=a,va=0x%x)\n", cmd, va);
		     );
	    (ga_vm_cnt[1])++;
	    break;

	 case PDEVCMD_TOP:
	    GX_DEBUG(GX_GAB,
		     gx_printf("ga_vm_hook(cmd=t,va=0x%x)\n", cmd, va);
		     );
	    (ga_vm_cnt[2])++;
	    break;

	 default:
	    GX_DEBUG(GX_GAB,
		     gx_printf("ga_vm_hook(cmd=? 0x%x)\n", cmd);
		     );
	    cprintf("ga_vm_hook: bad cmd 0x%x\n", cmd);
	    panic("ga_vm_hook");
	}
    }
    else
	printf("ga_vm_hook: no server?\n");

    return 0;
}
/* end ga_vm_hook. */


ga_open(dev, flag)
{
#   ifdef p_dev_VM_maint
    /* express interest in server's vm activity... */
    if (GX_IAMSERVER)
    {
	gx_serverp->p_dev_VM_maint = ga_vm_hook;
	GX_DEBUG(GX_GAB,
		 gx_printf("ga_open: pid=%d\n", gx_serverp->p_pid);
		 );
    }
#   endif
}
/* end ga_open. */

#endif 0

/******************************************************************
 ** ga_cons_init():                                              **
 **    Graphic device console initialization.  This routine gets **
 **    called before anything else, and is (among other things)  **
 **    the only safe place to allocate physically contiguous     **
 **    memory.                                                   **
 **                                                              **
 ******************************************************************/

extern int console_magic;

ga_cons_init()
{
    register int reg;
    int tmp1,tmp2;      		/* ROM debug only */
    extern int cpu;

    if (cpu == DS_3100)  return (0);
    reg = tc_where_option("ga");
    if (reg == 0) return (0);

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
	     gx_printf("ga_cons_init(reg=0x%x)\n", reg);
	     );

    gao = (gaMap *)reg;

    /* begin required linkage */
    _gx_vdac       = GA_VDAC(gao);
    _gx_vdacReset  = GA_ROM(gao);	/* write-only */
    _gx_stamp	   = reg + 0xc0000;	/* stic stamp space    @ 0x..0c0000 */
    _gx_stic	   = GA_STIC(gao);	/* stic register space @ 0x..180000 */
    _gx_modtype	   = ((_gx_stic->modcl & ~STIC_CF_CONFIG_OPTION)
		      | STIC_OPT_2DA_SH);

    _gx_config     = ga_config;
    _gx_ioctl	   = ga_ioctl;
    _gx_getPacket  = ga_getPacket;
    _gx_sendPacket = ga_sendPacket;

    /* begin optional linkage */
#   if 0
    _gx_open	   = ga_open;
#   endif 0
    _gx_close	   = ga_close;

    /*
     * The STIC DMA area for the 2D accelerator has unusual
     * alignment requirements:  The physical starting address must be a
     * 32K-aligned address.
     * Because all the stamp packets have a 3-longword microcode header,
     * the the ring buffer must start 12 bytes before that.
     */

    /*
     * This is number of bytes to km_alloc().  128Kb + (1 page) extra
     * are allocated, within which will be located a 32Kb-aligned
     * STIC packet DMA area.
     */
    gx_priv_size = sizeof(gxPriv) + _128K + NBPG;

#   ifdef GA_ALLOC_KM
    KM_ALLOC(gx_priv, gxPriv *, gx_priv_size, KM_DEVBUF, KM_NOW_CL_CO_CA);
    gx_priv = (gxPriv *) svtophy(gx_priv);
    if ((u_long)gx_priv >= (0x800000-gx_priv_size)) {
	printf("ga_cons_init: km_alloc out-of-bounds 0x%x\n", gx_priv);
	panic("ga_cons_init");
    }
    gx_priv = (gxPriv *) (PHYS_TO_K1(gx_priv));
#   endif
#   ifdef GA_ALLOC_DT
    gx_priv = (gxPriv *) (PHYS_TO_K1(Ring2da));
#   endif
#   ifdef GA_ALLOC_TX
    gx_priv = (gxPriv *) PHYS_TO_K1(ga_dummy);
#   endif

    tmp1 = (int)&gx_priv->ringbufferoffset + 4;
#   define TMPMASK ((1<<15) - 1)
    gx_priv->ringbufferoffset =
	( (tmp1 + TMPMASK & ~TMPMASK) - 12 - (int)gx_priv ) / 4;
#   undef TMPMASK  

    /* if before the beginning, adjust upwards by 8K *longwords* */
    if (gx_priv->ringbufferoffset < 0)
	gx_priv->ringbufferoffset += (1 << 13);

    {
	register int i;
	gaComArea = (ga_ComAreaPtr) gx_ringbuffer;
	GX_DEBUG(GX_YAK,
		 gx_printf("ga_cons_init: comarea=0x%x\n", gaComArea);
		 );
	for (i = 0; i < GA_QUEUE_PACKETS; i++)
	{
	    gaComArea->SRV2DCom.intr_qpoll[i] =
		(long *)((char *)GA_POLL(gao) +
			 GX_SYS_TO_DMA(&gaComArea->IntrBuf[i][3]));
	    GX_DEBUG(GX_YAK,
		     gx_printf("ga_cons_init: Q#%d qpoll=0x%x intrbuf=0x%x\n",
			       i, gaComArea->SRV2DCom.intr_qpoll[i],
			       gaComArea->IntrBuf[i]);
		     );
	}
    }

    gxp = &(gx_info);

    GX_DEBUG(GX_GAB,
	     int *pb1;
	     int *pb2;
	     gx_printf("ga_cons_init: priv=0x%x vdac=0x%x, rboff=%d\n",
		       gx_priv, _gx_vdac, gx_priv->ringbufferoffset);
	     pb1 = pb2 = ga_getPacket();
	     gx_puts("ga_cons_init: buf        stic       dma        poll\n");
	     do {
		 u_long r1;
		 u_long r2;
		 r1 = GX_SYS_TO_STIC(pb2);
		 r2 = GX_SYS_TO_DMA(pb2);
		 gx_printf("              0x%8x 0x%8x 0x%8x 0x%8x\n",
			   pb2, r1, r2, (char *)GA_POLL(gao) + r2 );
		 pb2 = ga_getPacket();
	     } while (pb1 != pb2);
	     );

    ga_init_cons();

    return (1);

} /* end ga_cons_init() */
