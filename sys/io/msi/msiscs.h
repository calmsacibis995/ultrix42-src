/*
 *	@(#)msiscs.h	2.7	(ULTRIX)	10/12/89
 */

/************************************************************************
 *                                                                      *
 *                      Copyright (c) 1989 by                           *
 *              Digital Equipment Corporation, Maynard, MA              *
 *                      All rights reserved.                            *
 *                                                                      *
 *   This software is furnished under a license and may be used and     *
 *   copied  only  in accordance with the terms of such license and     *
 *   with the  inclusion  of  the  above  copyright  notice.   This     *
 *   software  or  any  other copies thereof may not be provided or     *
 *   otherwise made available to any other person.  No title to and     *
 *   ownership of the software is hereby transferred.                   *
 *                                                                      *
 *   The information in this software is subject to change  without     *
 *   notice  and should not be construed as a commitment by Digital     *
 *   Equipment Corporation.                                             *
 *                                                                      *
 *   Digital assumes no responsibility for the use  or  reliability     *
 *   of its software on equipment which is not supplied by Digital.     *
 *                                                                      *
 ************************************************************************
 *
 *
 *   Facility:	Systems Communication Architecture
 *		Mayfair Storage Interconnect Port Driver
 *
 *   Abstract:	This module contains Mayfair Storage Interconnect Port
 *		Driver( MSI ) constants and data structure definitions
 *		visible to SCS.
 *
 *   Creator:	Todd M. Katz	Creation Date:	December 06, 1988
 *
 *   Modification History:
 *
 *   16-Jun-1989	Pete Keilty
 *	Changes smp locks to type of struct lock_t.
 */

/* MSI Data Structure Definitions.
 */
typedef struct _msi_dmapinfo {		/* Double Mapping Buffer Information */
    u_long		protopte;	/* Proto-pte( only PFN is missing )  */
    struct pte		*dmap_bpteaddr;	/* Double mapping buffer sva pte ptr */
    char		*dmap_baddr;	/* Double mapping buffer address     */
    union	{			/* First overlaid field		     */
	u_char		*saddr;		/*  Target/Source segment address    */
	u_long		sboff;		/*  Target/Source segment byte offset*/
    } un1;
#define	Saddr	un1.saddr
#define	Sboff	un1.sboff
    u_long		ssize;		/* Number of bytes in segment	     */
} MSI_DMAPINFO;

typedef	struct _msi_portid {		/* Local/Remote MSI Port ID Info     */
    u_short		port_type[ 2 ];	/* Port type			     */
    struct _msirpi	portinfo;	/* Port information		     */
    u_short		mbz[ 10 ];	/* MBZ				     */
} MSI_PORTID;

typedef	struct _msi_pportinfo {		/* Per-DSSI Port Information	     */
    struct _msibq xretryq;		/* MSIB transmit retry queue	     */
    struct		{		/* Remote port status flags	     */
	u_long	path		:  1;	/*  Path exists			     */
	u_long	vc		:  1;	/*  Virtual circuit enabled	     */
	u_long	dip		:  1;	/*  Transmit delaying in progress    */
	u_long			: 29;
    } rpstatus;
					/* Remote port status flag bit masks */
#define	MSI_RPPATH	0x00000001	/*  ( used when performance matters )*/
#define	MSI_RPVC	0x00000002
#define	MSI_RPDIP	0x00000004
    u_long	  xretry_timer;		/* Xmt retry timer( 10 msec units )  */
    u_long	  xretrys;		/* Current transmit retry attempt    */
    u_long	  xseqno;		/* Next transmit sequence number     */
					/*  ( Occupies bits: 9-11 )	     */
    u_long	  rseqno;		/* Next expected receive seq number  */
					/*  ( Occupies bits: 9-11 )	     */
} MSI_PPORTINFO;

/* Field				     Lock
 * ---------------			--------------
 * comqh				COMQH
 * comql				COMQL
 * mfreeq				MFREEQ
 * dfreeq				DFREEQ
 * rbusy				RFP
 * xbusy				XFP
 * xfree				XFP
 * lpstatus.active			XFP/RFP - need both to modify
 * lpstatus.timer			XFP
 * rdmap				RFP
 * xdmap				XFP
 * perport.xretryq			XFP
 * perport.rpstatus.path		XFP
 * perport.rpstatus.vc			XFP/RFP - need both to modify
 * perport.rpstatus.dip			XFP
 * perport.xretry_timer			XFP
 * perport.xretrys			XFP
 * perport.xseqno			XFP
 * perport.rseqno			RFP
 *
 * All other volatile fields/bits controlled by standard PCCB lock.
 *
 * Lock Hierarchy: PCCB -> PB -> RFP -> XFP -> { COMQH,COMQL,DFREEQ,MFREEQ }
 */
typedef	struct _msipccb	{		/* MSI Specific Fields of PCCB	     */
    struct _msibq	comqh;		/* MSIB high priority command queue  */
    struct _msibq	comql;		/* MSIB low priority command queue   */
    struct _msibq	mfreeq;		/* MSIB message free queue	     */
    struct _msibq	dfreeq;		/* MSIB datagram free queue	     */
    struct _siibq	*rbusy;		/* First rcv-in-progress SIIBUF ptr  */
    struct _siibq	*xbusy;		/* First xmt-in-progress SIIBUF ptr  */
    struct _siibq	*xfree;		/* First free transmit SIIBUF pointer*/
    u_char		*siibuffer;	/* SII 128K RAM buffer address	     */
    u_char		*siiregs;	/* SII registers base address	     */
    u_long		randomseed;	/* Random number generator seed	     */
    u_long		pkt_size;	/* Size of port command	packet	     */
    u_long		msg_ovhd;	/* Size of message overhead	     */
    u_long		dg_ovhd;	/* Size of datagram overhead	     */
    u_short		retdat_ovhd;	/* Size of RETDAT overhead	     */
    u_short		lretdat_cssize;	/* Size of local RETDAT comp section */
    struct		{		/* MSI register pointers	     */
	vu_short	*msicsr;	/*  Control/Status register	     */
	vu_short	*msidscr;	/*  DSSI control register	     */
	vu_short	*msidssr;	/*  DSSI status register	     */
	vu_short	*msiidr;	/*  ID register			     */
	vu_short	*msitr;		/*  Timeout register		     */
	vu_short	*msitlp;	/*  Target list pointer register     */
	vu_short	*msiilp;	/*  Initiator list pointer register  */
	vu_short	*msidcr;	/*  Diagnostic control register	     */
	vu_short	*msicomm;	/*  SII command register	     */
	vu_short	*msidstat;	/*  Data transfer status register    */
	vu_short        *msiisr3;       /*  Main control diagnostic register */
    } siiregptrs;
    struct		{		/* Local port status flags	     */
	u_long	init		:  1;	/*  First time initialization 	     */
	u_long	active		:  1;	/*  Port active			     */
	u_long	timer		:  1;	/*  Retry delay timer active 	     */
	u_long	xfork		:  1;	/*  Transmit Fork Process scheduled  */
	u_long	rfork		:  1;	/*  Receive Fork Process scheduled   */
	u_long  optlpcinfo	:  1;	/*  Opt local port crash info flag   */
	u_long			: 26;
    } lpstatus;
					/* Local port status flag bit masks  */
#define	MSI_ACTIVE	0x00000002	/*  ( used when performance matters )*/
#define	MSI_XFORK	0x00000008
#define	MSI_RFORK	0x00000010
    struct		{		/* Optional local port crash info    */
	struct _msih	*pkth;		/*  Address of MSI packet	     */
	u_long		pktsize;	/*  Size of MSI packet		     */
 	u_long		pport_addr;	/*  Packet remote port station addr  */
    } lpcinfo;
    union		{		/* Optional error logging information*/
	u_long		portnum;	/*  Remote port station address	     */
    } errlogopt;
    u_short		min_msg_size;	/* Minimum message size		     */
    u_short		max_msg_size;	/* Maximum message size		     */
    u_short		min_dg_size;	/* Minimum datagram size	     */
    u_short		max_dg_size;	/* Maximum datagram size	     */
    u_short		min_id_size;	/* Minimum ID size		     */
    u_short		max_id_size;	/* Maximum ID size		     */
    u_short		min_idreq_size;	/* Minimum IDREQ size		     */
    u_short		max_idreq_size;	/* Maximum IDREQ size		     */
    u_short		min_sntdat_size;/* Minimum SNTDAT size		     */
    u_short		max_sntdat_size;/* Maximum SNTDAT size		     */
    u_short		min_datreq_size;/* Minimum DATREQ{0,1,2} size	     */
    u_short		max_datreq_size;/* Maximum DATREQ{0,1,2} size	     */
    u_short		save_dssr;	/* Cached DSSI status register	     */
    u_short		save_dstat;	/* Cached data transfer status reg   */
    struct _msi_dmapinfo rdmap;		/* Receive Fork Process dmap buf info*/
    struct _msi_dmapinfo xdmap;		/* Transmit Fork Process dmap bufinfo*/
    struct kschedblk	rforkb;		/* Receive Fork Process fork block   */
    struct kschedblk	xforkb;		/* Transmit Fork Process fork block  */
    struct lock_t	comqh_lk;	/* Command queue high lock structure */
    struct lock_t	comql_lk;	/* Command queue low lock structure  */
    struct lock_t	dfreeq_lk;	/* Datagram free queue lock structure*/
    struct lock_t	mfreeq_lk;	/* Message free queue lock structure */
    struct lock_t	rfp_lk;		/* Receive Fork Process lock struct  */
    struct lock_t	xfp_lk;		/* Transmit Fork Process lock struct */
    struct _msi_portid	lpidinfo;	/* Local port identification info    */
    struct _msi_pportinfo perport[ MSI_MAXNUM_PORT ]; /* Per-DSSI port info  */
} MSIPCCB;
