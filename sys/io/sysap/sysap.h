/*
 *	@(#)sysap.h	4.2	(ULTRIX)	10/16/90
 */

/************************************************************************
 *                                                                      *
 *                      Copyright (c) 1988 - 1989 by                    *
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
 *		Systems Applications
 *
 *   Abstract:	This module contains generic System Applications - Systems
 *		Communication Services( SYSAP - SCS ) constants, data
 *		structure definitions, and macros.
 *
 *   Creator:	Todd M. Katz	Creation Date:	March 23, 1985
 *
 *   Modification History:
 *
 *   16-Oct-1990	Pete Keilty
 *	Added new path failure reason PF_PORTERROR for port errors
 *	and PF_ERROR now is for path failure other than port errors.
 *
 *   06-Jun-1990	Pete Keilty
 * 	Added CIKMF and SHAC hardware types to the local port info.
 *	block. Also packet multiple variable to the path info. block.
 *
 *   09-Nov-1989	David E. Eiche		DEE0080
 *	Add the software port and interconnect type fields to
 *	the local port information block (LPIB).  Add definitions
 *	for the various software ports and interconnects.
 *
 *   19-Sep-1989	Pete Keilty
 *	1. Add XCD to port hardware type, remove XCB.
 *	2. Add expl flag bit to local port info. block (lpib.flags.expl).
 *	   New explicit command addressing ports. 
 *
 *   21-May-1989	Pete Keilty
 *	Add double mapping flag to local port info. block (lpib.flags.dm).
 *	Used for MIPS cpu's that support CI/BVP port.
 *
 *   06-Dec-1988	Todd M. Katz		TMK0009
 *	1. Added to structure definitions PIB( union pd ) and LPIB( union pd )
 *	   MSI specific fields.
 *	2. Add padding when it is necessary to keep longword alignment.  This
 *	   most often must be done following declarations of type c_scaaddr.
 *	   While some space is wasted such alignment is essential for ports of
 *	   SCA to hardware platforms which require field alignment and access
 *	   type to match( ie- only longword aligned entities may be longword
 *	   accessed ).
 *	3. Modify TMK0006.  The MSB field lproc_name has been moved into
 *	   union definition ov1( first overlaid field ) and shorthand notation
 *	   Lproc_name has been created.
 *
 *   29-Sep-1988	Todd M. Katz		TMK0008
 *	Add the following return value: RET_INVPSTADDR.
 *
 *   23-Sep-1988	Todd M. Katz		TMK0007
 *	Change size of field force within structure definition MSB from
 *	u_short -> u_long.
 *
 *   29-Aug-1988	Todd M. Katz		TMK0006
 *	1. Add hardware port types: KFXSA, RF70, RF31, TF70, and TF31.
 *	2. Add field lproc_name( local SYSAP name ) to structure MSB.
 *	3. Increase the size of field reason( reason for path failure ) from a
 *	   u_short -> u_long.
 *
 *   29-Jul-1988	Todd M. Katz		TMK0005
 *	Add reason codes ADR_CONNECTION and ADR_VERSION.
 *
 *   03-Jul-1988	Todd M. Katz		TMK0004
 *	Added the constant NODENAME_SIZE and modified the SIB appropriately.
 *
 *   23-Mar-1988	Todd M. Katz		TMK0003
 *	Rename HPT_CIBCA -> HPT_CIBCA_AA and HPT_CIBCAB -> HPT_CIBCA_BA.
 *
 *   23-Mar-1988	Todd M. Katz		TMK0002
 *	1. Added hardware port type code for CIBCA-BA( HPT_CIBCAB ).
 *	2. Reserved range of Accept/Disconnect/Reject reason codes for use on
 *	   per-SYSAP basis.  Range begins at 0x100 << 0x3( 2048 decimal ).
 *	   This allows a maximum of 0xFF( 256 decimal ) reason codes common to
 *	   all SYSAPs.  The low order three bits of all reason codes are
 *	   reserved for use as a severity level.
 *
 *   08-Jan-1988	Todd M. Katz		TMK0001
 *	Formated module, revised comments, increased robustness, and
 *	added SMP support.
 */

/* SYSAP - SCS Constants.
 */
					/* Accept/Disconnect/Reject Reasons  */
					/* Severity values: bits 0-2	     */
					/*	0 = warning		     */
					/*	1 = success		     */
					/*	2 = error		     */
					/*	4 = severe error	     */
#define	ADR_SUCCESS		 1	/* Normal or success		     */
#define	ADR_NOLISTENER		10	/* No matching listener		     */
#define	ADR_NORESOURCE		18	/* No resources for connection	     */
#define	ADR_DISCONN		26	/* Disconnected			     */
#define	ADR_NOCREDIT		34	/* Insufficient credit for connection*/
#define	ADR_PATH_FAILURE	44	/* Path failed			     */
#define	ADR_BUSY		50	/* Listener is busy		     */
#define	ADR_NOSUPPORT		58	/* Connections to SYSAP NOT supported*/
#define	ADR_CONNECTION		66	/* Connection exists or in progress  */
#define	ADR_VERSION		74	/* Remote SYSAP runs bad protocol ver*/
					/* Per-SYSAP reason codes( => 2048 ) */

					/* Buffer Disposition Codes	     */
#define	RECEIVE_BUF		 0	/* Make sent buffer a receive buffer */
#define	DEALLOC_BUF	0x01000000	/* Deallocate sent buffer 	     */

					/* Control Routine Event Codes	     */
#define	CRE_CONN_REC		 1	/* Connect request received	     */
#define	CRE_CONN_DONE		 2	/* Connect request completed	     */
#define	CRE_ACCEPT_DONE		 3	/* Connection acceptance completed   */
#define	CRE_REJECT_DONE		 4	/* Connection rejection completed    */
#define	CRE_DISCONN_REC		 5	/* Disconnect request received	     */
#define	CRE_DISCONN_DONE	 6	/* Disconnect request completed	     */
#define	CRE_PATH_FAILURE	 7	/* Path failed			     */
#define	CRE_NEW_PATH		 8	/* New path discovered		     */
#define	CRE_BLOCK_DONE		 9	/* Block transfer completed	     */
#define	CRE_CREDIT_AVAIL	10	/* Send credits received	     */
#define	CRE_MAP_AVAIL		11	/* Mapping resources available	     */

					/* IPL Codes			     */
#define	IPL_SCS			21	/* IPL of SCS			     */
#define	IPL_POWER		31	/* Highest IPL value		     */

					/* Miscellaneous Constants	     */
#define	DATA_SIZE		16	/* Size of connection data	     */
#define	NAME_SIZE		16	/* Size of process names	     */
#define	NODENAME_SIZE		 8	/* Size of node name		     */

					/* Path Failure Reasons		     */
					/* Severity values: bits 0-2	     */
					/*	0 = warning		     */
					/*	1 = success		     */
					/*	2 = error		     */
					/*	4 = severe error	     */
#define	PF_SCSPROTOCOL		 2	/* 0x02 SCS protocol error 	     */
#define	PF_PPDPROTOCOL		10	/* 0x0A PPD protocol required failure*/
#define	PF_SCSTIMEOUT		18	/* 0x12 SCS timeout		     */
#define	PF_SYSAP		24	/* 0x18 SYSAP requested failure	     */
#define	PF_POWER		36	/* 0x24 Power failure occurred	     */
#define	PF_FATALERROR		44	/* 0x2C Fatal port error occurred     */
#define	PF_HOSTSHUTDWN		48	/* 0x30 Remote host requested shutdown*/
#define	PF_ERROR		58	/* 0x3A Nonfatal port error occurred  */
#define	PF_PORTERROR		68	/* 0x44 Severe  port error occurred  */

					/* Return Values		     */
					/* Severity values: bits 0-2	     */
					/*	0 = warning		     */
					/*	1 = success		     */
					/*	2 = error		     */
					/*	4 = severe error	     */
#define	RET_SUCCESS	    0x0001	/* Success			     */
#define	RET_ALLOCFAIL	    0x000A	/* Allocation failure		     */
#define	RET_INVCSTATE	    0x0012	/* Connection in invalid state	     */
#define	RET_INVCONNID	    0x001A	/* Invalid connection identification */
#define	RET_INVLPSTATE	    0x0022	/* Local port in invalid state	     */
#define	RET_INVPSTATE	    0x002A	/* Path in invalid state	     */
#define	RET_FAILURE	    0x0032	/* Failure			     */
#define	RET_MLOCK	    0x0038	/* Local port is maintenance locked  */
#define	RET_NOCREDITS	    0x0040	/* No credits to send message	     */
#define	RET_NOPATH	    0x004A	/* Path not exist		     */
#define	RET_NORESOURCES	    0x0050	/* No resources available	     */
#define	RET_NOCONN	    0x005A	/* SCS Connection not exist	     */
#define	RET_NOLPORT	    0x0062	/* Local port not exist		     */
#define	RET_NOSYSTEM	    0x006A	/* System not exist		     */
#define	RET_NOSUPPORT	    0x0070	/* Function unsupported on local port*/
#define	RET_ZEROSYSID	    0x0078	/* sca_system_id is un-initialized   */
#define	RET_CONNBUSY	    0x0080	/* Connection busy - can't disconnect*/
#define	RET_INVPSTADDR	    0x008A	/* Invalid port station address	     */

/* SYSAP - SCS Data Structure Definitions.
 */
typedef struct _bhandle	{		/* Buffer Handle		     */
    union		   {		/* PD specific buffer handle	     */
	struct _gvpbhandle gvp;		/*  Generic Vaxport buffer handle    */
	struct _uqbhandle  uq;		/*  UQ buffer handle		     */
    } pd;
    u_long	scsid;			/* SCS Identification number         */
} BHANDLE;

typedef struct _connid	{		/* Connection Identification Number  */
    u_short	index;			/* CBVTE index number		     */
    u_short	seq_num;		/* CBVTE sequence number	     */
} CONNID;

typedef struct	{			/* Communication Services Block	     */
    struct _connid connid;		/* Connection ID		     */
    u_char	   *buf;		/* Application data buffer	     */
    u_long	   size;		/* Application data transfer size    */
    union		{		/* First overlaid field		     */
	u_char		*aux;		/*  SYSAP's auxiliary structure	     */
	u_long 		blockid;	/*  Block data transfer ID number    */
    } ov1;
#define	Aux			ov1.aux
#define	Blockid			ov1.blockid
    union		{		/* Second overlaid field   	     */
	u_long		rboff;		/*  Remote buffer transfer offset    */
	struct buf	*sbh;		/*  System buffer handle	     */
    } ov2;
#define	Rboff			ov2.rboff
#define	Sbh			ov2.sbh
    union		{		/* Third overlaid field		     */
	short		nbufs;		/*  Number datagrams to add/subtract */
	short		ncredits;	/*  Number to adjust max send credit */
	u_long		disposal;	/*  Disposition of sent buffer	     */
	u_long		lboff;		/*  Local buffer transfer offset     */
    } ov3;
#define	Nbufs			ov3.nbufs
#define	Ncredits		ov3.ncredits
#define	Disposal		ov3.disposal
#define	Lboff			ov3.lboff
    struct _bhandle lbhandle;		/* Local buffer handle		     */
    struct _bhandle rbhandle;		/* Remote buffer handle		     */
} CSB;

typedef struct _cib	{		/* Connection Information Block	     */
    struct _connid lconnid;		/* Local connection ID		     */
    struct _connid rconnid;		/* Remote connection ID		     */
    u_short	   cstate; 		/* Connection state 		     */
#define	CS_CLOSED		 0	/*  Connection is closed	     */
#define	CS_LISTEN		 1	/*  Listening for a connection	     */
#define	CS_OPEN			 2	/*  Connection is open		     */
#define	CS_DISCONN_ACK		 3	/*  Disconnect sent and acknowledged */
#define	CS_DISCONN_REC		 4	/*  Request for disconnect received  */
#define	CS_DISCONN_SNT		 5	/*  Disconnect request sent	     */
#define	CS_DISCONN_MTCH		 6	/*  Both SYSAPs have sent disconnects*/
#define	CS_CONN_SNT		 7	/*  Connection request sent	     */
#define	CS_CONN_ACK		 8	/*  Connection sent and acknowledged */
#define	CS_CONN_REC		 9	/*  Request for connection received  */
#define	CS_ACCEPT_SNT		10	/*  Accept request sent		     */
#define	CS_REJECT_SNT		11	/*  Reject request sent		     */
#define	CS_PATH_FAILURE		12	/*  Path failed			     */
    u_short	   cbstate;		/* CB state			     */
#define	CB_NOT_WAIT		 0	/*  Not waiting at all		     */
#define	CB_CONN_PEND		 1	/*  Waiting to send a connect request*/
#define	CB_ACCEPT_PEND		 2	/*  Waiting to send an accept request*/
#define	CB_REJECT_PEND		 3	/*  Waiting to send a reject request */
#define	CB_DISCONN_PEND		 4	/*  Waiting to send disconn request  */
#define	CB_CREDIT_PEND		 5	/*  Waiting to send a credit request */
#define	CB_MAX_PEND	CB_CREDIT_PEND	/*  Maximum pending request number   */
#define	Dirid		cbstate		/* Directory ID ( Listeners Only )   */
    struct    	{			/* Connection status flags	     */
	u_short	cwait		:  1;	/*  SYSAP waiting for credits        */
	u_short	abort_fork	:  1;	/*  Connection abortion fork	     */
	u_short	disconnect	:  1;	/*  Disconnect -> conn rejection     */
	u_short	 		: 13;	/*  Unused			     */
    } status;
    u_short	   ntransfers;		/* Number of transfers in progress   */
    u_long	   reason; 		/* Reject / Disconnect reason	     */
    u_char	   rproc_name[ NAME_SIZE ];/* Remote SYSAP name		     */
    u_char	   lproc_name[ NAME_SIZE ];/* Local SYSAP name		     */
    u_char	   lconn_data[ DATA_SIZE ];/* Local SYSAP connection data    */
    u_char	   rconn_data[ DATA_SIZE ];/* Remote SYSAP connection data   */
    u_long	   reserved_credit;	/* Number credits reserved by SYSAP  */
    u_long	   snd_credit;		/* Current send credit		     */
    u_long	   min_snd_credit;	/* Local SYSAP's credit requirement  */
    u_long	   rec_credit;		/* Send credit held by remote SYSAP  */
    u_long	   init_rec_credit;	/* Remote SYSAP's maximum credit     */
    u_long	   min_rec_credit;	/* Remote SYSAP's credit requirement */
    u_long	   pend_rec_credit;	/* Credit pending extension	     */
    u_long	   dg_credit;		/* Number of local datagram credits  */
    u_long	   dgs_snt;		/* Number of datagrams sent	     */
    u_long	   dgs_rec;		/* Number of datagrams received	     */
    u_long	   dgs_discard;		/* Number of datagrams discarded     */
    u_long	   msgs_snt;		/* Number of messages sent	     */
    u_long	   msgs_rec;		/* Number of messages received	     */
    u_long	   sdatas_snt;		/* Number of send datas sent	     */
    u_long	   bytes_snt;		/* Number of bytes sent 	     */
    u_long	   rdatas_snt;		/* Number of request datas sent	     */
    u_long	   bytes_req;		/* Number of bytes requested	     */
    u_long	   bytes_mapped;	/* Number of bytes mapped	     */
} CIB;

typedef struct	{			/* Connection Management Service Blk */
    void	( *control )();		/* Connection control routine	     */
    void	( *msg_event )();	/* Msg event notification routine    */
    void	( *dg_event )();	/* Dg event notification routine     */
    u_char	*aux;			/* Auxiliary structure pointer	     */
    c_scaaddr	sysid;			/* System identification number	     */
    u_short			: 16;
    c_scaaddr	rport_addr;		/* Remote port station address	     */
    u_short			: 16;
    u_long	lport_name;		/* Local port device name	     */
    u_long	blockid;		/* Block data transfer id field	     */
    struct _connid connid;		/* Identification of connection	     */
    u_char	rproc_name[ NAME_SIZE ];/* Remote SYSAP name( blank filled ) */
    u_char	lproc_name[ NAME_SIZE ];/* Local SYSAP name( blank filled )  */
    u_char	conn_data[ DATA_SIZE ];	/* Connection data		     */
    u_short	status;			/* Status			     */
#define	Reason  status			/*  Reject / Disconnect reason	     */
    u_short	init_dg_credit;		/* Initial datagram credit	     */
    u_short	min_snd_credit;		/* SYSAP minimum credit requirement  */
    u_short	init_rec_credit;	/* Initial sequenced message credit  */
#define	Init_snd_credit init_rec_credit	/*  Initial send credit extended     */
} CMSB;

typedef struct	{			/* Information Service Block	     */
    c_scaaddr	   sysid;		/* Current system ID number 	     */
    u_short			: 16;
    c_scaaddr	   rport_addr;		/* Current remote port address	     */
    u_short			: 16;
    u_long	   lport_name;		/* Current local port device name    */
    struct _connid connid;		/* Current connection ID	     */
    c_scaaddr	   next_sysid;		/* Next system identification number */
    u_short			: 16;
    c_scaaddr	   next_rport_addr;	/* Next remote port address	     */
    u_short			: 16;
    u_long	   next_lport_name;	/* Next local port device name	     */
    struct _connid next_connid;		/* Next connection ID		     */
} ISB;

typedef struct _lpib	{		/* Local Port Information Block	     */
    u_short	nerrs;			/* Number of errors on port	     */
    u_short	nreinits;		/* Number of re-inits on port	     */
    struct	{			/* Local port type		     */
	u_long	hwtype		:  8;	/*  Hardware port type		     */
	u_long	swtype		:  8;	/*  Software port type		     */
	u_long	ictype		:  8;	/*  Interconnect type		     */
	u_long			:  7;	/*  Unused			     */
	u_long	dual_path	:  1;	/*  0/1 for single / dual path port  */
    } type;
#define	HPT_UQSSP		 1	/*  UQSSP storage port		     */
#define	HPT_CI780		 2	/*  CI780 communications port	     */
#define	HPT_CI750		 3	/*  CI750 communications port	     */
#define	HPT_HSC			 4	/*  HSC communications port	     */
#define	HPT_CIBCI		 5	/*  CIBCI communications port	     */
#define	HPT_KL10		 6	/*  KL10 communications port	     */
#define	HPT_CIBCA_BA		10	/*  CIBCA-BA communications port     */
#define	HPT_CIBCA_AA		11	/*  CIBCA-AA communications port     */
#define	HPT_BVPSSP		12	/*  BVPSSP storage port		     */
#define	HPT_CIXCD		14	/*  CIXCD communications port	     */
#define	HPT_SII			32	/*  SII communications port	     */
#define	HPT_KFQSA		33	/*  KFQSA storage port		     */
#define	HPT_SHAC		34	/*  SHAC communications port	     */
#define	HPT_KFXSA		35	/*  KFXSA storage port		     */
#define	HPT_CIKMF		36	/*  Dual CI communications ports     */
#define	HPT_RF71		48	/*  RF71 disk storage port	     */
#define	HPT_RF30		49	/*  RF30 disk storage port	     */
#define	HPT_RF31		50	/*  RF31 disk storage port	     */
#define	HPT_TF70		64	/*  TF70 tape storage port	     */
#define	HPT_TF85		65	/*  TF85 tape storage port	     */
#define SPT_UQSSP		 1	/*  UQSSP software port		     */
#define SPT_CI			 2	/*  CI software port		     */
#define SPT_BVPSSP		 3	/*  BVPSSP software port	     */
#define SPT_MSI			 4	/*  MSI software port		     */
#define ICT_SBICMI		 0	/*  SBI or CMI interconnect	     */
#define ICT_BI			 1	/*  BI interconnect		     */
#define ICT_XMI			 2	/*  XMI interconnect		     */
#define ICT_UB			 3	/*  Unibus interconnect		     */
#define ICT_QB			 4	/*  Q-Bus interconnect		     */
#define ICT_SII			 5	/*  SII interconnect		     */
#define ICT_SHAC		 6	/*  SHAC interconnect		     */
    u_long	reason;			/* Reason for port failure	     */
    u_long	name;			/* Local port name		     */
    c_scaaddr	addr;			/* Local Port address		     */
    struct {
        u_short	 dm	: 1;		/* Double map port into Vaxmap -MIPS */
        u_short	 expl	: 1;		/* Explicit command addr. port       */
        u_short		: 14;
    } flags;
    union		 {		/* PD dependent fields		     */
	struct  _gvplpib gvp;		/*  Generic Vaxport specific fields  */
	struct  _msilpib msi;		/*  MSI specific fields		     */
	struct  _uqlpib	 uq;		/*  UQ specific fields		     */
    } pd;
    union		   {		/* PPD dependent fields		     */
	struct  _cippdlpib cippd;	/*  CI PPD specific fields	     */
    } ppd;
} LPIB;

typedef struct	{			/* Maintenance Service Block	     */
    u_long	lport_name;		/* Local port device name	     */
    c_scaaddr	rport_addr;		/* Remote port station address	     */
    u_short			: 16;
    union	{			/* First overlaid field		     */
	u_long	force;			/*  Force remote reset		     */
	u_long	startaddr;		/*  Remote system start address	     */
	u_char	lproc_name[ NAME_SIZE ];/*  Local SYSAP name( blank filled ) */
    } ov1;
#define	Force		ov1.force
#define	Lproc_name	ov1.lproc_name
#define	Startaddr	ov1.startaddr
} MSB;

typedef struct _pib	{		/* Path Information Block	     */
    u_long	lport_name;		/* Local port device name	     */
    c_scaaddr	rport_addr;		/* Remote port station address	     */
    u_short	state;			/* Path state			     */
#define	PS_CLOSED		 0	/*  Newly created PB		     */
#define	PS_START_SNT		 1	/*  START PPD datagram sent	     */
#define PS_START_REC		 2	/*  START PPD datagram received	     */
#define	PS_STACK_SNT		 3	/*  STACK PPD datagram sent	     */
#define	PS_OPEN			 4	/*  Path open			     */
#define	PS_PATH_FAILURE		 5	/*  Path has failed		     */
#define	PS_MAX_STATE	PS_PATH_FAILURE	/*  Maximum path state number	     */
    struct	{			/* Remote port type		     */
	u_long	hwtype		:  8;	/*  Hardware port type		     */
	u_long			: 23;	/*  Unused			     */
	u_long	dual_path	:  1;	/*  0/1 for single / dual path port  */
    } type;
    u_long	reason;			/* Reason for path failure	     */
    u_short	nconns;			/* Number of connections 	     */
    u_short	duetime;		/* SCS response due time( seconds )  */
    struct	{			/* Path status flags		     */
	u_short	sanity		:  1;	/*  Sanity timer enabled flag	     */
	u_short			: 15;	/*  Unused			     */
    } status;
    u_short	protocol;		/* Remote PPD protocol level	     */
    u_long	path_pktmult;		/* Path data pkt multiple value	     */
    union		{		/* PD dependent fields		     */
	struct _gvppib	gvp;		/*  Generic Vaxport specific fields  */
	struct _msipib	msi;		/*  MSI specific fields		     */
    } pd;
} PIB;

typedef struct _sib	{		/* System Information Block	     */
    c_scaaddr	sysid;			/* System identification number	     */
    u_short 	npaths;			/* Number paths to system	     */
    u_short 	max_dg;			/* Maximum size of application dg    */
    u_short 	max_msg;		/* Maximum size of application msg   */
    u_long	swtype;			/* Software type 		     */
    u_long	swver;			/* Software version		     */
    u_quad	swincrn;		/* Software incarnation number	     */
    u_long	hwtype;			/* Hardware type		     */
    u_dodec	hwver;			/* Hardware version		     */
    u_char	node_name[ NODENAME_SIZE ];/* SCA node name		     */
} SIB;

typedef struct	{			/* SCS Information Block	     */
    u_long	max_gvpbds;		/* Maximum number of GVPBDs	     */
    u_long	max_conns;		/* Maximum number of connections     */
    u_long	cushion;		/* SCS send credit cushion	     */
    u_long	gvp_qretry;		/* GVP queuing failure retry count   */
    u_short	gvp_free_bds;		/* Number of free GVPBDs	     */
    u_short	free_cbvtes;		/* Number of free CBVTEs	     */
    u_short	nconns;			/* Number of logical SCS connections */
    u_short	nlisteners;		/* Number of listeners		     */
    u_short	npaths;			/* Number of paths		     */
    u_short	nsystems;		/* Number of systems		     */
    u_short	nlports;		/* Number of local ports	     */
    u_short	sanity;			/* SCS sanity timer		     */
    struct _sib	system;			/* Local system information	     */
} SCSIB;
