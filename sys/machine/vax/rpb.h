/* sccsid  =  @(#)rpb.h	4.1	ULTRIX	7/2/90 */
/************************************************************************
 *									*
 *			Copyright (c) 1985 by				*
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
/*	rpb.h	6.1	83/07/29	*/
/*
 * Modification History:
 *
 * 14-Aug-86 -- tresvik
 *	add cpu and cpu_subtype to the end of the rpb for use by
 *	installation developers
 *
 * 12-Jun-86 -- tresvik
 *	add pointer to vmbinfo at the top of memory so the network 
 *	installation software can find the network parameter block
 *	appended to vmb_info.
 *
 * 4-feb-86 -- tresvik
 *	expanded stucture definition for use by VMB boot path
 *
 */
/*
 * The restart parameter block, which is a page in (very) low
 * core which runs after a crash.  Currently, the restart
 * procedure takes a dump.
 */
struct rpb {
	struct	rpb *rp_selfref;	/* self-reference */
	int	(*rp_dumprout)();	/* routine to be called */
	long	rp_checksum;		/* checksum of 31 words of dumprout */
	long	rp_flag;		/* set to 1 when dumprout runs */
	long	haltpc;			/* pc at restart/halt */
	long	haltpsl;		/* pscl at restart/halt */
	long	haltcode;		/* code describing restart reason */
	long	bootr0;			/* saved boot parameter r0 */
	long	bootr1;			/* saved boot parameter r1 */
	long	bootr2;			/* saved boot parameter r2 */
	long	bootr3;			/* saved boot parameter r3 */
	long	bootr4;			/* saved boot parameter r4 */
	long	bootr5;			/* saved boot parameter r5 */
	long	iovec;			/* address of bootstrap QIO vector */
	long	iovecsz;		/* size of boot qio routine */
	long	fillbn;			/* logical block number of boot file */
	long	filsiz;			/* size of boot file */
	long	pfnmap[2];		/* descriptor for PRN bitmap */
	long	pfncnt;			/* count of physical pages */
	long	svaspt;			/* system virtual address of spt */
	long	csrphy;			/* uba device csr address (physical) */
	long	csrvir;			/* uba device csr address (virtual) */
	long	adpphy;			/* adapter config reg (physical) */
	long	adpvir;			/* adapter config reg (virtual) */
	short	unit;			/* unit number */
	char	devtyp;			/* device type code */
	char	slave;			/* slave unit number */
	char	file[40];		/* boot file name (ascic) */
	char	confreg[16];		/* array of adapter types */
	char	hdrpgcnt;		/* count of header pages */
	char	bootndt[2];		/* boot adapter nexus devtyp */
	char	flags;			/* misc flag bits */
	long	isp;			/* power fail interrupt sp */
	long	pcbbas;			/* process control block base */
	long	sbr;			/* system base register */
	long	scbb;			/* system control block base */
	char	*cca_addr;		/* Calypso CCA addr.  This field was
					   used by VMS for software interrupt
					   summary reg */
	long	slr;			/* system length register */
	char	memdsc[64];		/* memory descriptor */
	long	bugchk;			/* bugchk loop addr for MP secondary */
	char	wait[4];		/* bugchk loop code for MP secondary */
	long	badpgs;			/* number of bad pages found in scan */
	char	ctrllr;			/* controller letter designator */
	/*
	 * The following belong to us for network boot and installation
	 */
	long	*vmbinfo;		/* points to the vmbinfo structure 
					   at the top of memory */
	long	cpu;			/* Save the cpu type */
	long	cpu_subtype;		/* Save the cpu subtype */
	long	ws_display_type;	/* major number of graphics console */
/* the dump stack grows from the end of the rpb page not to reach here */
};
#ifdef KERNEL
extern	struct rpb rpb;
#endif
