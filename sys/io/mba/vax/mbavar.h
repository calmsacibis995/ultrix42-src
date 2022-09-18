
/*	@(#)mbavar.h	4.1	(ULTRIX)	7/2/90	*/

/************************************************************************
 *									*
 *			Copyright (c) 1984, 1986 by			*
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
 * mbavar.h	6.1	07/29/83
 *
 * Modification History
 *
 * Massbus adapter kernel structures and definitions
 *
 * 10-Jul-86   -- jaw	added adpt/nexus to ioctl
 *
 * 19-Mar-86 - ricky palmer
 *
 *	Derived from 4.2BSD labeled: mbavar.h	6.1	83/07/29.
 *	Added copyright notice and cleaned up file. V2.0
 *
 */

/*
 * This file contains definitions related to the kernel structures
 * for dealing with the massbus adapters.
 *
 * Each mba has a mba_hd structure.
 * Each massbus device has a mba_device structure.
 * Each massbus slave has a mba_slave structure.
 *
 * At boot time we prowl the structures and fill in the pointers
 * for devices which we find.
 */

/*
 * Per-mba structure.
 *
 * The initialization routine uses the information in the mbdinit table
 * to initialize the what is attached to each massbus slot information.
 * It counts the number of devices on each mba (to see if bothering to
 * search/seek is appropriate).
 *
 * During normal operation, the devices attached to the mba which wish
 * to transfer are queued on the mh_act? links.
 */
struct	mba_hd {
	short	mh_active;		/* Set if mba is active 	*/
	short	mh_ndrive;		/* No. of devices, avoid seeks	*/
	struct	mba_regs *mh_mba;	/* Virt. addr. of mba		*/
	struct	mba_regs *mh_physmba;	/* Phys. addr. of mba		*/
	struct	mba_device *mh_mbip[8]; /* What is attchd. to each dev. */
	struct	mba_device *mh_actf;	/* Head of queue to transfer	*/
	struct	mba_device *mh_actl;	/* Tail of queue to transfer	*/
};

/*
 * Per-device structure
 * (one for each RM/RP disk, and one for each tape formatter).
 *
 * This structure is used by the device driver as its argument
 * to the massbus driver, and by the massbus driver to locate
 * the device driver for a particular massbus slot.
 *
 * The device drivers hang ready buffers on this structure,
 * and the massbus driver will start i/o on the first such buffer
 * when appropriate.
 */
struct	mba_device {
	struct	mba_driver *mi_driver;
	short	mi_unit;		/* Unit number to the system	*/
	short	mi_mbanum;		/* The mba it is on		*/
	short	mi_drive;		/* Controller on mba		*/
	short	mi_dk;			/* Driver number for iostat	*/
	short	mi_alive;		/* Device exists		*/
	short	mi_type;		/* Driver specific unit type	*/
	struct	buf mi_tab;		/* Head of queue for this dev.	*/
	struct	mba_device *mi_forw;
	struct	mba_regs *mi_mba;
	struct	mba_drv *mi_drv;
	struct	mba_hd *mi_hd;
	short	mi_adpt;		/* sbi number			*/
	short	mi_nexus;		/* TR level			*/
};

/*
 * Tape formatter slaves are specified by
 * the following information which is used
 * at boot time to initialize the tape driver
 * internal tables.
 */
struct	mba_slave {
	struct	mba_driver *ms_driver;
	short	ms_ctlr;		/* Which of several formatters	*/
	short	ms_unit;		/* Which unit to system 	*/
	short	ms_slave;		/* Which slave to formatter	*/
	short	ms_alive;
};

/*
 * Per device-type structure.
 *
 * Each massbus driver defines entries for a set of routines used
 * by the massbus driver, as well as an array of types which are
 * acceptable to it.
 */
struct mba_driver {
	int	(*md_attach)(); 	/* Attach a device		*/
	int	(*md_slave)();		/* Attach a slave		*/
	int	(*md_ustart)(); 	/* Unit start routine		*/
	int	(*md_start)();		/* Setup a data transfer	*/
	int	(*md_dtint)();		/* Data transfer complete	*/
	int	(*md_ndint)();		/* Non-data transfer interrupt	*/
	short	*md_type;		/* Array of drive type codes	*/
	char	*md_dname, *md_sname;	/* Device, slave names		*/
	struct	mba_device **md_info;	/* Backptrs. to mbinit structs	*/
};

/*
 * Possible return values from unit start routines.
 */
#define MBU_NEXT	0x00		/* Skip to next operation	*/
#define MBU_BUSY	0x01		/* Dual port busy; wait on intr.*/
#define MBU_STARTED	0x02		/* Non-data transfer started	*/
#define MBU_DODATA	0x03		/* Data transfer rdy.;start mba */

/*
 * Possible return values from data transfer interrupt handling routines
 */
#define MBD_DONE	0x00		/* Data transfer complete	*/
#define MBD_RETRY	0x01		/* Error occurred, retry	*/
#define MBD_RESTARTED	0x02		/* Driver restarted i/o 	*/

/*
 * Possible return values from non-data transfer int. handling routines
 */
#define MBN_DONE	0x00		/* Non-data transfer complete	*/
#define MBN_RETRY	0x01		/* Failed; retry the operation	*/
#define MBN_SKIP	0x02		/* Don't do anything		*/

/*
 * Clear attention status for specified device.
 */
#define mbclrattn(mi)	((mi)->mi_mba->mba_drv[0].mbd_as = 1 \
			 << (mi)->mi_drive)

/*
 * Kernel definitions related to mba.
 */
#ifdef	KERNEL
int	nummba;
#if NMBA > 0
struct	mba_hd mba_hd[NMBA];
extern	struct	mba_device mbdinit[];
extern	struct	mba_slave mbsinit[];
extern	Xmba0int(), Xmba1int(), Xmba2int(), Xmba3int();
#endif
#endif /*	KERNEL */
