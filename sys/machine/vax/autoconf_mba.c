/*
 * autoconf_mba.c
 */

#ifndef lint
static char *sccsid = "@(#)autoconf_mba.c	4.1	ULTRIX	7/2/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1983 by				*
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
 * Modification History:
 *
 * 10-Jul-86   -- jaw	added adpt/nexus to ioctl
 *
 * 18-Mar-86 -- jaw  add routines to cpu switch for nexus/unibus addreses
 *		     also got rid of some globals like nexnum.
 *
 * 12-Mar-85 -tresvik
 *	Removed previous change.  Problem taken care of in autoconf.c
 *
 * 27-Feb-85 -tresvik
 *	Changes for support of the VAX8600 were merged in.
 *
 */

#include "mba.h"
#include "uba.h"

#include "../machine/pte.h"

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/map.h"
#include "../h/buf.h"
#include "../h/dk.h"
#include "../h/vm.h"
#include "../h/conf.h"
#include "../h/dmap.h"

#include "../machine/cpu.h"
#include "../machine/mem.h"
#include "../machine/mtpr.h"
#include "../machine/nexus.h"
#include "../machine/scb.h"
#include "../io/mba/vax/mbareg.h"
#include "../io/mba/vax/mbavar.h"
#include "../io/uba/ubareg.h"
#include "../io/uba/ubavar.h"


extern	int	dkn;
extern	int	(*mbaintv[])();
extern	int	(*ubaintv[])();
extern	int	ioanum;
extern	int	nNMBA;
extern	int	nNUBA;
extern	int	(*Set_conf)();
extern	int	isGENERIC;

#if NMBA > 0
struct	mba_device *mbaconfig();
/*
 * Find devices attached to a particular mba
 * and look for each device found in the massbus
 * initialization tables.
 */
mbafind(nxv, nxp,nexnum)
	struct nexus *nxv, *nxp;
	int nexnum;
{
	register struct mba_regs *mdp;
	register struct mba_drv *mbd;
	register struct mba_device *mi;
	register struct mba_slave *ms;
	int dn, dt, sn;
	struct mba_device fnd;

	mdp = (struct mba_regs *)nxv;
	mba_hd[nummba].mh_mba = mdp;
	mba_hd[nummba].mh_physmba = (struct mba_regs *)nxp;
	setscbnex(mbaintv[nummba],nexnum);
	fnd.mi_mba = mdp;
	fnd.mi_mbanum = nummba;
	for (mbd = mdp->mba_drv, dn = 0; mbd < &mdp->mba_drv[8]; mbd++, dn++) {
		/*
		 * Unfortunately, when a dual port kit is added to a
		 * TM78 the Drive present (DPR) bit goes away and TM78
		 * drives cannot be sized.  Therefore,  the check for
		 * the TAP bit in the drive type register.
		 *
		 * Perhaps, this check could be changed to sense for
		 * non-exisitant drive (NED) after accessing a drive
		 * register.
		 */
 		if (((mbd->mbd_ds&MBDS_DPR) == 0) && 
		    (!(mbd->mbd_dt & MBDT_TAP)))
			continue;
		mdp->mba_sr |= MBSR_NED;		/* si kludge */
		dt = mbd->mbd_dt & 0xffff;
		if (dt == 0)
			continue;
		if (mdp->mba_sr&MBSR_NED)
			continue;			/* si kludge */
		if (dt == MBDT_MOH)
			continue;
		fnd.mi_drive = dn;
#define	qeq(a, b)	( a == b || a == '?' )
		if ((mi = mbaconfig(&fnd, dt,nexnum)) && (dt & MBDT_TAP))
		    for (sn = 0; sn < 8; sn++) {
			mbd->mbd_tc = sn;
		        for (ms = mbsinit; ms->ms_driver; ms++)
			    if (ms->ms_driver == mi->mi_driver &&
				ms->ms_alive == 0 && 
				qeq(ms->ms_ctlr, mi->mi_unit) &&
				qeq(ms->ms_slave, sn) &&
				(*ms->ms_driver->md_slave)(mi, ms, sn)) {
					printf("%s%d at %s%d slave %d\n"
					    , ms->ms_driver->md_sname
					    , ms->ms_unit
					    , mi->mi_driver->md_dname
					    , mi->mi_unit
					    , sn
					);
					ms->ms_alive = 1;
					ms->ms_ctlr = mi->mi_unit;
					ms->ms_slave = sn;
				}
		    }
	}
	mdp->mba_cr = MBCR_INIT;
	mdp->mba_cr = MBCR_IE;
}

/*
 * Have found a massbus device;
 * see if it is in the configuration table.
 * If so, fill in its data.
 */
struct mba_device *
mbaconfig(ni, type,nexnum)
	register struct mba_device *ni;
	register int type,nexnum;
{
	register struct mba_device *mi;
	register short *tp;
	register struct mba_hd *mh;

	for (mi = mbdinit; mi->mi_driver; mi++) {
		if (mi->mi_alive)
			continue;
		tp = mi->mi_driver->md_type;
		for (mi->mi_type = 0; *tp; tp++, mi->mi_type++)
			if (*tp == (type&MBDT_TYPE))
				goto found;
		continue;
found:
#define	match(fld)	(ni->fld == mi->fld || mi->fld == '?')
		if (!match(mi_drive) || !match(mi_mbanum))
			continue;
		printf("%s%d at mba%d drive %d\n",
		    mi->mi_driver->md_dname, mi->mi_unit,
		    ni->mi_mbanum, ni->mi_drive);
		mi->mi_alive = 1;
		mh = &mba_hd[ni->mi_mbanum];
		mi->mi_hd = mh;
		mh->mh_mbip[ni->mi_drive] = mi;
		mh->mh_ndrive++;
		mi->mi_mba = ni->mi_mba;
		mi->mi_drv = &mi->mi_mba->mba_drv[ni->mi_drive];
		mi->mi_mbanum = ni->mi_mbanum;
		mi->mi_adpt = ioanum;
		mi->mi_nexus = nexnum;
		mi->mi_drive = ni->mi_drive;
		/*
		 * If drive has never been seen before,
		 * give it a dkn for statistics.
		 */
		if (mi->mi_driver->md_info[mi->mi_unit] == 0) {
			mi->mi_driver->md_info[mi->mi_unit] = mi;
			if (mi->mi_dk && dkn < DK_NDRIVE)
				mi->mi_dk = dkn++;
			else
				mi->mi_dk = -1;
		}
		(*mi->mi_driver->md_attach)(mi);
		return (mi);
	}
	return (0);
}
#endif
