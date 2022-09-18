#ifndef lint
/* static char *sccsid = "@(#)lat_protocol.h	4.1.1.3	2/29/88"; */
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

/************************************************************************
 *			Modification History				*
 *									*
 *      Chung Wong  - 1/7/88                                            *
 *              Increase the size of lat_ini.lin_dev to 4*MAXNAM to     *
 *              accomodate the master node name and port number.        *
 *		Added struct lat_service.                               *
 *                                                                      *
 *	Peter Harbo - 4/15/86						*
 *		For LAT 5.1: new slot error codes (STOP_SL..), new	*
 *		new message types, new structures for solicit, response *
 *		information msgs, for LIOCCMD ioctl, status msgs, 	*
 *		entry control block for connection requests		*
 *									*
 ************************************************************************/

/*	lat_protocol.h	0.0	11/9/84	 */
/*      lat_protocol.h  2.0     4/15/86  */

/*
 * LAT virtual circuit message formats.
 */
#define MAXNAM	16			/* longest service or node name */

struct vc_hdr				/* virtual circuit header */
    {
	u_char	vhd_rrf:1,		/* response requested */
		vhd_mas:1,		/* master/slave flag */
		vhd_type:6;		/* message type */
	u_char	vhd_slots;		/* number of slots present */
	u_short	vhd_dstid;		/* destination circuit id */
	u_short	vhd_srcid;		/* source circuit id */
	u_char	vhd_seq;		/* message sequence number */
	u_char	vhd_ack;		/* message acknowledgement number */
    };

struct vc_start				/* start message */
    {
	u_short	vst_dgsize;		/* minimum datagram size */
	u_char	vst_pver;		/* protocol version number */
	u_char	vst_eco;		/* protocol ECO number */
	u_char	vst_slots;		/* max # of slots/circuit */
	u_char	vst_dlbufs;		/* # of dedicated data link buffers */
	u_char	vst_stimer;		/* server circuit timer */
	u_char	vst_kalive;		/* keep alive timer */
	u_short	vst_facility;		/* facility number */
	u_short	vst_product;		/* product code */
    };

struct vc_stop				/* stop message */
    {
	u_char	vsp_reason;		/* disconnect reason code */
	u_char	vsp_reasonlen;		/* length of reason text */
    };
#define STOP_NOSLOTS	1		/* no slots connected on VC */
#define STOP_BADFORMAT	2		/* illegal msg or bad format rcvd */
#define STOP_VCHALT	3		/* circuit halted by user */
#define STOP_NOPROGRESS	4		/* no progress being made */
#define STOP_TIMELIMIT	5		/* time limit expired */
#define STOP_RETRANSMIT	6		/* retransmit limit reached */
#define STOP_NORESOURCE	7		/* insufficient resources */
#define STOP_BADTIMER	8		/* server circuit timer out of range */

struct slot_hdr				/* slot header */
    {
	u_char	shd_dstid;		/* destination slot id */
	u_char	shd_srcid;		/* source slot id */
	u_char	shd_count;		/* byte count */
	u_char	shd_credits:4,		/* credits being transferred */
		shd_type:4;		/* message type */
    };
#define shd_reason shd_credits		/* alternate use of credits field */

struct slot_start			/* start slot */
    {
	u_char	sst_class;		/* service class */
	u_char	sst_minAsize;		/* minimum attention slot size */
	u_char	sst_minDsize;		/* minimum data slot size */
    };

#define STOP_SLDISC	1		/* user requested disconnect */
#define STOP_SLSHUT	2		/* system shutting down */
#define STOP_SLINVSLOT	3		/* invalid slot received */
#define STOP_SLINVSERV	4		/* invalid service class */
#define STOP_SLNORES	5		/* no resources */
#define STOP_SLINUSE	6		/* service in use */
#define STOP_SLNOSUCH	7		/* no such service */
#define STOP_SLDISABLED	8		/* service is disabled */
#define STOP_SLNOTOFF	9		/* service is not offered */
#define STOP_SLNAME	10		/* port name is unknown */
#define STOP_SLPASSWD	11		/* invalid password */
#define STOP_SLQUEUE	12		/* entry is not in queue */
#define STOP_SLREJ	13		/* immediate access rejected */
#define STOP_SLDENIED	14		/* access denied */
#define STOP_SLCORRUPT	15		/* corrupted solicit request */
#define STOP_SLILLEGAL	16		/* command type not supported */
#define STOP_SLNOSTART	17		/* start slot can't be sent */
#define STOP_SLDELETE	18		/* entry deleted by local node */
#define STOP_SLREQPAR	19		/* illegal request parameters */

#define LAT_VER_LOW	5		/* lowest version supported */
#define LAT_VER		5		/* version number */
#define LAT_ECO		1		/* ECO number */
#define LAT_ULTRIX	11		/* Ultrix-32 host */

#define MSG_RUN		0		/* run message */
#define MSG_START	1		/* start message */
#define MSG_STOP	2		/* stop message */
#define MSG_DR1		10		/* service class 1 directory message */
#define MSG_CMD  	12		/* command message */
#define MSG_STAT	13		/* status message */
#define MSG_SOL		14		/* service class 1 Solicit Info msg */
#define MSG_RESP	15		/* service class 1 Response Info msg */

#define SLOT_DATA_A	0		/* data_a slot */
#define SLOT_START	9		/* start slot */
#define SLOT_DATA_B	10		/* data_b slot */
#define SLOT_ATT	11		/* attention slot */
#define SLOT_REJECT	12		/* reject slot */
#define SLOT_STOP	13		/* stop slot */

/*
 * Service class specific message formats.
 */
struct slotb_1				/* data_b slot for service class 1 */
    {
	u_char	sb1_flags;		/* control flags */
	u_char	sb1_spout;		/* stop output character */
	u_char	sb1_stout;		/* start output character */
	u_char	sb1_spin;		/* stop input character */
	u_char	sb1_stin;		/* start input character */
    };
#define SB1_ENAINPUT	0x1		/* enable input flow control */
#define SB1_DISINPUT	0x2		/* disable input flow control */
#define SB1_ENAOUTPUT	0x4		/* enable output flow control */
#define SB1_DISOUTPUT	0x8		/* disable output flow control */
#define SB1_BREAK	0x10		/* break condition detected */
#define SB1_SET		0x20		/* set port characterics */
#define SB1_REPORT	0x40		/* report port characterics */

/*
 * Service class specific multicast directory message formats.
 */
struct direct_1
    {
	u_char	dr1_type;		/* message type + flags */
	u_char	dr1_srvtimer;		/* server circuit timer */
	u_char	dr1_Hver;		/* high protocol version */
	u_char	dr1_Lver;		/* low protocol version */
	u_char	dr1_Cver;		/* current protocol version */
	u_char	dr1_eco;		/* current protocol eco */
	u_char	dr1_inc;		/* message incarnation */
	u_char	dr1_change;		/* change flags */
	u_char	dr1_framesize[2];	/* data link receive frame size */
	u_char	dr1_nodetimer;		/* node multicast timer */
	u_char	dr1_status;		/* node status */
      };

/*
 * Service class specific solicit information message format.
 */
struct solicit_1
    { 
	u_char	sol1_type;		/* message type + flags */
	u_char	sol1_protofmt;		/* protocol format */
	u_char	sol1_Hver;		/* high protocol version */
	u_char	sol1_Lver;		/* low protocol version */
	u_char	sol1_Cver;		/* current protocol version */
	u_char	sol1_eco;		/* current protocol eco */
	u_char	sol1_framesize[2];	/* data link receive frame size */
	u_char	sol1_solid[2];		/* solicitation id */
	u_char	sol1_resptimer[2];	/* response timer */
	u_char  sol1_dstnodelen;	/* dest node name length */
	u_char  sol1_dstnode[20];	/* dest node name */
    };

/*
 * Service class specific response information message format.
 */
struct response_1
    {
	u_char	rs1_type;		/* message type + flags */
	u_char	rs1_protofmt;		/* protocol format */
	u_char	rs1_Hver;		/* high protocol version */
	u_char	rs1_Lver;		/* low protocol version */
	u_char	rs1_Cver;		/* current protocol version */
	u_char	rs1_eco;		/* current protocol eco */
	u_char	rs1_framesize[2];	/* data link receive frame size */
	u_char	rs1_solid[2];		/* solicitation id */
	u_char	rs1_respstat[2];	/* response status */
	u_char	rs1_nodestat[2];	/* source node status */
	u_char	rs1_srcnode[6];		/* source node address */ 
	u_char	rs1_nodetimer[2];	/* node multicast timer */
    };

struct lat_cmd { 			/* command message */
	u_char	lcm_type;		/* message type + flags */
	u_char	lcm_protofmt;		/* protocol format */
	u_char	lcm_Hver;		/* high protocol version */
	u_char	lcm_Lver;		/* low protocol version */
	u_char	lcm_Cver;		/* current protocol version */
	u_char	lcm_eco;		/* current protocol eco */
	u_char	lcm_framesize[2];	/* data link receive frame size */
	u_char	lcm_reqid[2];		/* request identifier */
	u_char  lcm_entryid[2];		/* entry identifier */
	u_char	lcm_cmdtype;		/* command type */
	u_char	lcm_cmdmod;		/* command modifier */
};

struct lat_ucom {			/* User command ioctl structure */
        u_char  luc_ifname[IFNAMSIZ];	/* Interface name */
  	u_char  luc_addr[6];		/* Ethernet address */
	u_short luc_len;		/* Length of user data */
	u_char  luc_cmdtype;		/* Command type */
        u_char  luc_cmdmod;		/* Command modifier */
	u_char  luc_objnodelen;		/* Object node name length */
	u_char  luc_objnodenam[MAXNAM];	/* Object node name */
	u_char  luc_subportlen;		/* Subject port length */
        u_char  luc_subport[MAXNAM];	/* Subject port nam */
	u_char  luc_subdescrlen;	/* Subject description length = 0 */
	u_char  luc_objsrvclen;		/* Object service length */
	u_char  luc_objsrvc[MAXNAM];	/* Object service */
	u_char  luc_objportlen;		/* Object port length */
	u_char  luc_objport[MAXNAM];	/* Object port name */
	u_char  luc_param;		/* End of parameter list = 0 */
};

struct lat_stat {			/* status message */
	u_char	lstat_type;		/* message type + flags */
	u_char	lstat_protofmt;		/* protocol format */
	u_char	lstat_Hver;		/* high protocol version */
	u_char	lstat_Lver;		/* low protocol version */
	u_char	lstat_Cver;		/* current protocol version */
	u_char	lstat_eco;		/* current protocol eco */
	u_char	lstat_framesize[2];	/* data link receive frame size */
	u_short	lstat_rexmit_timer;	/* status retransmit timer */
	u_char	lstat_entries;		/* entries counter */
	u_char  lstat_nodenamlen;	/* subject node name length */
};
	
struct ecb {				/* entry control block */
        u_char  	ecb_statrecd;	/* status message received */
	u_char  	ecb_inuse;      /* request or session active */
	u_char		ecb_hostinit;	/* host-initiated connects only */
	u_short 	ecb_reqid;	/* request identifier */
	u_short 	ecb_entryid;	/* entry identifier */
	int     	ecb_error;	/* ULTRIX error */
	struct mbuf	*ecb_cmdmsg;	/* cmd message to dequeue on intr */
	struct ifnet 	*ecb_if;	/* interface for this unit */
	struct {
	    u_short lat_family;
	    u_char  lat_addr[6];
	}		ecb_addr;	/* Ethernet address */
};

struct lat_statent {			/* entry in status message */
 	u_char  lent_len;		/* length of entry */
	u_char  lent_stat;		/* entry status */
	u_char  lent_err;		/* error error */
	u_char  lent_reserved;		/* reserved for future use */
	u_short lent_reqid;		/* request identifier */
	u_short lent_entryid;		/* entry identifier */
	u_short lent_lapsed;		/* time elapsed */
	u_short lent_minqu;		/* minimum queue */
	u_short lent_maxqu;		/* maximum queue */
};

struct lat_ini {			/* for LIOCINI ioctl */
	u_char  lin_cmd;		/* cmdbyte */
	char    lin_dev[4*MAXNAM];	/* name of LAT tty */
};

#define MAXSERVICE	8		/* maximum number of services */
#define LAT_SERVICEID	5		/* first service id number */
#define LAT_HIC		LAT_SERVICEID-1	/* HIC flag in ecb_hostinit */
struct lat_service {
	u_char  id;			/* service id byte */
	char    name[MAXNAM];		/* name of LAT service */
};
