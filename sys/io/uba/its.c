
#ifndef lint
static	char	*sccsid = "@(#)its.c	4.1  (ULTRIX)        7/2/90";
#endif	lint

/************************************************************************
 *									*
 *			Copyright (c) 1987 by				*
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

/*
 * its.c
 *
 * Modification history
 *
 * ITEX/FG101 Series 100 frame buffer driver
 *
 *  2-Dec-86 - lp/rsp (Larry Palmer/Ricky Palmer)
 *
 *	Created prototype frame buffer driver.
 */

/*
 *	This  driver  provides user level programs with the ability to
 *	read  and  write  any  of  the	frame  buffer registers.  This
 *	capability  is	provided  via  the  ITSMAP  ioctl  request and
 *	subsequent  user  direct  access  to  the  registers and frame
 *	memory.   The  driver  initializes  the  scanner hardware in a
 *	default  state.   The same initialization is possible by using
 *	the  registers	directly.  Note that DMA is NOT possible since
 *	the board does not support local DMA transfers to another QBUS
 *	device.   The  documentation  provided	by  Imaging Technology
 *	should	be  consulted  for further details involving the frame
 *	buffer	hardware/software.   This  driver  is  provided  as  a
 *	service  to  Digital  Ultrix  customers  who  would  like  the
 *	capability to combine the frame buffer hardware with their GPX
 *	system	in  order  to  create  digital	images from a standard
 *	television  camera hookup.  The driver is provided "as-is" and
 *	is  NOT  supported  under any Digital agreements or contracts.
 *	Electronic   mail   concerning	the  driver  may  be  sent  to
 *	decvax!rsp  or	decvax!lp  but	there  is  no guarantee of any
 *	response or support.
 *
 *	This driver assumes the following configuration:
 *
 *		FG101 at 173000
 *		Memory Map @ 0x30200000
 *
 *	The availble ioctl requests for this driver are:
 *
 *	(1) ITSMAP - maps the frame buffer into user space.
 *	(2) ITSGRABIT - starts the frame buffer grabbing frames (30/sec).
 *	(3) ITSFREEZE - freezes a frame.
 *	(4) ITSZOOM - initiates a zoom;subsequent calls cycle through.
 *
 */

#include "its.h"
#if NITS > 0 || defined(BINARY)

#include "../data/its_data.c"
int itsprobe();
int itsattach();
int itsintr();
unsigned short itsstd[] = { ITSSCSR , 0 };
unsigned short *itsmem;
struct uba_driver itsdriver =
		{
		itsprobe,		/* device probe entry		*/
		0,			/* no slave device		*/
		itsattach,		/* device attach entry		*/
		0,			/* no "fill csr/ba to start"	*/
		itsstd, 		/* device addresses		*/
		"its",			/* device name string		*/
		itsdinfo		/* ptr to its uba_device struct */
		};

/************************************************************************/
/*	probe frame buffer						*/
/************************************************************************/
itsprobe(reg, ctlr)
	caddr_t reg;
	int ctlr;
{
	register struct itsdevice *itsaddr = (struct itsdevice *)reg;

	br = 0x15;
	cvec = (uba_hd[numuba].uh_lastiv -= 4);

	/* Set up memory map */
	itsmem = (unsigned short *)(((char *)qmem)+ITSMEM);
	itsaddr->ac = ITSMAPEN|MAP|MAP_BYTE;
	return (sizeof (struct itsdevice));
}

/************************************************************************/
/*	itsattach for its frame buffer					*/
/************************************************************************/
itsattach(ui)
	struct uba_device *ui;
{
}

/************************************************************************/
/*	ioctls for its frame buffer					*/
/************************************************************************/
itsioctl(dev, cmd, data, flag)
	dev_t dev;
	int cmd;
	register caddr_t data;
	int flag;
{
	register struct its_softc *itp = &its_softc[minor(dev)];
	register struct uba_device *ui = itsdinfo[minor(dev)];
	register struct itsdevice *itsaddr = (struct itsdevice *)ui->ui_addr;
	int s,i=0, *zoom;
	struct	itsxypos *itxypos;
	struct memory_map *mmap;
	itxypos = (struct itsxypos *)data;
	mmap = (struct memory_map *)data;
	/*
	 * Check for and process its specific ioctl's
	 */
	switch( cmd )
		{
		case ITSGRABIT:
			s = spl5();
			/* start up frame grabbing */
			itp->it_state |= ITS_GRABBING;
			itsaddr->zoom &= ~REGMUX;
			itsaddr->csr |= ITSGRAB;
			itsaddr->zoom |= REGMUX;
			splx(s);
			break;
		case ITSFREEZE:
			s = spl5();
			itp->it_state |= ITS_FROZEN;
			itsaddr->zoom &= ~REGMUX;
			itsaddr->csr &= 0xc0ff;
			while(itsaddr->csr &0x3000)
				;
			itsaddr->csr |= ITSSNAP;
			while(itsaddr->csr & 0x3000)
				;
			itsaddr->zoom |= REGMUX;
			splx(s);
			break;
		case ITSPS:
			uprintf("Pan/scroll not currently implemented\n");
			break;

		case ITSMAP: /* Makes mmap->bytes of frame memory available to looser */
			s = spl5();
			maptouser((char *)itsaddr);
			for(i=0; i<btoc(mmap->bytes)+1; i++) {
				maptouser((char *)qmem+(i<<9)+ITSMEM);
			}
			mmap->buf = (char *)qmem+ITSMEM;
			mmap->reg = (char *)itsaddr;
			mtpr(TBIA,0);
			splx(s);
#ifdef notdef
		{
		register unsigned v;
		register int npf,o;
		struct pte *mpte;
		struct pte *pte = svtopte(((char *)qmem+ITSMEM));
		struct proc *rp;

		v = btop(mmap->buf);
		o = (int)mmap->buf & PGOFSET;
		npf = btoc(mmap->bytes + o) + 1;
		rp = u.u_procp;
		mpte = vtopte(rp, v);

		uprintf("remapping %x to %x (%x %x)\n", mpte, pte, *(int *)mpte, *(int *)pte);
		while (--npf >= 0) {
			*(int *)mpte = *(int *)pte;
		uprintf("remapping %x to %x (%x %x)\n", mpte, pte, *(int *)mpte, *(int *)pte);
			mpte++; pte++;
		}
		mtpr(TBIA, 0);
		}
#endif
			break;
		case ITSZOOM:
			zoom = (int *)data;
			itsaddr->zoom &= ~ZFACT;
			itsaddr->zoom |= *zoom&ZFACT;
			uprintf("zoom %x %x\n", *zoom, itsaddr->zoom);
			break;

		default:	/* unknown */
			break;
	}
	return (0);
}

/************************************************************************/
/*	itsreset reset frame buffer					*/
/************************************************************************/
itsreset(uban)
	register int uban;
{
	register int i;
	register struct uba_device *ui;
	register struct its_softc *itp = its_softc;

	for (i = 0; i < numNITS; i++, itp++) {
		if ((ui = itsdinfo[i]) == 0 || ui->ui_alive == 0 ||
		    ui->ui_ubanum != uban || itp->it_open == 0)
			continue;
	}
}

/************************************************************************/
/*	itsopen open frame buffer					*/
/************************************************************************/
itsopen(dev, flag)
	register dev_t dev;
	int flag;
{
	register struct its_softc *itsp;
	register struct uba_device *ui;

	if (ITSUNIT(dev) >= numNITS || (itsp = &its_softc[minor(dev)]) == (struct its_softc *)0 ||
	    (ui = itsdinfo[ITSUNIT(dev)]) == 0 || ui->ui_alive == 0)
		return (ENXIO);

	if(itsp->it_open == 0) {
		itsp->it_state = 0;
		itsp->it_cx = itsp->it_cy = 0;
		itsinit(minor(dev));
	}
	itsp->it_open++;
}

/************************************************************************/
/*	itsclose close frame buffer					*/
/************************************************************************/
itsclose(dev,flag)
	register dev_t dev;
	int flag;
{
	its_softc[minor(dev)].it_open = 0;
	its_softc[minor(dev)].it_state = 0;
}

/************************************************************************/
/*	itstrategy strategy routine for frame buffer			*/
/************************************************************************/
itsstrategy(bp)
	register struct buf *bp;
{
	register int i;
	register unsigned char *itsbuf;
	register struct uba_device *ui = itsdinfo[minor(bp->b_dev)];
	register struct itsdevice *itsaddr = (struct itsdevice *)ui->ui_addr;
	register struct its_softc *itsp = &its_softc[minor(bp->b_dev)];

	if(bp->b_flags & B_READ) {
		cprintf("read request %x\n", bp, itsmem);
		return;
	} else {
bad:
		bp->b_flags |= B_ERROR;
		iodone(bp);
		return;
	}
}

/************************************************************************/
/*	itsread read frame buffer					*/
/************************************************************************/
itsread(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	register struct uba_device *ui = itsdinfo[minor(dev)];
	register struct itsdevice *itsaddr = (struct itsdevice *)ui->ui_addr;
	register struct its_softc *itsp = &its_softc[minor(dev)];
	int ret;

	if(!(itsp->it_state & ITS_FROZEN)) {
		return(EIO);
	}
	return(physio(itsstrategy, &ritsbuf[minor(dev)], dev, B_READ,
	       minphys, uio));
}

/************************************************************************/
/*	itsintr interrupt routine for frame buffer			*/
/************************************************************************/
itsintr()
{
cprintf("itsintr\n");
}

/************************************************************************/
/*	itsinit initialization routine for frame buffer 		*/
/************************************************************************/
#define NGL 64
itsinit(unit)
{	/* check for existance and initialize if there*/
	register int i;
	register struct uba_device *ui = itsdinfo[unit];
	register struct itsdevice *itsaddr = (struct itsdevice *)ui->ui_addr;
	register short *itsm;
	itsaddr->lutcsr = 0xffff;	/* Reset */
	itsaddr->zoom &= ~REGMUX; /* Force to reg mode */
	/* Load all the LUTs */
	itsaddr->lutcsr = 0x2000;
	/* Use crystal & Super-Z mode access (8 bits/pixel) */
	itsaddr->csr = (CAMERA0|PLLOOP|SZMODE);
	itsaddr->ac = ITSMAPEN|MAP|MAP_BYTE;
	for(i=0,itsm = (short *)((char *)itsmem + LUTRED);i<=255;i++)	 /* this should load all   */
		{
		if(i>= 48 && i< 48+NGL)
		*itsm++ = (i-48)*(256/NGL);
		else
		*itsm++ = 0xff;
		}
	for(i=0,itsm = (short *)((char *)itsmem + LUTGRN);i<=255;i++)	 /* this should load all   */
		{
		if(i>= 48 && i<48+NGL)
		*itsm++ = (i-48)*(256/NGL);
		else
		*itsm++ = 0xff;
		}
	for(i=0,itsm = (short *)((char *)itsmem + LUTBLU);i<=255;i++)	 /* this should load all   */
		{
		if(i>= 48 && i<48+NGL)
		*itsm++ = (i-48)*(256/NGL);
		else
		*itsm++ = 0xff;
		}
	for(i=0, itsm = (short *)((char *)itsmem + LUTINP); i<=255; i++) {
		*itsm++ = (i/(256/NGL)) + 48;
	}
	itsaddr->lutcsr |= LUTSEL; /* Select Frame memory*/
	itsaddr->mac = MAC_PBENL|MAC_PBENH;
	itsaddr->csr |= 0x0000;
	itsaddr->pana = 0x0000;
	itsaddr->hmask = 0x0000;
	itsaddr->vam = 0x0000;
	itsaddr->xptr = 0x0000;
	itsaddr->yptr = 0x0000;
	itsaddr->xspin = 0x0010;
	itsaddr->yspin = 0x0000;
	itsaddr->scrolla = 0x0000;
	itsaddr->zoom &= ~ZFACT; /* No zoom */
	itsaddr->zoom |= REGMUX;
	itsaddr->panb = 0x0000;
	itsaddr->scrollb = 0x0000;
	return (0);
}
#endif
