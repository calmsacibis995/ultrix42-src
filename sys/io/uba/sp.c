#ifndef lint
static char *sccsid = "@(#)sp.c	4.1	(ULTRIX)	7/2/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1985-89 by			*
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
 *   This software is  derived	from  software	received  from	the	*
 *   University    of	California,   Berkeley,   and	from   Bell	*
 *   Laboratories.  Use, duplication, or disclosure is	subject  to	*
 *   restrictions  under  license  agreements  with  University  of	*
 *   California and with AT&T.						*
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
 * sp.c
 *
 * MicroVAX 2000/3100 pseudo driver for user devices
 *
 * The "sp" driver is a place holder for a user supplied device driver.
 * It provides the linkages needed to add a user written driver to the
 * system. It also contains comments about the driver to system interface.
 * The "sp" driver is not a tutorial on writing device drivers.
 *
 * Modification history
 * 
 * 03-Dec-89	Fred Canter
 *	Created this sp driver file.
 *
 */

#include "sp.h"
#if NSP > 0  || defined(BINARY)

#include "../data/sp_data.c"

#define	SPDEBUG

int spdebug = 0;

/*
 * Definition of the driver for the auto-configuration program.
 */
int	spprobe(), spattach(), spintr();
u_short spstd[] = { 0 };
struct	uba_driver spdriver =
	{ spprobe, 0, spattach, 0, spstd, "sp", spinfo };



extern	struct	nexus	nexus[];

/*
 * Routine for configuration to force the device to interrupt.
 *
 * NOTE: the KA410/KA420 CPUs don't trap on accesses to non existent
 *	 addresses. The badaddr function cannot be used to check for
 *	 device present or to determine device type. Must check the
 *	 signature in the device's firmware ROM.
 */

spprobe(reg)
	caddr_t reg;	/* DO NOT USE: physical address, needs to be virtual */
{
	/* nexus maps the interrupt controller and other CPU registers */
	register struct nb_regs *spiaddr = (struct nb_regs *)nexus;

	/* device CSR pointer */
	register struct spdevice *spaddr;

	register char *nxp;

	if(spdebug)
		printf("spprobe\n");
	else
		return(0);	/* Probe always fails unless debug flag on */

	/*
	 * Only allow this device to configure on KA410/KA420 processors.
	 */
	if (((cpu != VAXSTAR) && (cpu != C_VAXSTAR)) ||
	    ((vs_cfgtst&VS_VIDOPT) == 0))
		return(0);

	/*
	 * Map the device's I/O space before attempting
	 * to access any of its register, rom, or I/O addresses.
	 *
	 * NOTE: see also sys/machine/spt.s and io/uba/spreg.h.
	 */
	if (SPCSR_PAGES) {
	    nxp = (char *)SPCSR_PHYSADR;
	    nxaccess (nxp, SPCSRmap[0], (SPCSR_PAGES * 512));
	}
	if (SPROM_PAGES) {
	    nxp = (char *)SPROM_PHYSADR;
	    nxaccess (nxp, SPROMmap[0], (SPROM_PAGES * 512));
	}
	if (SPIOS_PAGES) {
	    nxp = (char *)SPIOS_PHYSADR;
	    nxaccess (nxp, SPIOSmap[0], (SPIOS_PAGES * 512));
	}

	/* Example of how to set device CSR pointer */
	spaddr = (struct spdevice *)spcsr;

	/* Set VDCSEL if driver uses VF interrupt */
	spiaddr->nb_vdc_sel = 1;	/* select option interrupts */

	/*
	 * Driver must make the device interrupt or it will not configure.
	 */

	/* Adjusts the vector if device has two interrupt vectors. */
	if (cvec && cvec != 0x200) /* check to see if interrupt occurred */
		cvec -= 4;	   /* point to first interrupt vector */

	return (1);	/* not sizeof anything, just says probe succeeded */
}

/*
 * Routine called to attach a device.
 */
spattach(ui)
	struct uba_device *ui;
{
#ifdef SPDEBUG
	if(spdebug)
		printf("spattach %x, %d\n", ui->ui_flags, ui->ui_unit);
#endif SPDEBUG
}


/*
 * Open routine.
 */
/*ARGSUSED*/
spopen(dev, flag)
	dev_t dev;
{
	register struct tty *tp;
	register int unit;
	register struct spdevice *addr;
	register struct uba_device *ui;
	int s;

	unit = minor(dev);
	if (unit >= nNSP || (ui = spinfo[unit])== 0 || ui->ui_alive == 0)
		return (ENXIO);
	tp = &sp_tty[unit];
}

/*
 * Close routine.
 */
/*ARGSUSED*/
spclose(dev, flag)
	dev_t dev;
	int flag;
{
}

spread(dev, uio)
	dev_t dev;
	struct uio *uio;
{
}

spwrite(dev, uio)
	dev_t dev;
	struct uio *uio;
{
}

/*
 * Interrupt service routine
 */
spintr(sp)
	int sp; /* module number */
{
}

/*
 * Ioctl routine.
 */
/*ARGSUSED*/
spioctl(dev, cmd, data, flag)
	dev_t dev;
	register int cmd;
	caddr_t data;
	int flag;
{
	register int sp, unit;
	register struct spdevice *addr;
	register struct tty *tp;
	register int s;
	struct uba_device *ui;
	struct sp_softc *sc;
	struct devget *devget;
	int error;

	/*
	 *		****** CAUTION ******
	 *
	 * All the following example code is for a serial line
	 * unit driver. The real code will be device dependent.
	 */

	unit = minor(dev);
	tp = &sp_tty[unit];
	sp = unit >> 3;	   /* module number */
	ui = spinfo[sp];
	sc = &sp_softc[sp];
	addr = (struct spdevice *)tp->t_addr;
#ifdef SPDEBUG
	if (spdebug)
		mprintf("spioctl: unit=%d, cmd=%d\n", unit, cmd&0xff);
#endif

#ifdef	notdef
	switch (cmd) {

/*
 * Example of DEVIOGET ioctl. This ioctl returns information
 * about the device to the caller. DEVIOGET allows the file
 * command to print device information.
 */
	case DEVIOCGET: 			/* device status */
		devget = (struct devget *)data;
		bzero(devget,sizeof(struct devget));

		s = spl5();
		addr->csr.low = SP_RIE|(unit & LINEMASK);
		sp_softcsr = SP_RIE|(unit & LINEMASK);
		if ((addr->fun.fs.stat&SP_MSTAT) == 0) { /* have modem cntl */
			sc->sc_category_flags[unit&LINEMASK] |= DEV_MODEM;
			if (tp->t_cflag & CLOCAL) 
			    sc->sc_category_flags[unit&LINEMASK] &= ~DEV_MODEM_ON;
			else
			    sc->sc_category_flags[unit&LINEMASK] |= DEV_MODEM_ON;
		}
		else
			sc->sc_category_flags[unit&LINEMASK] &=
				~(DEV_MODEM|DEV_MODEM_ON);
		splx(s);

		devget->category = DEV_TERMINAL;	/* terminal cat.*/

		devget->bus = DEV_NB;			/* NO bus	*/
		bcopy(DEV_TM_SLE,devget->interface,
		      strlen(DEV_VS_SLU));		/* interface	*/
		bcopy(DEV_UNKNOWN,devget->device,
		      strlen(DEV_UNKNOWN));		/* terminal	*/
		devget->adpt_num = 0;			/* NO adapter	*/
		devget->nexus_num = 0;			/* fake nexus 0 */
		devget->bus_num = 0;			/* NO bus	*/
		devget->ctlr_num = sp;			/* cntlr number */
		devget->slave_num = unit&LINEMASK;	/* line number	*/
		bcopy(ui->ui_driver->ud_dname,
		      devget->dev_name,
		      strlen(ui->ui_driver->ud_dname)); /* Ultrix "sp"	*/
		devget->unit_num = unit&LINEMASK;	/* sp line?	*/
		devget->soft_count =
		      sc->sc_softcnt[unit&LINEMASK];	/* soft err cnt */
		devget->hard_count =
		      sc->sc_hardcnt[unit&LINEMASK];	/* hard err cnt */
		devget->stat = sc->sc_flags[unit&LINEMASK]; /* status	*/
		devget->category_stat =
		      sc->sc_category_flags[unit&LINEMASK]; /* cat. stat. */
		break;

	default:
		return (ENXIO);
	}
#endif	notdef
	return (0);
}

/*
 * Stop output on a line, e.g. for ^S/^Q or output flush.
 */
/*ARGSUSED*/
spstop(tp, flag)
	register struct tty *tp;
{
	register struct spdevice *addr;
	register int unit, s;

}

spreset(uban)
	int uban;
{
}

#endif
