/*	@(#)dump.h	4.1	(ULTRIX)	7/2/90	*/
/************************************************************************
 *									*
 *			Copyright (c) 1986 by				*
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
 *			Modification History
 *
 * 13-Oct-89 -- Alan Frechette
 *	Added the variable "lastkpage" which holds the (PFN) of the
 *	last kernel page to dump out. This is needed because we no 
 *	longer dump out the buffer cache for partial selective crash
 *	dumps.
 *
 * 26-July-89 -- Alan Frechette
 *	If DEV_BSIZE is not defined then include "param.h".
 *
 * 24-July-89 -- Alan Frechette
 *	Redesigned the crash dump strategy to selectively dump out
 *	pages of physical memory. Added defines and data structures
 *	for partial selective dumping. Removed all network structures
 *	and defines from this file. All the network structures and 
 *	defines exist in the file /sys/sas/mop.h.
 *
 * 19-Mar-87 -- prs
 *	Added structures and defines to support crash dumping over
 *	the network. (diskless workstation)
 *
 ************************************************************************/

#ifndef DEV_BSIZE
#ifdef KERNEL
#include "../h/param.h"
#else
#include <sys/param.h>
#endif /* KERNEL */
#endif /* DEV_BSIZE */

/*
 * Value of dumptype. (diskless network dump)
 *   0 --> No dump.
 *  -1 --> Full dump.
 *  +# --> Partial selective dump.
 */
#define NET_NO_DUMP 		0
#define NET_FULL_DUMP 	       -1

/* 
 * Pages to be dumped. (partial selective dump)
 */
#define KERNEL_PAGES 	0	/* Dump out KERNEL IMAGE pages       */
#define KMALLOC_PAGES	1	/* Dump out KMALLOC DATA pages       */
#define AUAREA_PAGES	2	/* Dump out ACTIVE USER AREA pages   */
#define IUAREA_PAGES	3	/* Dump out INACTIVE USER AREA pages */
#define AUPROG_PAGES	4	/* Dump out ACTIVE USER PROG pages   */
#define IUPROG_PAGES	5	/* Dump out INACTIVE USER PROG pages */

#define NPAGETYPES	6	/* Number of page types supported  */
#define NPFNDESC (DEV_BSIZE/4)	/* Number of (PFN's) in a dumpdesc */
#define WRITE_DUMPDESC	1	/* Write out dump descriptor block */
#define WRITE_DUMPPAGES	2	/* Write out physical memory pages */

int dumppages[NPAGETYPES];	/* Holds the number of pages to dump */
int dumpdesc[NPFNDESC];		/* Holds page frame numbers to dump  */
int lastkpage;			/* Holds last kernel page to dump    */

/*
 * Generic dump structure.
 */
struct dumpinfo {
	int size_to_dump;	/* Initial number of blocks to dump */
	int partial_dump;	/* Partial selective dump switch    */
	int blkoffs;		/* Offset where dump device starts  */
	int pagesdumped;	/* Keep track of # of pages dumped  */
	int totaldumppages;	/* Total number of pages to dump    */
	int totaldumpblks;	/* Total number of blocks to dump   */
};

struct dumpinfo dumpinfo;
