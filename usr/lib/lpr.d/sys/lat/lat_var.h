
#ifndef lint
/* sccsid  =  @(#)lat_var.h	6.2	(ULTRIX)	1/28/88; */
#endif lint
 
 
/************************************************************************
 *									*
 *			Copyright (c) 1984 by				*
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
/*	lat_var.h	0.0	11/9/84	*/
/*	lat_var.h	2.0	4/15/86 */
 
/************************************************************************
 *			Modification History				*
 *									*
 *	Chung Wong - 8/24/87						*
 *		Changed LAT_MAXVC and LAT_MAXSLOTS from 64 to 256.      *
 *									*
 *	Peter Harbo - 4/15/86						*
 *		Addition of symbols for command message timeouts, 	*
 *		and command, solicit retry limits, timeout values.      *
 *		Also new members of sclass structure (new fn ptrs).	*
 *		ttytounit() macro for converting from tty to unit.	*
 *		ECB_IN_USE added for ecb structure.			*
 ************************************************************************/
 
#define MAXCLASS	1	/* max class supported */
 
#define MINASIZE	255	/* minimum attention slot size */
#define MINDSIZE	255	/* minimum data slot size */
 
#define LAT_MAXVC	256	/* max # of virual circuits supported */
#define LAT_MAXSLOTS	256	/* max # of slots in the system */
 
#define LAT_MTIMER	20	/* multicast timer interval */
#define LAT_XTIMER	(1*2)	/* transmit timer */
#define LAT_XRETRY	60	/* retry count */
#define LAT_RTIMER	(1*2)	/* resource recovery timer */
#define LAT_CMDTIMEOUT	5	/* timeout for TS response to command msgs */
#define LAT_STATTIMEOUT 65	/* timeout for TS status messages */
#define LAT_CMDRETRY	2	/* retry limit for command message */
 
 
#define LAT_RESPTIMER	(1*2)	/* response timer */
#define LATOFFSET	14	/* leave space for Ethernet header */
 
#define LAT_SOLXMIT	2	/* retry limit for solicit info msg */
#define LAT_SOLTIMER	(1*6)	/* response timer */
 
/*
 * Return pointer to end of data in MBUF.
 */
#define mtoe(x,t)       ((t)(((x)->m_off > MSIZE && (x)->m_cltype == M_CLTYPE1) ? (int)(x)->m_clptr + (x)->m_len : (int)(x) + (x)->m_off + (x)->m_len))

 
/*
 * Increment a counter, latching if necessary.
 */
#define INC(o) if (latctrs.o != (unsigned)0xffffffff) latctrs.o++
 
struct lataddr			/* generic LAT Ethernet address */
    {
	u_short	lat_family;	/* address family (AF_LAT) */
	u_char	lat_addr[6];	/* Ethernet address */
    };
 
struct latrecv			/* received message prefix */
    {
	struct ether_header rcv_hdr; /* ethernet header */
	struct ifnet *rcv_ifp;	/* received interface descriptor */
    };
 
struct que			/* general queue header */
    {
	struct mbuf *q_head;	/* queue head pointer */
	struct mbuf *q_tail;	/* queue tail pointer */
    };
 
#define ENQUEUE(q, m) \
    { \
	(m)->m_act = 0; \
	if ((q)->q_tail == 0) \
	    (q)->q_head = m; \
	else \
	    (q)->q_tail->m_act = m; \
	(q)->q_tail = m; \
    }
 
#define DEQUEUE(q, m) \
    { \
	(m) = (q)->q_head; \
	if (m) \
	{ \
	    if (((q)->q_head = (m)->m_act) == 0) \
		(q)->q_tail = 0; \
	    (m)->m_act = 0; \
	} \
    }
 
struct lat_slot			/* LAT slot descriptor */
    {
	struct lat_vc *lsl_vc;	/* virtual circuit for this slot */
	u_char	lsl_class;	/* service class for this slot */
	u_char	lsl_state:3,	/* slot state */
		lsl_bslot:1,	/* flag to send a data_b slot */
		lsl_reason:4;	/* reason code if rejecting/stopping slot */
	u_char	lsl_locid;	/* local slot id */
	u_char	lsl_remid;	/* remote slot id */
	u_char	lsl_remcredits;	/* credits extended to remote */
	u_char	lsl_loccredits;	/* credits extended to local */
	u_char	lsl_attsize;	/* minimum attention slot size */
	u_char	lsl_datasize;	/* minimum data slot size */
	struct sclass *lsl_scl;	/* service class descriptor */
	caddr_t	lsl_data;	/* class dependant data */
    };
#define SST_FREE	0	/* slot is available */
#define SST_REJECT	1	/* slot is being rejected */
#define SST_STARTING	2	/* slot is starting up */
#define SST_RUNNING	3	/* slot is running */
#define SST_STOP	4	/* slot is being stopped */
 
struct lat_vc			/* LAT vitual circuit descriptor */
    {
	struct ifnet *lvc_if;	/* Ethernet interface to use */
	struct lataddr lvc_addr;/* Ethernet address for virtual circuit */
	u_short	lvc_locid;	/* local circuit id */
	u_short	lvc_remid;	/* remote circuit id */
	u_short	lvc_dgsize;	/* max datagram size to send */
	u_short	lvc_timer;	/* retransmission timer */
	u_char	lvc_counter;	/* retransmission counter */
	u_char	lvc_resource;	/* resource recovery timer */
	u_char	lvc_kalive;	/* server keep-alive timer */
	u_char	lvc_state;	/* virtual circuit state */
	u_char	lvc_reason;	/* stop reason code */
	u_char	lvc_act;	/* number of active slots on circuit */
	u_char	lvc_rrf;	/* rrf flag for next run message */
	u_char	lvc_rcvact;	/* receive active on circuit */
	u_char	lvc_nxmt;	/* next message number to transmit */
	u_char	lvc_ack;	/* highest message number received */
	u_char	lvc_lxmt;	/* lowest unack'd message number sent */
	u_char	lvc_hxmt;	/* highest unack'd message number sent */
	struct que lvc_xmtq;	/* pending transmit queue */
	struct que lvc_ackq;	/* pending ack queue */
    };
#define VST_HALTED	0	/* virtual circuit not in use */
#define VST_STARTING	1	/* virtual circuit starting */
#define VST_RUNNING	2	/* virtual circuit running */
#define VST_STOPING	3	/* virtual circuit stopping */
 
/*
 * Service class descriptor
 */
struct sclass
    {
	u_char	scl_state;		/* class state */
	int	(*scl_direct)();	/* directory service routine */
	int	(*scl_solicit)();	/* solicit info message */
	int	(*scl_response)();	/* response info message */
	int	(*scl_new)();		/* process new slot request */
	int	(*scl_rdataa)();	/* process received data_a slot */
	int	(*scl_rdatab)();	/* process received data_b slot */
	int	(*scl_rother)();	/* process other received slot */
	int	(*scl_sdataa)();	/* build class dependent data_a slot */
	int	(*scl_sdatab)();	/* build class dependent data_b slot */
	int	(*scl_sother)();	/* build class dependent other slot */
	int	(*scl_hangup)();	/* hangup the device */
	struct mbuf *scl_dmsg;		/* Ptr to cls specific directory msg */
	struct que scl_smsg;		/* Ptr to cls specific sol msg */
	struct que scl_rmsg;		/* Ptr to cls specific resp msg */
    };
 
#define CHA_NODGRP	0x1	/* node group codes changed */
#define CHA_NODESC	0x2	/* node descriptor changed */
#define CHA_SRVNAM	0x4	/* service names changed */
#define CHA_SRVRAT	0x8	/* service ratings changed */
#define CHA_SRVDES	0x10	/* service descriptors changed */
#define CHA_SRVCLS	0x20	/* service classes changed */
#define CHA_OTHER	0x80	/* other parameters changed */
 
#define ECB_INUSE 	   1    /* Port in use */
 
 
 
 
 
 
 
 
 
 
 

