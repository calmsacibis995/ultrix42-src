/*
 * static  char    *sccsid = "@(#)ether_driver.h	4.1  (ULTRIX)	7/2/90";
 */
/************************************************************************
 *									*
 *			Copyright (c) 1988, 1989 by			*
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
 *									*
 * Ethernet software status per interface				*
 *	Common portion for all drivers					*
 *									*
 *			Modification History				*
 *									*
 * 18-Nov-88	- Jeffrey Mogul, DECWRL					*
 *	Created (loosely based on if_qe_data.c)				*
 *									*
 ************************************************************************/

/*
 * Ethernet software status per interface;
 *	Common portion for all drivers (to be embedded in
 *	driver-specific status block).
 *
 * Each interface is referenced by a network interface structure,
 * ess_if, which the routing code uses to locate the interface.
 * This structure contains the output queue for the interface, its address, ...
 */
#define	ess_if	 ess_ac.ac_if		/* network-visible interface 	*/
#define	ess_addr ess_ac.ac_enaddr	/* hardware Ethernet address 	*/

struct	ether_driver {
	struct	arpcom ess_ac;		/* Ethernet common part 	*/
	struct	estat ess_ctrblk;	/* Counter block		*/
	long	ess_ztime;		/* Time counters last zeroed	*/
	short	ess_enetunit;		/* unit no. for packet filter	*/
	long	ess_missed;		/* count of missed rx packets	*/
};
