/*
 * if.h
 */

/* static     char    *sccsid = "@(#)if.h	4.8	(ULTRIX)	11/14/90"; */

/************************************************************************
 *									*
 *		      Copyright (c) 1985, 1989 by			*
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
 *			Modification History				*
 *
 *	Chran-Ham Chang 9-Nov-90
 *		Added structure to support FDDI read status 
 *
 *	Ursula Sinkewicz 18-Sep-90					
 *		X.25 changes (from NAC)					
 *									*
 *	Chran-Ham Chang 7-Sep-90				        *
 *		Added structures to support FDDI MIB  			* 
 *								        *
 *	Matt Thomas 20-Aug-90						*
 *		Add if_sysid_type cell to ifnet structure.		*
 *									*
 *	Chran-Ham Chang 9-Aug-90				        *
 *		Added ifeeporm structure to support the FDDI EEPROM     * 
 *		update					                *
 *									*
 *	Matt Thomas 20-Nov-89						*
 *		Augment ifstate for DSV11 (and other device) support	*
 *									*
 *	Uttam Shikarpur 24-Oct-89					*
 *	Added ifversion field to ifnet structure, and est_mbloksent and *
 *	est_mbytesent to the estat structure for network management.	*	
 *									*
 *	Matt Thomas 17-Jul-89						*
 *		Change ifnetptr references to m_ifp			*
 *									*
 *	Ursula Sinkewicz 28-Feb-89					*
 * 		SMP/mips merge. (Added changes from R. Bhanukitsiri	*
 * 		02/06/89).						*
 *									*
 * 	Larry Palmer 15-Jan-88						*
 *		Final 43bsd release					*
 *									*
 *  001 - Larry Cohen 3/6/85  - add LOOPBACK flag to indicate that 	*
 *		interface is in loopback mode.				*
 *									*
 *	Larry Cohen  -	09/16/85					*
 * 		Add 43bsd alpha tape changes for subnet routing		*
 *									*
 *	Ursula Sinkewic - 03/05/86					*
 *		Added bscintrq for BISYNC 2780/3780			*
 *									*
 *      Ed Ferris - 4/18/86						*
 * 		Add point to point state info for DECnet.		*
 ***********************************************************************/

#ifndef KERNEL
#include <sys/smp_lock.h>
#else
#include "../h/smp_lock.h"  /* 8.4.88.us */
#endif
/*
 * Structures defining a network interface, providing a packet
 * transport mechanism (ala level 0 of the PUP protocols).
 *
 * Each interface accepts output datagrams of a specified maximum
 * length, and provides higher level routines with input datagrams
 * received from its medium.
 *
 * Output occurs when the routine if_output is called, with three parameters:
 *	(*ifp->if_output)(ifp, m, dst)
 * Here m is the mbuf chain to be sent and dst is the destination address.
 * The output routine encapsulates the supplied datagram if necessary,
 * and then transmits it on its medium.
 *
 * On input, each interface unwraps the data received by it, and either
 * places it on the input queue of a internetwork datagram routine
 * and posts the associated software interrupt, or passes the datagram to a raw
 * packet input routine.
 *
 * Routines exist for locating interfaces by their addresses
 * or for locating a interface on a certain network, as well as more general
 * routing and gateway routines maintaining information used to locate
 * interfaces.  These routines live in the files if.c and route.c
 */

/*
 * Structure defining a queue for a network interface.
 *
 * (Would like to call this struct ``if'', but C isn't PL/1.)
 *
 * EVENTUALLY PURGE if_net AND if_host FROM STRUCTURE
 */
#define	IFVERLEN	512
struct ifnet {
	char	*if_name;		/* name, e.g. ``en'' or ``lo'' */
	short	if_unit;		/* sub-unit for lower level driver */
	short	if_mtu;			/* maximum transmission unit */
#ifdef old
	int	if_net;			/* network number of interface */
#endif
	short	if_flags;		/* up/down, broadcast, etc. */
	short	if_timer;		/* time 'til if_watchdog called */
	int	if_metric;		/* routing metric (external only) */
	struct	ifaddr *if_addrlist;	/* linked list of addresses per if */
	struct	sockaddr if_addr;	/* address of interface */
	struct	ifqueue {
		struct	mbuf *ifq_head;
		struct	mbuf *ifq_tail;
		int	ifq_len;
		int	ifq_maxlen;
		int	ifq_drops;
		struct lock_t lk_ifqueue; /* SMP lock for ifqueue */
	} if_snd;			/* output queue */
/* procedure handles */
	int	(*if_init)();		/* init routine */
	int	(*if_output)();		/* output routine */
	int	(*if_ioctl)();		/* ioctl routine */
	int	(*if_reset)();		/* bus reset routine */
	int	(*if_watchdog)();	/* timer routine */
/* generic interface statistics */
	int	if_ipackets;		/* packets received on interface */
	int	if_ierrors;		/* input errors on interface */
	int	if_opackets;		/* packets sent on interface */
	int	if_oerrors;		/* output errors on interface */
	int	if_collisions;		/* collisions on csma interfaces */
/* end statistics */
	int	d_affinity;		/* 8.9.88.us  Nonsymm net devices. */
	char	if_version[IFVERLEN];	/* ptr to hardware version and rev */
	struct	ifnet *if_next;
	int	if_type;
	struct	lock_t *lk_softc;
	int	(*if_start)();		/* Start routine */
	int	if_sysid_type;		/* MOP SYSID device code */
};


/*
 * The ifaddr structure contains information about one address
 * of an interface.  They are maintained by the different address families,
 * are allocated and attached when an address is set, and are linked
 * together so all addresses for an interface can be located.
 */
struct ifaddr {
	struct	sockaddr ifa_addr;	/* address of interface */
	union {
		struct	sockaddr ifu_broadaddr;
		struct	sockaddr ifu_dstaddr;
	} ifa_ifu;
#define	ifa_broadaddr	ifa_ifu.ifu_broadaddr	/* broadcast address */
#define	ifa_dstaddr	ifa_ifu.ifu_dstaddr	/* other end of p-to-p link */
	struct	ifnet *ifa_ifp;		/* back-pointer to interface */
	struct	ifaddr *ifa_next;	/* next address for interface */
};


#define	IFF_UP		0x1		/* interface is up */
#define	IFF_BROADCAST	0x2		/* broadcast address valid */
#define	IFF_DEBUG	0x4		/* turn on debugging */
#define	IFF_LOOPBACK	0x8		/* is a loopback net */
#define	IFF_POINTOPOINT	0x10		/* interface is point-to-point link */
#define	IFF_NOTRAILERS	0x20		/* avoid use of trailers */
#define	IFF_RUNNING	0x40		/* resources allocated */
#define	IFF_NOARP	0x80		/* no address resolution protocol */
#define	IFF_CANTCHANGE	(IFF_BROADCAST | IFF_POINTOPOINT | IFF_RUNNING)
#define IFF_PROMISC	0x100		/* receive all packets */
#define IFF_ALLMULTI	0x200		/* receive all multicast packets */
#define IFF_DYNPROTO	0x400		/* support dynamic proto dispatching */ 
#define IFF_MOP		0x800		/* device in MOP mode */
#define IFF_OACTIVE	0x1000		/* Device outputting */
#define IFF_802HDR	0x2000		/* 802 encapsulation */
#define	IFF_PFCOPYALL	0x4000		/* pfilt gets packets to this host */

/* interface types for benefit of parsing media address headers */
#define IFT_OTHER	0x1		/* none of the following */
#define IFT_1822	0x2		/* old-style arpanet imp */
#define IFT_HDH1822	0x3		/* HDH arpanet imp */
#define IFT_X25DDN	0x4		/* x25 to imp */
#define IFT_X25		0x5		/* PDN X25 interface */
#define	IFT_ETHER	0x6		/* Ethernet I or II */
#define	IFT_ISO88023	0x7		/* CMSA CD */
#define	IFT_ISO88024	0x8		/* Token Bus */
#define	IFT_ISO88025	0x9		/* Token Ring */
#define	IFT_ISO88026	0xa		/* MAN */
#define	IFT_STARLAN	0xb
#define	IFT_P10		0xc		/* Proteon 10MBit ring */
#define	IFT_P80		0xd		/* Proteon 10MBit ring */
#define IFT_HY		0xe		/* Hyperchannel */
#define IFT_FDDI	0xf
#define IFT_LAPB	0x10
#define IFT_SDLC	0x11
#define IFT_T1		0x12
#define IFT_CEPT	0x13
#define IFT_ISDNBASIC	0x14
#define IFT_ISDNPRIMARY	0x15
#define IFT_PTPSERIAL	0x16
#define	IFT_LOOP	0x18		/* loopback */
#define IFT_EON		0x19		/* ISO over IP */
#define	IFT_XETHER	0x1a		/* obsolete 3MB experimental ethernet */
#define	IFT_NSIP	0x1b		/* XNS over IP */
#define	IFT_SLIP	0x1c		/* IP over generic TTY */

/*
 * Output queues (ifp->if_snd) and internetwork datagram level (pup level 1)
 * input routines have queues of messages stored on ifqueue structures
 * (defined above).  Entries are added to and deleted from these structures
 * by these macros, which should be called with ipl raised to splimp().
 */
#define	IF_QFULL(ifq)		((ifq)->ifq_len >= (ifq)->ifq_maxlen)
#define	IF_DROP(ifq)		((ifq)->ifq_drops++)
#define	IF_ENQUEUE(ifq, m) { \
	(m)->m_act = 0; \
	if ((ifq)->ifq_tail == 0) \
		(ifq)->ifq_head = m; \
	else \
		(ifq)->ifq_tail->m_act = m; \
	(ifq)->ifq_tail = m; \
	(ifq)->ifq_len++; \
}
#define	IF_ENQUEUEIF(ifq, m, ifp) { \
	(m)->m_act = 0; \
	if ((ifq)->ifq_tail == 0) \
		(ifq)->ifq_head = m; \
	else \
		(ifq)->ifq_tail->m_act = m; \
	(ifq)->ifq_tail = m; \
	(ifq)->ifq_len++; \
	(m)->m_ifp = ifp; \
}
#define	IF_PREPEND(ifq, m) { \
	(m)->m_act = (ifq)->ifq_head; \
	if ((ifq)->ifq_tail == 0) \
		(ifq)->ifq_tail = (m); \
	(ifq)->ifq_head = (m); \
	(ifq)->ifq_len++; \
}

/*
 * Packets destined for level-1 protocol input routines
 * have a pointer to the receiving interface prepended to the data.
 * IF_DEQUEUEIF extracts and returns this pointer when dequeueing the packet.
 * IF_ADJ should be used otherwise to adjust for its presence.
 */
#ifdef BSD43
#define	IF_ADJ(m) { \
	(m)->m_off += sizeof(struct ifnet *); \
	(m)->m_len -= sizeof(struct ifnet *); \
	if ((m)->m_len == 0) { \
		struct mbuf *n; \
		MFREE((m), n); \
		(m) = n; \
	} \
}
#else
/* ifnet is held someplace else */
#define	IF_ADJ(m) { \
	if ((m)->m_len == 0) { \
		struct mbuf *n; \
		MFREE((m), n); \
		(m) = n; \
	} \
}
#endif
#define	IF_DEQUEUEIF(ifq, m, ifp) { \
	(m) = (ifq)->ifq_head; \
	if (m) { \
		if (((ifq)->ifq_head = (m)->m_act) == 0) \
			(ifq)->ifq_tail = 0; \
		(m)->m_act = 0; \
		(ifq)->ifq_len--; \
		(ifp) = (m)->m_ifp; \
	} \
}
#define	IF_DEQUEUE(ifq, m) { \
	(m) = (ifq)->ifq_head; \
	if (m) { \
		if (((ifq)->ifq_head = (m)->m_act) == 0) \
			(ifq)->ifq_tail = 0; \
		(m)->m_act = 0; \
		(ifq)->ifq_len--; \
	} \
}

#define	IFQ_MAXLEN	50
#define	IFNET_SLOWHZ	1		/* granularity is 1 second */

/*
 * Interface request structure used for socket
 * ioctl's.  All interface ioctl's must have parameter
 * definitions which begin with ifr_name.  The
 * remainder may be interface specific.
 */
struct	ifreq {
#define	IFNAMSIZ	16
	char	ifr_name[IFNAMSIZ];		/* if name, e.g. "en0" */
	union {
		struct	sockaddr ifru_addr;
		struct	sockaddr ifru_dstaddr;
		struct	sockaddr ifru_broadaddr;
		short	ifru_flags;
		int	ifru_metric;
		caddr_t	ifru_data;
	} ifr_ifru;
#define	ifr_addr	ifr_ifru.ifru_addr	/* address */
#define	ifr_dstaddr	ifr_ifru.ifru_dstaddr	/* other end of p-to-p link */
#define	ifr_broadaddr	ifr_ifru.ifru_broadaddr	/* broadcast address */
#define	ifr_flags	ifr_ifru.ifru_flags	/* flags */
#define ifr_metric	ifr_ifru.ifru_metric	/* metric */
#define	ifr_data	ifr_ifru.ifru_data	/* for use by interface */
};


/*
 * structure used to query de and qe for physical addresses
 */
struct ifdevea {
        char    ifr_name[IFNAMSIZ];             /* if name, e.g. "en0" */
        u_char default_pa[6];                   /* default hardware address */
        u_char current_pa[6];                   /* current physical address */
};

/*
 * structure used in SIOCSTATE request to set device state and ownership
 */
#define	IFMAXENTITY	5

struct ifstate {
	char    ifr_name[IFNAMSIZ];		/* if name, e.g. "en0" */
	u_short	if_family;			/* current family ownership */
	u_short	if_next_family;			/* next family ownership */
	u_short	if_mode:3,			/* mode of device */
		if_ustate:1,			/* user requested state */
		if_nomuxhdr:1,			/* if set, omit mux header */
		if_dstate:4,			/* current state of device */
		if_xferctl:1,			/* xfer control to nxt family */
		if_rdstate:1,			/* read current state */
		if_wrstate:1,			/* set current state */
		if_reserved:4;
	u_short if_dataportstate:3,		/* data port entity states */
	        if_allocate:1,			/* allocate port */
	        if_deallocate:1,		/* deallocate port */
	        if_start:1,			/* start port */
	        if_stop:1,			/* stop port */
	        if_txabort:1,			/* transmit abort */
	        if_setframe:1,			/* set protocol framing & CRC */
	        if_frametype:3,			/* protocol framing types */
	        if_errorchecktype:3,		/* error check types */
		if_setrxbuffsize:1;		/* set receive buffer size */
	struct protosw *if_pr;			/* client protocol module */
	int     if_rxbuffsize;			/* receive buffer size */
	int     if_lec[IFMAXENTITY];		/* DNA Local Entity Class */
	u_char  *if_lei;			/* DNA Local Entity Instance */
	u_short if_leisiz;			/* size of Local Entity Instance */
	caddr_t if_clientname;                  /* client local Entity Name */
	caddr_t if_clientref;			/* client handle */
};

#define	IFS_USROFF	0x0	/* user request to stop device */
#define	IFS_USRON	0x1	/* user request to start device */
#define	IFS_DDCMPFDX	0x0	/* operate in DDCMP full duplex */
#define	IFS_MOP		0x1	/* operate in MOP */
#define	IFS_DDCMPHDXP	0x2	/* operate in DDCMP half duplex, primary */
#define	IFS_DDCMPHDXS	0x3	/* operate in DDCMP half duplex, secondary */
#define IFS_NOMUXHDR	0x1	/* do not multiplex on point to point line */
#define IFS_MUXHDR	0x0	/* multiplex on point to point line */
#define IFS_HALTED	0x0	/* device state = halted */
#define IFS_STARTING	0x1	/* device state = starting */
#define IFS_RUNNING	0x3	/* device state = running */
#define IFS_HALTING	0x4	/* device state = halting */
#define IFS_OWNREQ	0x5	/* device state = ownership requested */
#define IFS_OWNREL	0x6	/* device state = ownership released */
#define IFS_ENTEREDMOP	0x7	/* device state = mop */
#define IFS_XFERCTL	0x1	/* xfer control to next family */
#define IFS_RDSTATE	0x1	/* read device state */
#define IFS_WRSTATE	0x1	/* set device state */

#define IFS_OPEN		0x0
#define IFS_OPENDISABLED	0x1	/* Data Port sub-entity states */
#define IFS_CALLATTACHED	0x2
#define IFS_CLOSEPENDING	0x3

#define IFS_ALLOCATE		0x1	/* effectively PhOpenPort */
#define IFS_DEALLOCATE		0x1	/* effectively PhClosePort */
#define IFS_STARTPORT		0x1	/* effectively GetPhPort */
#define IFS_STOPPORT		0x1	/* effectively ReleasePhPort */
#define IFS_TXABORT		0x1
#define IFS_SETFRAME		0x1
#define IFS_SETRXBUFFSIZE	0x1

#define IFS_HDLC		0x0
#define IFS_SDLC		0x1
#define IFS_DDCMP		0x2	/* protocol framing types */
#define IFS_BISYNC		0x3
#define IFS_GENBYTE		0x4

/* error check types - from DSV11 manual */
#define IFS_CRC_CCITT_1		0x0	/* CRC-CCITT preset to all 1s */
#define IFS_CRC_CCITT_0		0x1	/* CRC-CCITT preset to all 0s */
#define IFS_LRC_VRC_ODD		0x2	/* LRC/VRC odd */
#define IFS_CRC_16		0x3	/* CRC-16 */
#define IFS_VRC_ODD		0x4	/* VRC odd */        
#define IFS_VRC_EVEN		0x5	/* VRC even */
#define IFS_LRC_VRC_EVEN	0x6	/* LRC/VRC even */
#define IFS_CRC_NONE		0x7	/* no error control */

struct ifdata {
	u_char data[128];
};

/* Structure used in SIOCGIFCONF request.
 * Used to retrieve interface configuration
 * for machine (useful for programs which
 * must know all networks accessible).
 */
struct	ifconf {
	int	ifc_len;		/* size of associated buffer */
	union {
		caddr_t	ifcu_buf;
		struct	ifreq *ifcu_req;
	} ifc_ifcu;
#define	ifc_buf	ifc_ifcu.ifcu_buf	/* buffer address */
#define	ifc_req	ifc_ifcu.ifcu_req	/* array of structures returned */
};

/*
 * Structure for EEPROM downline upgrades. (Supported by FDDI adapters)
 */
struct ifeeprom {
	char	ife_name[IFNAMSIZ];		/* if name, e.g. "fza0" */
	u_char	ife_data[64];			/* block of EEPROM data */
	u_long	ife_offset;			/* byte offset from base */
	u_long	ife_blklen;			/* len of EEPROM block */
	u_long	ife_lastblk;			/* True if last block */
};
#define IFE_NOTLAST     0x0     /* Intermediary block of EEPROM image */
#define IFE_LASTBLOCK   0x1     /* Last block of EEPROM image */

#define IFE_SUCCESS     0x0     /* Block of EEPROM successfully written */
#define IFE_RETRY       0x1     /* Retry last block written */
#define IFE_FAIL        0x2     /* Fail entire EEPROM image write sequence */

/*
 * Structure to return module version information
 */

/*
 * interface statistics structures
 */
struct estat {				/* Ethernet interface statistics */
	u_short	est_seconds;		/* seconds since last zeroed */
	u_int	est_bytercvd;		/* bytes received */
	u_int	est_bytesent;		/* bytes sent */
	u_int	est_blokrcvd;		/* data blocks received */
	u_int	est_bloksent;		/* data blocks sent */
	u_int	est_mbytercvd;		/* multicast bytes received */
	u_int	est_mblokrcvd;		/* multicast blocks received */
	u_int	est_deferred;		/* blocks sent, initially deferred */
	u_int	est_single;		/* blocks sent, single collision */
	u_int	est_multiple;		/* blocks sent, multiple collisions */
	u_short	est_sendfail_bm;	/*	0 - Excessive collisions */
					/*	1 - Carrier check failed */
					/*	2 - Short circuit */
					/*	3 - Open circuit */
					/*	4 - Frame too long */
					/*	5 - Remote failure to defer */
	u_short	est_sendfail;		/* send failures: (bit map)*/
	u_short	est_collis;		/* Collision detect check failure */
	u_short	est_recvfail_bm;	/*	0 - Block check error */
					/*	1 - Framing error */
					/*	2 - Frame too long */
	u_short	est_recvfail;		/* receive failure: (bit map) */
	u_short	est_unrecog;		/* unrecognized frame destination */
	u_short	est_overrun;		/* data overrun */
	u_short	est_sysbuf;		/* system buffer unavailable */
	u_short	est_userbuf;		/* user buffer unavailable */
	u_int	est_mbytesent;		/* multicast bytes sent */
	u_int	est_mbloksent;		/* multicast blocks sent */
};

struct dstat {				/* DDCMP pt-to-pt interface statistics */
	u_short	dst_seconds;		/* seconds since last zeroed */
	u_int	dst_bytercvd;		/* bytes received */
	u_int	dst_bytesent;		/* bytes sent */
	u_int	dst_blockrcvd;		/* data blocks received */
	u_int	dst_blocksent;		/* data blocks sent */
	u_short	dst_inbound_bm;		/*	0 - NAKs sent, header crc */
					/*	1 - NAKs sent, data crc */
					/*	2 - NAKs sent, REP response */
	u_char	dst_inbound;		/* data errors inbound: (bit map) */
	u_short	dst_outbound_bm;	/*	0 - NAKs rcvd, header crc */
					/*	1 - NAKs rcvd, data crc */
					/*	2 - NAKs rcvd, REP response */
	u_char	dst_outbound;		/* data errors outbound: (bit map) */
	u_char	dst_remotetmo;		/* remote reply timeouts */
	u_char	dst_localtmo;		/* local reply timeouts */
	u_short	dst_remotebuf_bm;	/*	0 - NAKs rcvd, buffer unavailable */
					/*	1 - NAKs rcvd, buffer too small */
	u_char	dst_remotebuf;		/* remote buffer errors: (bit map) */
	u_short	dst_localbuf_bm;	/*	0 - NAKs sent, buffer unavailable */
					/*	1 - NAKs sent, buffer too small */
	u_char	dst_localbuf;		/* local buffer errors: (bit map) */
	u_char	dst_select;		/* selection intervals elapsed */
	u_short	dst_selecttmo_bm;	/*	0 - No reply to select */
					/*	1 - Incomplete reply to select */
	u_char	dst_selecttmo;		/* selection timeouts: (bit map) */
	u_short	dst_remotesta_bm;	/*	0 - NAKs rcvd, receive overrun */
					/*	1 - NAKs sent, header format */
					/*	2 - Select address errors
					/*	3 - Streaming tributaries */
	u_char	dst_remotesta;		/* remote station errors: (bit map) */
	u_short	dst_localsta_bm;	/*	0 - NAKs sent, receive overrun */
					/*	1 - Receive overrun, NAK not sent */
					/*	2 - Transmit underruns */
					/*	3 - NAKs rcvd, header format */
	u_char	dst_localsta;		/* local station errors: (bit map) */
};


struct fstat {
        u_short fst_second;             /* seconds since last zeroed */
	u_int	fst_frame;		/* total number of frames seen */ 
	u_int	fst_error;		/* MAC counter frame error */
	u_int 	fst_lost;		/* MAC counter frame count */
        u_int   fst_bytercvd;           /* bytes received */
        u_int   fst_bytesent;           /* bytes sent */
        u_int   fst_pdurcvd;            /* data blocks received */
        u_int   fst_pdusent;            /* data blocks sent */
        u_int   fst_mbytercvd;          /* multicast bytes received */
	u_int   fst_mpdurcvd;           /* multicast blocks received */
        u_int   fst_mbytesent;          /* multicast bytes sent */
        u_int   fst_mpdusent;           /* multicast blocks sent */
	u_short	fst_underrun;		/* transmit underrun error */
        u_short fst_sendfail;           /* sent failures: (bit map)*/
        u_short	fst_fcserror;		/* FCS check failure */
	u_short	fst_fseerror;		/* frame status error */
	u_short	fst_pdualig;		/* frame alignment error */
	u_short	fst_pdulen;		/* frame length error */
	u_short	fst_pduunrecog;		/* frame unrecognized */
	u_short fst_mpduunrecog;	/* multicast frame unrecognized */
        u_short fst_overrun;            /* data overrun */
        u_short fst_sysbuf;             /* system buffer unavailable */
        u_short fst_userbuf;            /* user buffer unavailable */
	u_short fst_ringinit;		/* other station ring reinit. intiated*/
	u_short fst_ringinitrcv;	/* ring reinitialization initiated */
	u_short fst_ringbeacon;		/* ring beacon process initiated */
	u_short fst_duptoken;		/* duplicat token detected */
	u_short fst_dupaddfail;		/* duplicate address test failures */
	u_short	fst_ringpurge;		/* ring purge errors */
	u_short fst_bridgestrip;	/* bridge strip errors */
	u_short fst_traceinit;		/* traces initiated */
	u_short fst_tracerecv;		/* traces received */
	u_short fst_lem_rej;		/* LEM reject count */
	u_short fst_lem_events;		/* LEM events count */
	u_short fst_lct_rej;		/* LCT reject count */
	u_short fst_tne_exp_rej;	/* TNE expired reject count */
	u_short fst_connection;		/* Completed Connection count */
	u_short fst_ebf_error;		/* Elasticity Buffer Errors */
};


/* 
 * FDDI MIB structures 
 */

/* SMT group */

struct fddismt {
	u_char	smt_stationid[8];	/* FDDI station id */ 
	u_short	smt_opversionid;	/* operation version id */
	u_short	smt_hiversionid;	/* highest version id */
	u_short	smt_loversionid;	/* lowest version id */
	short 	smt_macct;		/* number of MACs */
	short	smt_nonmasterct;	/* number of non master port */
	short	smt_masterct;		/* number of master port */
	short   smt_pathsavail;		/* available path type */
	short	smt_configcap;		/* configuration capabilities */
	short	smt_configpolicy;	/* configuration policy */
	u_short	smt_connectpolicy;	/* connection policy */
	u_short	smt_timenotify;		/* neighbor notification prot. time*/
	short	smt_statusreport;	/* status reporting protocol */
	short	smt_ecmstate;		/* state of ECM state machine */
	short	smt_cfstate;		/* CF_state */
	short	smt_holdstate;		/* current state of Hold function */
	short	smt_remotedisconn;	/* remotely disconnection flag */ 
	u_char	smt_msgtimestamp[8];	/* timestamp for SMT frames */
	u_char	smt_trantimestamp[8];	/* timestamp for last event */
	u_char	smt_setinfo[12];	/* station last parameter change */
	u_char	smt_lastsetid[8];	/* Last station ID change */
}; 	
		
struct fstatus {
	short	led_state;		/* LED State */
	short   rmt_state;		/* RMT state */
	short   link_state;		/* LINK state */
	short   dup_add_test;		/* duplicate address test */
	short   ring_purge_state;	/* ring purge state */
	u_long  neg_trt;		/* Negotiated TRT */
	u_char  upstream[6];		/* Upstream Neighbor */
	u_char	una_timed_out;		/* una timed out flag */
	u_char 	frame_strip_mode;	/* frame strip mode */
	u_char	claim_token_mode;	/* claim token yield mode */
	u_char 	phy_state;		/* Physical state */
	u_char	neighbor_phy_type;	/* Neighborshort */		   
	u_char  rej_reason;		/* reject reason */
	u_int	phy_link_error;		/* phy link error estimate */
	u_char	ring_error;		/* ring error reasons */
	u_long	t_req;			/* TRT request */
	u_long  tvx;			/* TVX value */
	u_long  t_max;			/* TRT maximum */
        u_long  lem_threshold;          /* lem threshold */
	u_char  mla[6];			/* station physical address */
	u_char  fw_rev[4];		/* firmware revision */
	u_char  phy_rev[4];		/* ROM revision */	
	u_char	pmd_type;		/* PMD type */
	u_char  dir_beacon[6];		/* Last Direct Beacon address */
	short	smt_version;		/* SMT version */
	short   state;			/* Adapter State */
};	

	
/* MAC group */

struct fddimac {
	short	mac_index;		/* MAC index */
	short	mac_fsc;		/* MAC frame status capabilities */
	u_long	mac_gltmax;		/* Greastest lower bound of T_max */
	u_long	mac_gltvx;		/* Greastest lower bound of TVX */
	short	mac_paths;		/* path types available */
	short	mac_current;		/* association of the MAC with path*/
	u_char	mac_upstream[6];	/* upstream neighbor */
	u_char	mac_oldupstream[6];	/* previous upstream neighbor */
	short	mac_dupaddrtest;	/* duplicate address test */
	short   mac_pathsreq;		/* paths requested */
	short	mac_downstreamtype;	/* downstream PC-type */
	u_char	mac_smtaddress[6];	/* MAC address for SMT */
	u_long	mac_treq;		/* TRT time */
	u_long	mac_tneg;		/* Neg. TRT time */
	u_long	mac_tmax;		/* max. TRT time */
	u_long	mac_tvx;		/* TVX value */
	u_long	mac_tmin;		/* min. TRT time */
	short	mac_framestatus;	/* current frame status */
	int	mac_counter;		/* frame counters */
	int	mac_error;		/* frame error counters */
	int	mac_lost;		/* frame lost counter */
	short	mac_rmtstate;		/* Ring Management state */
	short	mac_dupaddr;		/* duplicate address flag */
	short	mac_condition;		/* MAC condition */
};

/* PATH group */

struct fddipath {
	short	path_configindex;	/* path configuration index */
	short	path_type;		
	short	path_portorder;			
	u_long	path_sba;		/*synchronous bandwidth allocation*/ 
	short	path_sbaoverhead;	/* SBA overhead */
	short	path_status;		/* path status */
};

/* PORT group */

struct fddiport {
	short	port_index;		/* port index */
	short	port_pctype;		/* value of the port's PC_type */
	short	port_pcneighbor;	/* PC_neighbor of remote port*/
	short   port_connpolicy;	/* connection policies */
	short	port_remoteind;		/* remote MAC indicated */
	short 	port_CEstate;		/* Current Configuration state */
	short   port_pathreq;		/* path request */
	u_short port_placement;		/* upstream MAC */
	short	port_availpaths;	/* available paths */
	u_int	port_looptime;		/* time for MAC loval loop */
	u_long	port_TBmax;		/* TB_max */
	short	port_BSflag;		/* the Break state, BF_flag */
	u_int	port_LCTfail;		/* counter for Link confidence test */
	short	port_LerrEst;		/* Link error estimate */
	u_int	port_Lemreject;		/* Link reject count */
	u_int	port_Lem;		/* Link error monitor count */
	short	port_baseLerEst;	/* value of port Ler Estimate */
	u_int	port_baseLerrej;	/* Ler reject count */
	u_int	port_baseLer;		/* Ler count */
	u_char	port_baseLerTime[8];	/* Ler timestamp */	
	short   port_Lercutoff;		/* error rate cut off limit */
	short	port_alarm;		/* error rate cause alarm generate*/
	short	port_connectstate;	/* connect state */
	short   port_PCMstate;		/* PCM state */
	short	port_PCwithhold;	/* PC_withhold */
	short	port_Lercondition;	/* true if Ler-Est <= Ler_alarm */
};

/* Attachment group */

struct	fddiatta {
	short 	atta_index;		/* attachment index */
	short	atta_class;		/* attachment class */
	short	atta_bypass;		
};
	

/*
 * interface counter ioctl request
 */
struct ctrreq {
	char	ctr_name[IFNAMSIZ];	/* if name */
	char	ctr_type;		/* type of interface */
	union {
		struct estat ctrc_ether;/* ethernet counters */
		struct dstat ctrc_ddcmp;/* DDCMP pt-to-pt counters */
		struct fstat ctrc_fddi; /* FDDI counters */
		struct fstatus status_fddi; /* FDDI stsatus */
		struct fddismt smt_fddi;/* fddi SMT attributes */ 
		struct fddimac mac_fddi;/* fddi MAC attributes */
		struct fddipath path_fddi; /* fddi PATH attributes */
		struct fddiport port_fddi; /* fddi PORT attributes */
		struct fddiatta atta_fddi; /* fddi attatch attributes */ 
	} ctr_ctrs;
};
#define CTR_ETHER 0			/* Ethernet interface */
#define CTR_DDCMP 1			/* DDCMP pt-to-pt interface */
#define CTR_FDDI 2			/* FDDI interface */
#define FDDIMIB_SMT	3		/* FDDI MIB SMT group */
#define FDDIMIB_MAC	4		/* FDDI MIB MAC group */
#define FDDIMIB_PATH	5		/* FDDI MIB PATH group */
#define FDDIMIB_PORT	6		/* FDDI MIB PORT group */
#define FDDIMIB_ATTA	7		/* FDDI MIB Attatchment Group */
#define FDDI_STATUS	8		/* FDDI status */
#define ctr_ether ctr_ctrs.ctrc_ether
#define ctr_ddcmp ctr_ctrs.ctrc_ddcmp
#define ctr_fddi  ctr_ctrs.ctrc_fddi
#define sts_fddi  ctr_ctrs.status_fddi
#define fmib_smt  ctr_ctrs.smt_fddi
#define fmib_mac  ctr_ctrs.mac_fddi
#define fmib_path  ctr_ctrs.path_fddi
#define fmib_port  ctr_ctrs.port_fddi
#define fmib_atta  ctr_ctrs.atta_fddi

#define CTR_HDRCRC	0		/* header crc bit index */
#define CTR_DATCRC	1		/* data crc bit index */
#define CTR_BUFUNAVAIL	0		/* buffer unavailable bit index */

#ifdef KERNEL
#include "../net/net/if_arp.h"
struct	ifnet *ifnet;
struct	ifnet *if_ifwithaddr(), *if_ifwithnet(), *if_ifwithaf();
struct	ifnet *if_ifonnetof();
#ifdef INET
struct	ifqueue	ipintrq;		/* ip packet input queue */
#endif
#ifdef BSC
struct	ifqueue	bscintrq;		/* BISYNC packet input queue */
#endif
struct	ifqueue rawintrq;		/* raw packet input queue */
struct	in_addr if_makeaddr();
struct	ifaddr *ifa_ifwithaddr(), *ifa_ifwithnet(), *ifa_ifwithaf();
#else /* KERNEL */
#include <net/if_arp.h>
#endif
