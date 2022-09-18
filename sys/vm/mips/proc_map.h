/* 	@(#)proc_map.h	4.1	(ULTRIX)	7/2/90 	*/
/************************************************************************
 *									*
 *			Copyright (c) 1989 by				*
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

/* ------------------------------------------------------------------------
 * Modification History: /sys/vm/mips/proc_map.h
 *
 * 30-Nov-89 -- jmartin
 *	Support for coprocessor access to process virtual memory.
 *
 *  1 Feb 89 -- jmartin
 *	derived from /sys/h/proc.h
 * -----------------------------------------------------------------------
 */

/*
 * This file defines the virtual memory resources of the process.
 */

struct proc_map {
	int	pm_tlbpid;	/* tlb context of this proc */
	struct	tlbinfo *pm_tlbinfo;	/* per process tlb mappings */
	int	pm_tlbindx;	/* index of next wired tlb entry to use */
	long	pm_textstart;	/* starting address of user text */
	long	pm_datastart;	/* starting address of user data */
	struct	pte *pm_textbr;	/* text  page table base */
	struct	pte *pm_databr;	/* data  page table base */
	struct	pte *pm_stakbr;	/* stack page table base */
	short	pm_textpt;	/* copy of text  page table size */
	short	pm_datapt;	/* copy of data  page table size */
	short	pm_stakpt;	/* copy of stack page table size */
	/* tlb update routine for a device owned by this proc */
	int	(*dev_VM_maint)();
};

#define p_tlbpid	proc_map.pm_tlbpid
#define p_tlbinfo	proc_map.pm_tlbinfo
#define p_tlbindx	proc_map.pm_tlbindx
#define p_textstart	proc_map.pm_textstart
#define p_datastart	proc_map.pm_datastart
#define p_textbr	proc_map.pm_textbr
#define p_databr	proc_map.pm_databr
#define p_stakbr	proc_map.pm_stakbr
#define p_textpt	proc_map.pm_textpt
#define p_datapt	proc_map.pm_datapt
#define p_stakpt	proc_map.pm_stakpt
#define p_dev_VM_maint	proc_map.dev_VM_maint

/*
 * *p_dev_VM_maint is a function of two arguments:  the first is a
 * command or operation token; the second is a virtual page number in
 * the process.  The purpose of the function is to synchronize the
 * invalidation of address translations with a device which uses those
 * translations.  The commands are defined below.
 *
 * The function returns NULL for failure and !NULL for success.
 * Presently, it is necessary that the function succeed.  Therefore,
 * this function stalls until the device it serves is done with the
 * address translation for the virtual page.  In the future, failure
 * would indicate that the device is busy with the page, and that page
 * is not presently available for invalidation.
 */

#define PDEVCMD_ONE	0	/* invalidate the specified translation */
#define PDEVCMD_ALL	1	/* invalidate all device translation data */
/*
 * The following operation would be useful if shared memory were
 * swapped, which it isn't, or if expand()/ptexpand() freed page table
 * pages when shrinking a region, which they don't.  As it is, only
 * swapout changes the physical location of process page table pages.
 * ptexpand() changes the virtual address of page table pages, but not
 * the physical address.
 */
#define PDEVCMD_TOP	2	/* invalidate a freed page table page */
