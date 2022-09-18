/*
 *	@(#)if_xnareg.h	4.1	(ULTRIX)	7/2/90
 */

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
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

/*
 * DEBNI/DEXNA device registers.
 */
struct	xnareg {
	u_long _reg;
};

struct	xnadevice {
	struct	xnabase {
		union {
			u_long	_xdev;
			struct	xmi_reg	_xnaxmi;
			struct	bi_nodespace	_xnabi;
		} _xnabase_un;
	} *xnabase;
	struct	xnareg *_xpd1;
	struct	xnareg *_xpd2;
	struct	xnareg *_xpst;
	struct	xnareg *_xpud;
	struct	xnareg *_xpci;
	struct	xnareg *_xpcp;
	struct	xnareg *_xpcs;
};

#define	XNAPST_BI	0x0f0
#define	XNAPD1_BI	0x0f4
#define	XNAPD2_BI	0x0f8
#define	XNAPUD_BI	0x0fc
#define	XNAPCI_BI	0x208
#define	XNAPCP_BI	0x20c
#define	XNAPCS_BI	0x210

#define	XNAPD1_XMI	0x100
#define	XNAPD2_XMI	0x104
#define	XNAPST_XMI	0x108
#define	XNAPUD_XMI	0x10c
#define	XNAPCI_XMI	0x110
#define	XNAPCP_XMI	0x114
#define	XNAPCS_XMI	0x118

/*
 * Definition for device type register
 */
#define	xna_dtype	xnabase->_xnabase_un._xdev

/*
 * Values for device type register
 */
#define	XNADEXNA	XMI_XNA
#define	XNADEBNI	BI_XNA

/*
 * Definitions for base register access (BI). biic_ctrl used for BI
 * error logging; xfadr and xfaer registers not supported for BI.
 */
#define	xctrl_bi		xnabase->_xnabase_un._xnabi.biic.biic_ctrl
#define xber_bi		xnabase->_xnabase_un._xnabi.biic.biic_err
/* binode_pad is an array of int's */
#define	xcomm_bi	xnabase->_xnabase_un._xnabi.binode_pad[0x040]

/*
 * Definitions for base register access (XMI).
 */
#define xber_xmi	xnabase->_xnabase_un._xnaxmi.xmi_xbe
#define xfadr_xmi	xnabase->_xnabase_un._xnaxmi.xmi_fadr
/* xmi_pad is an array of char's */
#define	xcomm_xmi	xnabase->_xnabase_un._xnaxmi.xmi_pad[0x000];
#define	xfaer_xmi	xnabase->_xnabase_un._xnaxmi.xmi_pad[0x020]

/*
 * DEBNI/DEXNA port data/control registers
 */
#define xnapst		_xpst->_reg
#define xnapd1		_xpd1->_reg
#define xnapd2		_xpd2->_reg
#define xnapud		_xpud->_reg
#define xnapci		_xpci->_reg
#define xnapcp		_xpcp->_reg
#define xnapcs		_xpcs->_reg

/*
 * Port Control register commands. Writing ANY value to a port control
 * reg. issues the corresponding command.
 */
#define	XPCP_POLL	0x01
#define	XPCS_SHUT	XPCP_POLL
#define	XPCI_INIT	XPCP_POLL

/*
 * Port status register defines.
 */
#define XPST_MASK	0xff
#define	XPST_RESET	0x00
#define XPST_UNDEF	0x01
#define XPST_INIT	0x02

/*
 * Format of a DEBNI/DEXNA buffer address.
 */
struct xnaaddr	{
	u_long	xaddr_lo;
	unsigned xaddr_hi:16;
	unsigned xlen:16;
#define	xmbz	xlen
};

/*
 * Format of a DEBNI/DEXNA line counter entry. Each entry is a quad-word.
 */
struct xnactr_ent {
	u_short lo[2];
	u_short hi[2];
};

/*
 * Up to 12 multicast addresses per user definition
 */
#define	NMULTI	12

/*
 * DEBNI/DEXNA port data block. The port data block provides host/port
 * communications. Block is KM_ALLOC'd in driver and must be aligned to
 * 512-byte boundary.
 */
struct xnapdb {
	unsigned 	addr_mode:8;		/* Addressing Mode */
	unsigned 	resvd1:24;
	struct		xnaaddr	cmd;		/* Command ring address */
	struct		xnaaddr	recv;		/* Receive ring address */
	u_long		spt_len;		/* System page table len. */
	struct		xnaaddr	spt;		/* System page table addr. */
	u_long		gpt;			/* Global page table addr. */
	/*
	 * Host interrupt and error interrupt data
	 */
	struct		xnavec {
			unsigned level:2;
			unsigned vector:14;
			unsigned nid_mask:16;
	} ivec, evec;
	struct		xnaaddr	ubua;		/* UBUA counter addr. */
	struct		xnactr_ent p_sbua;	/* Potental sysbuf unavail */
	struct		xnactr_ent a_sbua;	/* Actual sysbuf unavail */
	struct		xnactr_ent a_dor;	/* Actual Data Overrun */
	u_char		driver_resvd[52];	/* reserved to driver	*/
	u_char		port_err[128];		/* error log area	*/
	u_char		port_resvd[256];	/* reserved to port	*/
};

/*
 * DEBNI/DEXNA addressing modes. (Only use AM_VIRT30 for VAXen).
 */
#define AM_VIRT30	30
#define AM_VIRT30_I	31
#define AM_VIRT34	34
#define AM_VIRT34_I	35
#define AM_PHYS40	40
#define AM_PHYS40_I	41

/*
 * DEBNI/DEXNA command and transmit ring entry. Command and transmit ring
 * entries are of fixed length determined by XNA_XMIT_NBUFS. Only a single
 * buffer segment is allowed for commands. The "_buf_un" address is used
 * to keep the head of the mbuf chain to be freed for transmits, and the
 * address of the KM_ALLOC'd buffer for commands. (For commands, this
 * address is the address used to wakeup the issuer of the command.)
 */
struct xnacmd_ring {
	unsigned	usr_index:8;
	unsigned	nbufs:8;
	unsigned	error:8;
	unsigned	status:8;
	struct		mbuf *mbuf_tofree;
/*
 * MUST be: (2 <= XNA_XMIT_NBUFS <= 15)
 */
#define	XNA_XMIT_NBUFS	12
	struct		xnaaddr bseg[XNA_XMIT_NBUFS];
};

/*
 * DEBNI/DEXNA receive ring entry. Only a single buffer address allowed.
 */
struct xnarecv_ring {
	unsigned	len:16;
	unsigned	usr_index:8;
	unsigned	error:6;
	unsigned	status:2;
	struct		xnaaddr	bseg;
	struct mbuf	*mbuf_recv;
};

/*
 * Transmit, command status bits
 */
#define ST_MAP		0x10
#define ST_CMD		0x20
#define ST_TERR		0x40
#define ST_TOWN		0x80

/*
 * Receive status bits
 */
#define ST_RERR		0x01
#define ST_ROWN		0x02

/*
 * Transmit error codes
 */
#define XMITERR_FRAME		0x01
#define XMITERR_ADDR_INVAL	0x02
#define XMITERR_ADDR_TRANS	0x03
#define XMITERR_TRANSFER	0x04
#define XMITERR_LOSS		0x05
#define XMITERR_RETRY		0x06
#define XMITERR_LATE		0x07
#define XMITERR_TIMEOUT		0x08
#define XMITERR_OTHER		0x09

/*
 * Receive error codes
 */
#define	RECVERR_FRAME		0x01
#define RECVERR_ADDR_INVAL	0x02
#define RECVERR_ADDR_TRANS	0x03
#define	RECVERR_TRANSFER	0x04
#define	RECVERR_LONG		0x05
#define RECVERR_CRC		0x06

/*
 * Command error codes
 */
#define	CMDERR_FRAME		XMITERR_FRAME
#define	CMDERR_ADDR_INVAL	XMITERR_ADDR_INVAL
#define	CMDERR_ADDR_TRANS	XMITERR_ADDR_TRANS
#define	CMDERR_TRANSFER		XMITERR_TRANSFER
#define	CMDERR_CMDINVAL		XMITERR_LOSS
#define	CMDERR_OPINVAL		XMITERR_RETRY

/*
 * DEBNI/DEXNA command buffer formats. Command buffers are KM_ALLOC'd and cast
 * to the appropriate union member. Normal transmit buffers consist of the
 * data regions of mbufs passed in by xnaoutput(). The maintenence commands
 * are not implemented by the ULTRIX port driver.
 */
struct xnacmd_buf {
	u_long		opcode;		/* Command opcodes, defined above */
	union	{
		/*
		 * XNA parameter block
		 * (opcode = CMD_PARAM)
		 */
		struct	_xnaparam {
			u_long		sysdate_lo;
			u_long		sysdate_hi;
			struct		xnaaddr dpa;
			struct		xnaaddr apa;
			u_char		bvc[8];
			u_short		cur_src;
			u_short		cur_dst;
			u_short		cur_mca;
			u_short		cur_user;
			u_short		max_adr;
			u_short		max_user;
			unsigned	loop_mode:2;
			unsigned	flags:6;
			unsigned	resvd:24;
			u_char		serial_num[12];
		} _xnaparam;

		/*
		 * XNA counters block (quad-word values). These counters
		 * cannot be cast as a pair of longwords safely, since the
		 * DEBNI does not maintain VAX-style word ordering.
		 * (opcode = CMD_RDCNTR, CMD_RCCNTR)
		 */
		struct	_xnactrs {
			struct		xnactr_ent	seconds;
			struct		xnactr_ent	bytercvd;
			struct		xnactr_ent	bytesent;
			struct		xnactr_ent	blokrcvd;
			struct		xnactr_ent	bloksent;
			struct		xnactr_ent	mbytercvd;
			struct		xnactr_ent	mblokrcvd;
			struct		xnactr_ent	mbytesent;
			struct		xnactr_ent	mbloksent;
			struct		xnactr_ent	deferred;
			struct		xnactr_ent	single;
			struct		xnactr_ent	multiple;
			/*
			 * Bit map values (expanded into counters).
			 */
			struct		xnactr_ent	sendfail_retry;
			struct		xnactr_ent	sendfail_carrier;
			struct		xnactr_ent	sendfail_short;
			struct		xnactr_ent	sendfail_open;
			struct		xnactr_ent	sendfail_long;
			struct		xnactr_ent	sendfail_defer;
			struct		xnactr_ent	recvfail_crc;
			struct		xnactr_ent	recvfail_frame;
			struct		xnactr_ent	recvfail_long;
			struct		xnactr_ent	unrecog;
			struct		xnactr_ent	overrun;
			struct		xnactr_ent	sysbuf;
			struct		xnactr_ent	userbuf;
			struct		xnactr_ent	collis;
		} _xnactrs;

		/*
		 * XNA user counters block (quad-word values).
		 * (opcode = CMD_RCUCNTR, CMD_RDUCNTR)
		 */
		struct	_xnauctrs {
			struct		xnactr_ent	seconds;
			struct		xnactr_ent	bytercvd;
			struct		xnactr_ent	bytesent;
			struct		xnactr_ent	blokrcvd;
			struct		xnactr_ent	bloksent;
			struct		xnactr_ent	recvfail_error;
			struct		xnactr_ent	recvfail_mca;
			struct		xnactr_ent	recvfail_long;
			struct		xnactr_ent	sendfail;
		} _xnauctrs;

		/*
		 * XNA user start command. NOTE: if defining an 802
		 * user, the "xnasnap" struct will come after the N'th
		 * multi_addr, where: 0 <= N <= NMULTI. This means that
		 * the format of this structure may be of variable length.
		 * (opcode = CMD_USTART, CMD_UCHANGE, CMD_USTOP)
		 */
		struct	_xnaustart {
			unsigned	sap_ptt:16;
			unsigned	resvd:8;
			unsigned	mode:8;
			struct		xnaaddr	user_phys;
			u_short		addr_len;
			u_short		addr_alloc;
			struct		multi_addr {
					u_char	addr[6];
			} multi_addr[NMULTI];
			struct		xnasnap {
					u_char	snap_sap_id[5];
					u_char	ngsaps;
					u_char	gsap[NMULTI];
			} xnasap;
		} _xnaustart;

		/*
		 * SYSID data region
		 */
		u_char	_xnadata[464];	/* largest SYSID block == 464 bytes */
	} _cmd_un;
};

/*
 * defines for union members
 */
#define	xnaparam	_cmd_un._xnaparam
#define	xnactrs		_cmd_un._xnactrs
#define	xnauctrs	_cmd_un._xnauctrs
#define	xnaustart	_cmd_un._xnaustart
#define	xnasysid	_cmd_un._xnadata

/*
 * Command opcodes. Maintenence commands not supported.
 */
#define	CMD_NOP		0x00
#define CMD_SYSID	0x01
#define CMD_PARAM	0x02
#define CMD_RCCNTR	0x03
#define CMD_RDCNTR	0x04
#define CMD_RCUCNTR	0x05
#define CMD_RDUCNTR	0x06
#define CMD_USTART	0x07
#define CMD_UCHANGE	0x08
#define CMD_USTOP	0x09
#define CMD_MAINT	0x0a

#define	CMD_COMPLETE	0xfe		/* Command complete signal */
#define CMD_INVAL	0xff		/* Command failure signal */

/*
 * Bits for PARAM and USTART (UCHANGE) command flags
 */
#define	PARAM_NOLOOP	0x00
#define	PARAM_ELOOP	0x01
#define PARAM_ILOOP	0x02

#define	PARAM_DCRC	0x01
#define	PARAM_BOO	0x02
#define	PARAM_BTM	0x04
#define	PARAM_MPA	0x08
#define	PARAM_LEN	0x10

#define	USTART_BAD	0x01
#define	USTART_PAD	0x02
#define	USTART_CL1	0x04
#define	USTART_PROM	0x08
#define	USTART_802	0x10
#define	USTART_BDC	0x20
#define	USTART_UNK	0x40
#define	USTART_AMC	0x80

/*
 * The ethernet user
 */
#define XNA_ETHERU	0x00
