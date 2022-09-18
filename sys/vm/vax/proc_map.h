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
 * Modification History: /sys/vm/vax/proc_map.h
 *
 *  1 Feb 89 -- jmartin
 *	derived from /sys/h/proc.h
 * -----------------------------------------------------------------------
 */

/*
 * This file defines the virtual memory resources of the process.
 */
struct proc_map {
	struct	pte *pm_p0br;	/* page table base P0BR */
	short	pm_szpt;	/* copy of page table size */
	struct pcb *pm_pcb;	/* pointer to pcb in uarea */
};


#define p_pcb		proc_map.pm_pcb
#define p_p0br		proc_map.pm_p0br
#define p_szpt		proc_map.pm_szpt

