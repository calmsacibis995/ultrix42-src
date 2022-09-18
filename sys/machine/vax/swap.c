#ifndef lint
static char *sccsid = "@(#)swap.c	4.1	ULTRIX	7/2/90";
#endif lint
/************************************************************************
 *									*
 *			Copyright (c) 1986, 1987, 1988, 89 by		*
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

/************************************************************************
 * Modification history : /sys/vax/swap.c
 *
 * 29-May-1990 -- Robin
 *      Moved gets() to a new file in machine/common so that it
 *      can be used by diskless kernels.
 *
 * 06-Mar-90 -- rafiey (Ali Rafieymehr)
 *	Added KDM70 support.
 *
 * 12-Jun-1989 -- gg
 *	Removed variables dmmin, dmmax and dmtext.	
 *
 * 30-May-89	darrell
 *	Added include of ../../machine/common/cpuconf.h -- cpu types
 *	were moved there.
 *
 * 5-May-89 -- Adrian Thoms
 *	Added VVAX boot device type
 *
 * 04-Apr-89 -- Tim Burke
 *	Modify do_config to allow MSCP (ra) disks with multiple major numbers
 *	to operate.
 *
 * 25-Sep-88 -- Fred Canter
 *	Clean up comments.
 *
 * 01-Sep-88 -- darrell
 *	Added checks for VAX60, since VMB uses BTD$K_SII for both
 *	DSSI and SCSI, and other changes to make swap on boot
 *	work for VAX60 SCSI disks.
 *
 * 19-Aug-88 -- fred (Fred Canter)
 *	Removed last of PVAX BTD$ kludge.
 *
 * 14-Jul-88 -- fred (Fred Canter)
 *	New VMB BTD$ number for PVAX SCSI disks (now 42).
 *	Remove pert of BTD hack which allowed sharing of BTD$ 36.
 *
 * 06-Jun-88 -- fred (Fred Canter)
 *	Make swap on boot work for PVAX SCSI disks.
 *
 * 12-Feb-88 -- fred (Fred Canter)
 *	Minor changes for VAX420 (CVAXstar/PVAX) support.
 *
 * 1-Jan-88 -- jaw
 *	remove code to search for logical unit number for a BDA disk in
 *	the case of swap on boot.  Simalar code was remove ealier for the 
 * 	UDA/CI....  This breaks the ability to have a physical unit number 
 *	different from the UNIX logical number.
 *
 * 12-11-87	Robin L. and Larry C.
 *      Added portclass/kmalloc support to the system.
 *
 * 19-Nov-87 -- rsp (Ricky Palmer)
 *	Added support for HSC and UDA types so that swap on boot works
 *	with new SCA support. Also updated code in SWAPTYPE=1 case
 *	so that disk logical unit numbers (as well as plugs (slaves)) 
 *	of 0 - 31 are supported as valid boot devices.
 *
 * 04-Dec-86 -- prs
 *	Added conditional, so "swap on boot" syntax will work on
 *	a MicroVAX I processor.
 *
 * 06-Aug-86 -- prs
 *	Merged the two files, swapgeneric.c and swapboot.c into one,
 *	swap.c. Added the compiler option SWAPTYPE to distinguish
 *	between the two types of "swap on ...." syntax.
 *	SWAPTYPE = 0 ----> "swap on ra0" or alike syntax
 *	SWAPTYPE = 1 ----> "swap on generic"
 *	SWAPTYPE = 2 ----> "swap on boot"
 *
 ************************************************************************/

#include "mba.h"

#include "../h/types.h"
#include "../machine/pte.h"
#include "../machine/cpu.h"
#include "../../machine/common/cpuconf.h"

#include "../h/param.h"
#include "../h/conf.h"
#include "../h/buf.h"
#include "../h/vm.h"
#include "../h/vmmac.h"
#include "../h/systm.h"
#include "../h/reboot.h"

#include "../machine/cons.h"
#include "../machine/mtpr.h"
#include "../machine/rpb.h"
#include "../machine/io/mba/mbareg.h"
#include "../machine/io/mba/mbavar.h"
#include "../io/uba/ubareg.h"
#include "../io/uba/ubavar.h"

#include "../machine/sas/vmb.h"

/*
 * Swap on boot generic configuration
 */
dev_t	rootdev, dumpdev;
int	nswap;

#if SWAPTYPE > 0
struct	swdevt swdevt[] = {
	{ -1,	1,	0 },
	{ 0,	0,	0 },
};
#endif

long	dumplo;
int	unit = 0;
int	swaponroot = 0;

extern struct mba_device mbdinit[];
extern struct uba_device ubdinit[];

struct	uba_driver *uba_drive;
struct	uba_hd	   *uba_head;
struct	uba_device *uba_dev;

struct	mba_driver *mba_drive;
struct	mba_hd	   *mba_head;
struct	mba_device *mba_dev;

extern 	int *vmbinfo;

extern	struct rpb;

setconf()
/*
 * Only include this section of code if "swap on boot" syntax was used.
 */
#if SWAPTYPE == 2
{
	register struct genericconf *gc;
	int found_flag = 0;
	long longp;
	int rpbunit;

	if (!vmbinfo) {
		printf("boot device not found\n");
		ask_for_root();
		return(0);
	}
	/*
	 * Search table for boot device
	 */
	for (gc = genericconf; gc->gc_driver; gc++)
		if (gc->gc_BTD_type == rpb.devtyp) {
#ifdef VAX60
	/* HACK - DAD
	 * BTD$K_SII is used for with the DSSI driver and the SCSI
	 * driver. The following is a quick hack to get GENERIC 
	 * kernels working on VAX60.  A better solution needs
	 * to be found.
	 */
		    if ((cpu == VAX_60) && (rpb.devtyp == BTD$K_SII)){
			if (strcmp(gc->gc_name, "rz") != 0) {
			    continue;
			}
		    }
#endif VAX60
		    found_flag = 1;
		    break;
		}
	if (!found_flag) {
		printf("boot device not in table\n");
		ask_for_root();
		return(0);
	}
	unit = -1;
	/*
	 * Scan device structures for ULTRIX number of the boot device.
	 */
	switch(rpb.devtyp) {

	case BTD$K_MB:
		for (mba_dev = (struct mba_device *)mbdinit;mba_dev->mi_driver;mba_dev++) {
			if (mba_dev->mi_alive) {
				mba_drive = mba_dev->mi_driver;
				if (*mba_drive->md_dname == *gc->gc_name)
				if (mba_dev->mi_drive == rpb.unit) {
					mba_head = mba_dev->mi_hd;
					longp = (long)mba_head->mh_physmba;
					if (longp == rpb.adpphy) {
						unit = (long)mba_dev->mi_unit;
						break;
					}
				}
			}
		}
		break;
	case BTD$K_DQ:
		for (uba_dev = (struct uba_device *)ubdinit;uba_dev->ui_driver;uba_dev++) {
			if (uba_dev->ui_alive) {
				uba_drive = uba_dev->ui_driver;
				if (*uba_drive->ud_dname ==*gc->gc_name)
				if (uba_dev->ui_slave == rpb.unit) {
				     uba_head = uba_dev->ui_hd;
				     longp = (long)uba_head->uh_physuba;
				     if (longp == rpb.adpphy) {
				 	  unit = (long)uba_dev->ui_unit;
					  break;
				     }
				}
			}
		}
		break;
        case BTD$K_DISK9:                               /* VVAX */
	case BTD$K_BDA:
	case BTD$K_UDA: 
	case BTD$K_KDM70: 
	case BTD$K_HSCCI: 
	case BTD$K_SII:
	case BTD$K_BVPSSP:
	    unit = rpb.unit;
#ifdef VAX60
		/*
		 * This is needed because the SCSI driver as well as the
		 * DSSI driver use DTD$K_SII.  VMB passes the SCSI unit number
		 * with bus number in the unit number.  Divide by 100 in that 
		 * case to get the unit number.
		 *
		 * This shouldn't be done for the DSSI case because it will
		 * limit the maximum DSSI ra disk unit number to be 99 on the
		 * VAX_60.
		 */
		if(cpu == VAX_60) {
		    if(unit >=100) {
			unit = unit / 100;
		    }
		}
#endif VAX60

		break;
	case BTD$K_KA420_DISK:
		for (uba_dev = (struct uba_device *)ubdinit;uba_dev->ui_driver;uba_dev++) {
		    if (uba_dev->ui_alive == 0)
			continue;
		    if ((long)uba_dev->ui_physaddr == rpb.csrphy) {
			uba_drive = uba_dev->ui_driver;
			if (*uba_drive->ud_dname == *gc->gc_name) {
			    rpbunit = rpb.unit;
			    if (rpbunit >= 100)
				rpbunit = rpbunit / 100;
			    if (uba_dev->ui_slave == rpbunit) {
				if (cpu_subtype==ST_VAXSTAR || cpu==MVAX_I) {
				    unit = (long)uba_dev->ui_unit;
				    break;
				}
				uba_head = uba_dev->ui_hd;
				longp = (long)uba_head->uh_physuba;
				if (longp == rpb.adpphy) {
				    unit = (long)uba_dev->ui_unit;
				    break;
				}
			    }
			}
		    }
		}
		break;
	default:
		for (uba_dev = (struct uba_device *)ubdinit;uba_dev->ui_driver;uba_dev++) {
			if (uba_dev->ui_alive) {
				if ((long)uba_dev->ui_physaddr == rpb.csrphy) {
					uba_drive = uba_dev->ui_driver;
					if (*uba_drive->ud_dname == *gc->gc_name)
					if (uba_dev->ui_slave == rpb.unit) {
					     if((cpu_subtype == ST_VAXSTAR) ||
					       (cpu == MVAX_I)) {
					 	 unit = (long)uba_dev->ui_unit;
						 break;
					     }
					     uba_head = uba_dev->ui_hd;
					     longp = (long)uba_head->uh_physuba;
					     if (longp == rpb.adpphy) {
					 	  unit = (long)uba_dev->ui_unit;
						  break;
					     }
					}
				}
			}
		}
	} /* switch */

	if (unit == -1) {
		printf("root device not configured\n");
		ask_for_root();
		return(0);
	}
	do_config(gc, unit);
	return;
}

/*
 * This code is taken from swapgeneric
 */

ask_for_root()
#endif SWAPTYPE == 2
{
/*
 * If the "swap on ra0" or alike syntax was used, then basically return.
 * Please note that the "swap on boot" and the "swap on generic" syntax
 * use this section of code.
 */
#if SWAPTYPE > 0
	register struct mba_device *mi;
	register struct uba_device *ui;
	register struct genericconf *gc;
	char cname[20];
	char *name = cname;
	char *name_hold = "  ";
	int  max_unitnum = 31;
/*
 * The swapgeneric code essentially had 2 functions, one to
 * prompt the user for the root device if the boot ask flag was set.
 * The second funtion it had was to first try to find a suitable root,
 * if none was found then halt. Since the "swap on boot" syntax defaults
 * to the prompting functionality, we have to special case the functionality
 * of the swap program figuring out a suitable root.
 */
#if SWAPTYPE == 1
	if (boothowto & RB_ASKNAME) {
#endif
retry:
		printf("root device? ");
		gets(name);
		for (gc = genericconf; gc->gc_driver; gc++)
			if (gc->gc_name[0] == name[0] &&
		    	gc->gc_name[1] == name[1])
				goto gotit;
		goto bad;
gotit:
		if (strlen(name) && name[strlen(name)-1] == '*') {
                        name[strlen(name)-1] = '\0';
                        swaponroot++;
                }
		if (MSCP_B_DEV(gc->gc_root)) {
			max_unitnum = MSCP_MAXDISK;
		}
		while(name[0] && (name[0] < '0' || name[0] > '9'))
                        name++;
		if (atoi(name) >= 0 && atoi(name) <= max_unitnum) {
                        unit = atoi(name);
                        do_config(gc, unit);
                        return;
                }
		printf("bad/missing unit number\n");
bad:
		printf("use ");
		for (gc = genericconf; gc->gc_driver; gc++) {
			/*
		 	 * Don't print ra twice !!
		 	 */
			if (strcmp(name_hold, gc->gc_name) != 0)
				printf("%s%%d ", gc->gc_name);
			name_hold = gc->gc_name;
		}
		printf("\n");
		goto retry;
/*
 * We must special case the functionality of calculating a suitable root
 * device, so the "swap on boot" syntax can take advantage of this code.
 */
#if SWAPTYPE == 1
	}
	unit = 0;
	for (gc = genericconf; gc->gc_driver; gc++) {
		for (mi = mbdinit; mi->mi_driver; mi++) {
			if (mi->mi_alive == 0)
				continue;
			if (mi->mi_unit == 0 && mi->mi_driver ==
			    (struct mba_driver *)gc->gc_driver) {
				printf("root on %s0\n",
				    mi->mi_driver->md_dname);
				do_config(gc, unit);
				return;
			}
		}
		for (ui = ubdinit; ui->ui_driver; ui++) {
			if (ui->ui_alive == 0)
				continue;
			if (ui->ui_unit == 0 && ui->ui_driver ==
			    (struct uba_driver *)gc->gc_driver) {
				printf("root on %s0\n",
				    ui->ui_driver->ud_dname);
				do_config(gc, unit);
				return;
			}
		}
	}
	printf("no suitable root\n");
	asm("halt");
#endif SWAPTYPE == 1
#endif SWAPTYPE > 0
}

do_config(gc, unit)
	struct genericconf *gc;
	int unit;
{
	int major_num, minor_num;

	major_num = major(gc->gc_root);
	/*
	 * MSCP disks (ra's) can occupy a range of unit numbers.  Determine
	 * major and minor number from the unit number.
	 *
	 * Shift left by 3 because the partition occupies the lower 3 bits
	 * of the minor number.  Add 1 to the unit number to indicate that
	 * swaping is done on the "b" partition.
	 */
	if (MSCP_B_DEV(gc->gc_root)) {
		/*
		 * The do_config routine can be called more than once.  If this
		 * is done the major number would be increased twice putting
		 * it at the wrong major number and possibly causing a panic.
		 * To avoid this only increase the major number if it is 
		 * presently set to the lowest major number.
		 */
		if ((major_num == MSCP_B_MIN) && (unit >= MSCP_UNITS_MAJOR))
			major_num += (unit/MSCP_UNITS_MAJOR);
		minor_num = (unit % MSCP_UNITS_MAJOR) << 3;
	}
	else {
		minor_num = unit << 3;
	}
	gc->gc_root = makedev(major_num, minor_num);
	rootdev = gc->gc_root;
	swdevt[0].sw_dev = dumpdev =
	    makedev(major(rootdev), minor(rootdev)+1);	/* b partition */
	/* swap size and dumplo set during autoconfigure */
	if (swaponroot)
		rootdev = dumpdev;
}

