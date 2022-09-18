/*	@(#)csmacd_if.h	4.1	(ULTRIX)	9/4/90	*/

/************************************************************************
 *									*
 *			Copyright (c) 1988 by				*
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
 * 1.00 03-Aug-1988
 *		DECnet-Ultrix	V3.x
 *
 * 			- created
 *
 *
 */

/* CSMACD Interface Literals */
#define CSMACD_PROTO	1		/* Protocol Number for CSMACD Mod */

/* Literals for Kernel Interface Commands */
#define CSMACD_OPENPORT		0
#define CSMACD_CLOSEPORT	1
#define CSMACD_ADD802		2
#define CSMACD_STRIP802		3
#define CSMACD_MAXDATA		4
#define CSMACD_INCOUT		5
#define CSMACD_DISMCAST		6
#define CSMACD_ENAMCAST		7
#define CSMACD_DISSAP		8
#define CSMACD_ENASAP		9
#define CSMACD_DISPTYP		10
#define CSMACD_ENAPTYP		11

/* Literals denoting data link frame types */
#define CSMACD_ETHERNET		0
#define CSMACD_RESERVED		1
#define CSMACD_802		2

/* Data Structures applicable to the DLI/CSMACD Kernel Interface */

/*
 *	Open Port Parameters Structure
 */
#define USER_SUPPLIED 	0x1		/* llc mode */ 
#define CLASS1_TYPE1 	0x0 		/* llc mode */
#define MAXMCAST  2
#define EASIZE    6
#define PISIZE    5
struct dli_802_3 { 
	u_short ptype; 
	u_char llc; 
	u_char selector; 
	u_char pid[PISIZE]; 
	u_char mcast[MAXMCAST][EASIZE]; 
	u_char macaddr[EASIZE];
};

struct dli_ifop { 
	struct mbuf *client;		/* DNA Local Entity client name */
	struct mbuf *portname;		/* DNA Local Entity port name */
	struct mbuf *station;		/* DNA Local Entity station name */
	struct ifnet *ifp; 		/* used if station name not given */
	struct protosw *protosw; 	/* pointer to client's protosw */
	caddr_t port; 			/* port id returned */
	u_char dltype; 			/* data link type used */
	union dli_dldef { 
		struct dli_802_3 type3; 
	} dldef; 
}; 
#define dl8023 dldef.type3 

/*
 *	Add 802 Header Parameters Structure
 */
struct dli_ifah { 
	caddr_t port; 
	u_char dsap; 
	u_char pid[PISIZE];
	struct mbuf *msg; 
};

/*
 *	Strip 802 Header Parameters Structure
 */
struct dli_ifsh { 
	struct ifnet *ifp;
	caddr_t port; 
	u_char dsap; 
	u_char ssap; 
	u_char ctlf; 
	u_char pid[PISIZE]; 
	struct mbuf *msg; 
}; 

/*
 *	Datalink Interface  Definition Structure
 */
struct dli_ifdef { 
	struct ifnet *ifp;
	caddr_t port; 
}; 


/*
 *	Increment Output Counters
 */
struct dli_ifincout { 
	caddr_t port; 
	u_int octets_sent;
	u_char mcast; 
};


/*
 *	Enable/Disable a multicast address.
 */
struct dli_ifmcast { 
	caddr_t port; 
	u_char mcast[EASIZE]; 
};

/*
 *	Enable/Disable a SAP, GSAP or SNAP Protocol ID.
 */
struct dli_ifsap { 
	caddr_t port; 
	u_char sap;
	u_char pid[PISIZE]; 
};

/*
 *	Enable/Disable an Ethernet protocol type.
 */
struct dli_ifptype { 
	caddr_t port; 
	u_short ptype; 
};
