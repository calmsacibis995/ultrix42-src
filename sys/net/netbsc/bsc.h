/* static	char	*sccsid = "@(#)bsc.h	4.1  (ULTRIX)		 7/2/90"; */

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
 * Socket address.
 */
struct sockaddr_bsc {
	u_short	sin_family;
	char 	sin_addr[14];		
};

/* 
 * SUBNET routing changes.
 */
struct bsc_ifaddr {
	struct 	ifaddr		bia_ifa;
#define		bia_addr	bia_ifa.ifa_addr
#define		bia_broadaddr	bia_ifa.ifa_broadaddr
#define		bia_dstaddr	bia_ifa.ifa_dstaddr
#define		bia_ifp		bia_ifa.ifa_ifp
	u_long	bia_net;
	u_long	bia_netmask;
	u_long	bia_subnet;
	u_long  bia_subnetmask;
	int	bia_flags;
	struct	bsc_ifaddr	*bia_next;
};


#ifdef KERNEL
struct  bscpcb *bsc_pcblookup();
extern  struct bsc_ifaddr *bsc_ifaddr;
extern	struct domain bscdomain;
extern	struct protosw bscnetsw[];
extern 	int nBSC;
#endif

#define BIA_SIN(bia)((struct sockaddr_bsc *)(&((struct bsc_ifaddr *)bia)->bia_addr))
