/*
 *	@(#)if_scs.h	4.1	(ULTRIX)	7/2/90
 */

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
/* Modification History:
 *
 *	4-Apr-1989 - Larry Cohen
 *		Add smp support.  
 *
 *	22-July-88 - Create by Larry Cohen
 */


#define SCSNET_HDR_SIZ sizeof(long) /* bytes for scsnet header info */

/* transfer control block - one per block transmit*/

struct scsnet_cntl {
	char state;		/* state of the transfer */
#define SCSNET_BDONE    0x01
#define SCSNET_BSTARTED 0x02
	int  xpte;		/* pte index of transfer buffer */
	struct mbuf *mchain;	/* save mbuf chain pointer */
	caddr_t clbuf_t;	/* pointer to output buffer */
	int clbuf_len;		/* length of output buffer */
	struct buf bufhdr;	/* buffer header for block transfer */
	CSB csb;		/* save lbhandle */
	short timer;		/* used by watch dog timer */
	short xindex;		/* transfer index number */  
	struct _netsystem *netsys; /* for path crash only */
};

/* convert pte table index to xmt buffer virtual address */
#define scsxtob(a)	(scsmempt + (a) * NPTEPG)	

/*
 * Structure of a message sent between peers to request or ack a block transfer
 */
struct scsnet_msg {
	struct _bhandle bhandle;	/* requester's local buffer handle */
	u_long  rspid;			/* pointer to requester's cntl block */
	long totlen;			/* length of block transfer */
	u_short cmd;			/* message type */
#define SCSNET_BCMD_REQUEST 	0
#define SCSNET_BCMD_ACK		1
#define SCSNET_BCMD_NACK 	2
	u_short off;			/* beginning offset of block transfer */
    	char proto_hdr[1];		/* protocol header starts here */
} scsnet_msg_x;


/* 
 *  Connection data
 */

struct scsnet_conn_data {
	struct in_addr rmt_inaddr;
	u_short version;
};
#define SCSNET_VERSION_ID 1


/*
 * netsystems contains information about connections to remote systems 
 */

struct _netsystem {
        struct _netsystem   *flink;	/* Forward queue link		     */
        struct _netsystem   *blink;	/* Backward queue link		     */
	int 	rmt_index;  /* used as a backpointer to knownsystem list */
#define SCSNET_NO_INDEX -1
	SIB	sys_info;   /* keeps host name for us */
	MSB	msb;	    /* in case we have to crash a path */
    	struct _connid connid;    /* Connection identification number  */
	int status;	    /* status of connection to remote system */
#define SCSNET_CONN_CLOSED	0	
#define SCSNET_CONN_SENT	1	
#define SCSNET_CONN_OPEN	2
#define SCSNET_CONN_REC		3
#define SCSNET_NETSYS_ALLOCATED 4
	int retries;	    /* how many times have we tried to contact sysap */
	struct lock_t lk_netsys; /* lock per system */
};

struct _netsystem	*scsnet_topsysaddr;	/* keep track of highest 
						 * allocated netsystem
						 * structure */
struct _netsystem netsys_head;  /* head of knownsystem queue */
struct lock_t lk_scsnetknownsys; 	/* lock to protect connections list */
struct lock_t lk_scsnetrecvs; 	/* lock to serialize scsnet recvs */
struct lock_t lk_netsystems; 	/* lock to protect netsystems list */

#define SCSNET_NO_SYS (struct _netsystem *)0


struct	ifnet scsnetif;

ISB netisb;  /* Info. service block */
CSB netcsb;  /* Communications services block */
CMSB netcmsb; /* Connection management services block */
SCSIB scsnet_local; /* local scs information */

extern u_long scs_dg_size;

extern char Sysbase[];

#define SYSAP_NET	"INET$RFC_790    "

struct in_addr scs_inet_addr; /* local host internet address */
int scsnet_lhost;	      /* local host index */

