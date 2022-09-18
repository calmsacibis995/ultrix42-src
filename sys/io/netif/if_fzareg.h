/* static	char	*sccsid = "@(#)if_fzareg.h	4.5	(ULTRIX)	11/9/90"; */

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

/*-----------------------------------------------------------------------
 *
 * Modification History:
 *
 * 27-Apr-90	chc (Chran-Ham Chang)
 *	Created this if_ne_data.c module
 *
 *--------------------------------------------------------------------- */

/*
 * DEFZA Port Register
 */
#define FZA_RESET	0x100200	/* port_reset register */
#define FZA_CTL_A	0x100500	/* port_control_a register */
#define FZA_CTL_B	0x100502	/* port_control_b register */
#define FZA_INT_EVENT	0x100400	/* port_interrupt_event */
#define FZA_INT_MASK	0x100404	/* port_interrupt_mask */
#define FZA_STATUS	0x100402	/* port_status */

/*
 * DEFZA DLU address
 */
#define FZA_DLU_ADDR	0x200000	/* packet memory base address */
	
struct fzareg {
	volatile unsigned short reg;
};

struct port_reg {
	struct fzareg *fza_reset ;
	struct fzareg *fza_ctl_a;
	struct fzareg *fza_ctl_b;
	struct fzareg *fza_intr_event;
	struct fzareg *fza_intr_mask;
	struct fzareg *fza_status;
};

#define PORT_REG volatile struct port_reg

/*
 * Definition for the PORT_RESET Register 
 */
#define	DLU_MODE	0x0002
#define RESET		0x0001	
#define FZAREG_CLEAR 	0x0000	

/*
 * definition for the PORT_CONTROL_A 
 */
#define SMT_RCV_OVERFLOW 0x0040
#define FLUSH_DONE	0x0020	
#define SHUT		0x0010
#define HALT		0x0008
#define CMD_POLL	0x0004
#define SMT_RCV_POLL	0x0002
#define XMT_POLL	0x0001

/*
 * definition for the PORT_CONTOL_B
 */
#define FZA_ACTIVE	0x0001

/*
 * definition for the PORT_INTERRUPT_EVENT
 */
#define DLU_DONE	0x0800	
#define	FLUSH_TX	0x0400
#define PM_PARITY_ERR	0x0200
#define	MB_PARITY_ERR	0x0100
#define	NXM_ERR		0x0080
#define	LINK_STATUS_CHG	0x0040
#define	STATE_CHG	0x0020
#define	UNS_POLL	0x0010
#define	CMD_DONE	0x0008
#define	SMT_XMT_POLL	0x0004	
#define	RCV_POLL	0x0002
#define	XMT_PKT_DONE	0x0001
#define RESERVE	 	0xf000	

#define FZA_INTR_MASK	~(PM_PARITY_ERR | MB_PARITY_ERR | NXM_ERR | RESERVE)

/*
 * definition for the PORT_STATUS register
 */

/*
 * Down Line Upgrade status 
 */
#define DLU_STATUS      0x3000
#define DLU_FAILURE	0x0000
#define DLU_ERROR	0x1000
#define DLU_SUCCESS	0x2000

/*
 * LINK STATUS
 */
#define LINK_STATUS		0x0800
#define LINK_UNAVAILABLE	0x0000
#define LINK_AVAILABLE		0x0800

/*
 * ADAPTER_STATE
 */
#define ADAPTER_STATE 		0x0700
#define STATE_RESET		0x0000	 	
#define STATE_UNINITIALIZED	0x0100
#define STATE_INITIALIZED	0x0200
#define STATE_RUNNING		0x0300
#define STATE_MAINTENANCE	0x0400
#define STATE_HALTED		0x0500
	
/*
 * TEST_ID or HALT_ID	bit 
 * XXX need to be defined
 */
#define HALT_UNKNOWN		0x0000	/* halt reason unknown */
#define HALT_HOST_DIRECTED  	0x0001	/* host request to halt state */
#define HALT_HBUS_PARITY   	0x0002	/* host max bus parity error */
#define HALT_HNXM		0x0003	/* host non exist memory */
#define HALT_ADAP_SW_FAULT 	0x0004	/* adapter software fault */ 
#define HALT_ADAP_HW_FAULT 	0x0005	/* adapter hardware fault */ 
#define HALT_CNS_PC_TEST 	0x0006	/* CNS PC trace path test  */ 
#define HALT_CNS_SW_FAULT	0x0007	/* CNS software fault */
#define HALT_CNS_HW_FAULT	0x0008	/* CND hardware fault */

/*
 * TEST_ID
 */
#define ID_FIELD		0x00ff	
/*
 * Format of a DEFZA line counter entry. Each entry is a quad-word.
 */
struct fzactr_ent {
	u_long lo;
	u_long hi;
};

/*
 * The adapter state for the driver reset routine
 */
#define FZA_NORMAL 	0
#define FZA_SHUT 	1
#define FZA_DLU		2
#define FZA_PC_TRACED   3	
 
/*
 * Up to 64 multicast addresses per user definition
 */
#define	NMULTI 64	

/* 
 * DEFZA receive ring entry. Each entry will need two 4K mbufs to fill
 */
struct fzarcv_ring {
	union _rcv_baddr1 {
		struct _rcv_own_addr {
			unsigned 	b_addr1:23;
			unsigned	resv:8;
			unsigned	own:1;
		} _rcv_own_addr;  
		u_long	rcv_addr1;
	} _rcv_baddr1 ; 	
	u_long bufaddr2 ;
	u_long rmc;
	u_long resv2;
}; 

#define rcv_own _rcv_baddr1._rcv_own_addr.own
#define bufaddr1 _rcv_baddr1.rcv_addr1 
#define FZARCVRING volatile struct fzarcv_ring

#define FZA_RCV_OWN 	0x01

/*
 * DEFZA transmit ring entry
 */

struct fzaxmt_ring {
	u_long rmc;			/* RMC info */
	struct mbuf *xmt_mbuf;		/* transmit mbuf chain */
	u_long own;			/* bit 31 RMC own bit */
	u_long resev;			/* reserved long word one */
};  
#define FZAXMTRING volatile struct fzaxmt_ring

#define FZA_RMC_OWN	0x80000000

/* 
 * DEFZA SMT ring entry - for both transmit and receive
 * The buffer address will point to the packet memory for a packet
 * based SMT frame ( maximum size of frame ). The number of descriptors
 * is defined by adapter at init time 
 */ 
struct fzasmt_ring {
	u_long own;			/* bit 31 host own bit */
	u_long rmc;			/* RMC info */
	u_long buf_addr;		/* buffer address */
	u_long resev;			/* reserved */
};
#define FZASMTRING volatile struct fzasmt_ring

#define FZA_HOST_OWN	0x80000000

#define FZACMD_PHY_ADDR	0x200400
#define FZAUNS_PHY_ADDR	0x200800

#define NFZACMD	64
#define NFZAUNS 64 

/*
 * DEFZA command ring entry and unsolicited ring entry
 */
struct fzacmd_uns_ring {
	unsigned 	cmdid:31;	/* command id */
	unsigned	own:1;		/* OWN bit */
	u_long status_id;		/* cmd status or event id */
	u_long buf_addr;		/* buffer address */
	u_long resev;			/* reserved */
};	

#define FZACMDRING 	volatile struct fzacmd_uns_ring
#define FZAUNSRING 	volatile struct fzacmd_uns_ring


/*
 * Unsolicited enevt
 */
#define UNS_UNDEFINED		0
#define UNS_RINGINIT		1 
#define UNS_RINGINITRCV		2
#define UNS_RINGBEACONINIT	3
#define UNS_DUPADDR		4
#define UNS_DUPTOKEN		5
#define UNS_RINGPURGEERR	6
#define UNS_BRIDGESTRIPERR	7
#define UNS_RINGOPOSC		8
#define UNS_DIRECTEDBEACON	9
#define UNS_PCINIT		10
#define	UNS_PCRCV		11
#define	UNS_XMT_UNDERRUN 	12
#define	UNS_XMT_FAILURE		13
#define	UNS_RCV_OVERRUN		14


#define RMC_PBC_MASK	0x00001fff	/* for frame length */


/* 
 * definition for the RMC Receive Completion Code
 */
#define FZA_RCV_RCC     	0x003fe000	/* rmc rcc bit 21 - 13 */
#define FZA_RCV_ERROR   	0x00200000	/* bit 21 indicate an error */
#define FZA_RCV_OVERRUN		0x00352000	/* frame too long */
#define FZA_RCV_INTERFACE_ERR 	0x00354000	/* rmc/mac interface error */ 
#define FZA_RCV_RCC_C		0x00100000	/* block check error */ 
#define FZA_RCV_FSB_E		0x04000000	/* frame statues error */
#define FZA_RCV_FSC		0x38000000	/* frame status counter 29-27 */
#define FZA_RCV_RCC_rrr		0x000e0000	/* rmc rrr field 19 - 17 */
#define FZA_RCV_RCC_NORMAL 	0x00000000
#define FZA_RCV_RCC_SA_MATCHED  0x00020000
#define FZA_RCV_RCC_DA_NOMATCHED  0x00040000
#define FZA_RCV_RCC_RMC_ABORT	0x00060000
#define FZA_RCV_RCC_INVALID_LEN 0x00080000		
#define FZA_RCV_RCC_FRAGMENT	0x000A0000
#define FZA_RCV_RCC_FORMAT_ERR	0x000C0000
#define FZA_RCV_MAC_RESET	0x000E0000

/*
 * definition for the RMC Transmit Descriptor  
 */
#define FZA_SOP			0x80000000	/* start of packet */
#define FZA_EOP 		0x40000000	/* end of packet */

#define FZA_XMT_DTP		0x20000000	/* discard this packet */
#define FZA_XMT_VBC		0x10000000	/* valid buffer byte count */
#define FZA_XMT_DCC		0x0f000000	/* DCC bit 27 - 24 */
#define FZA_XMT_SUCCESS		0x01000000	/* XMT successed */
/*
 * Packet Request Header
 */
#define FZA_PH0		0x00000020		/* packet request header */
#define FZA_PH1		0x00003800		/* packet request header */
#define FZA_PH2		0x00000000		/* packet request header */

/*
 * other TXM error information need to be defined XXXXXX
 */

/* 
 * FZA counter block (quad-word values). 
 */
struct 	_fzactrs {
		struct		fzactr_ent		sysbuf;
		struct		fzactr_ent		xmt_underrun;
		struct		fzactr_ent		xmt_fail;
		struct		fzactr_ent		rcv_overrun;
		struct		fzactr_ent		frame_count;
		struct		fzactr_ent		error_count;
		struct		fzactr_ent		lost_count;
		struct		fzactr_ent		ring_init_init;
		struct		fzactr_ent		ring_init_rcv;
		struct		fzactr_ent		ring_beacon_init;
		struct		fzactr_ent		dup_addr_fail;
		struct		fzactr_ent		dup_token;
		struct		fzactr_ent		ring_purge_err;
		struct		fzactr_ent		bridge_strip_err;
		struct		fzactr_ent		trace_init;
		struct		fzactr_ent		trace_rcvd;
		struct		fzactr_ent		lem_rej;
		struct		fzactr_ent		tne_exp_rej;
		struct		fzactr_ent		lem_event;
		struct		fzactr_ent		lct_rej;
		struct		fzactr_ent		connection;
		struct		fzactr_ent		elasticity_buf_err;
};	


struct fzamla {
		u_char	addr[4];
};	
struct fzacam {
	struct fzactr_ent addr[64];
};

/*
 * DEFZA command buffer formats. Command buffers are KM_ALLOC'd and cast
 * to the appropriate union member. 
 */
union fzacmd_buf {
		/*
  		 * DEFZA init command block
 		 */
		struct fzainit {
			long		xtm_mode;  /* 0/1 = 512/1024 entries */
			long		rcv_entry;	/* 2 - 256 */	
			struct 		_fzactrs  fzactrs;		
			char		pmc_rev[4];
			char		phy_rev[4];
			char		fw_rev[4];
			long		mop_type;
			u_long		rcvbase_addr;
			u_long		xmtbase_addr;
			long		xmt_entry;
			u_long		smtxmt_addr;				
			long		smtxmt_entry;
			u_long		smtrcv_addr;
			long		smtrcv_entry;
			char		mla[8];
			long		def_t_req;
			long		def_tvx;
			long		def_t_max;
			long		lem_threshold;
			struct		fzactr_ent def_station_id;
			long		pmd_type;
			long		smt_version;
		} fzainit;

		/*
		 * DEFZA PARAM command
		 */
		struct fzaparam {
			long		loop_mode;
			long		t_max;
			long		t_req;
			long		tvx;
			long		lem_threshold;
			struct		fzactr_ent station_id;
		} fzaparam;
		/*
		 * DEFZA RDCAM command
		 */
		struct	_fzactrs 	rdcnt;

		/*
		 * DEFZA SETCHAR Command
		 */
		struct fzasetchar {
			long		t_max;
			long		t_req;
			long		tvx;
			long		lem_threshold;
		} fzasetchar;
		
		/* 
  		 * DEFZA MODPROM Command
		 */
		struct fzamodprom {
			long 	llc_prom;
			long	smt_prom;
		} fzamodprom;

		/*
		 * DEFZA RDCAM and MODCAM Command 
		 */ 
		struct 	fzacam	fzacam_add;

		/*
		 * DEFZA STATUS Command
		 */
		struct fzastatus {
			long	led_state;
			long	rmt_state;
			long	link_state;
			long	dup_add_test;
			long	ring_purge_state;	
			u_long	neg_trt;
			u_char 	upstream[8];
			long    una_timed_out;
			long	frame_strip_mode;
			long	claim_token_mode;
			long	phy_state;
			long	neighbor_phy_type;
			long	rej_reason;
			long	phy_link_error;
			u_char	old_una_address[8];
			long	remote_mac_ind;
			long 	ring_error;
			u_char  dir_beacon[8];
		} fzastatus;
};
	
#define FZACMD_BUF volatile union fzacmd_buf
/*
 * Command definition 
 */
#define	CMD_NOP 	0x00000000
#define	CMD_INIT 	0x00000001
#define CMD_MODCAM	0x00000002 
#define CMD_PARAM	0x00000003
#define CMD_MODPROM	0x00000004
#define CMD_SETCHAR 	0x00000005
#define CMD_RDCNTR	0x00000006
#define CMD_STATUS 	0x00000007
#define CMD_RDCAM	0x00000008

/*
 * Command Status Field Information
 */
#define CMD_SUCCESS		0x00000000	/* success */
#define CMD_STATE_INVALID	0x00000001	/* invalid state */
#define CMD_XTM_INVALID 	0x00000002	/* transmit_mode invalid */
#define CMD_RCVENT_INVALID 	0x00000003	/* host_rcv_entries invalid */
#define CMD_LOOPBK_INVALID	0x00000004	/* lookback mode invalid */
#define CMD_TMAX_INVALID	0x00000005	/* T_MAX Invalid */
#define CMD_TREQ_INVALID	0x00000006	/* T_REQ Invalid */
#define CMD_TVX_INVALID		0x00000007	/* TVX Invalid */
#define CMD_LEM_INVALID		0x00000008	/* LEM_THRESHOLD Invalid */
#define CMD_STATION_ID_INVALID	0x00000009	/* STATION_ID Invalid */
#define CMD_CMD_INVALID		0x0000000a	/* Command Invalid */
#define CMD_LLCPROM_INVALID	0x0000000b	/* llc promisc. mode invalid */
#define CMD_SMTPROM_INVALID	0x0000000c	/* smt promisc. mode invalid */

/*
 * LOOP BACK MODE
 */
#define LOOP_NORMAL	0
#define LOOP_INTER	1
#define LOOP_EXTER	2

/*
 * Maximum limit reset number
 */
#define FZAMAXRESET	5	
 
/*
 * FRAME TYPE
 */
#define FZA_LLC	 1
#define FZA_SMT  0

/*
 * for FZA debug 
 */
#define	FZA_DEB_PROBE		0x0001
#define FZA_DEB_ATTACH		0x0002
#define FZA_DEB_INTR		0x0004
#define FZA_DEB_XMT		0x0008
#define FZA_DEB_RCV		0x0010

#define FZADEBUG	1

#define PRT_REG(sc) 	\
{ printf("Register set: reset 0x%x ctl_a 0x%x ctl_b 0x%x \n",	\
sc->reg_reset,sc->reg_ctla,sc->reg_ctlb);	\
printf("intr_enevt 0x%x intr_mask 0x%x status 0x%x \n",	\
sc->reg_intr,sc->reg_mask,sc->reg_status);	\
}	

#define PRT_MLA(p)	\
printf("mal is %02x-%02x-%02x-%02x-%02x-%02x \n",	\
p[0],p[1],p[2],p[3],p[4],p[5],p[6]);	

#define PRT_RINGADDR(sc)	\
{ 	\
printf("Ring address rring 0x%x ", sc->rring);	\
printf("tring 0x%x smttring 0x%x \n",sc->tring,sc->smttring);	\
printf("smtrring 0x%x  cmdring 0x%x",sc->smtrring,sc->cmdring);	\
printf("unsring 0x%x \n",sc->unsring);	\
}	

#define PRT_SC(sc)	\
{ 	\
printf("fzasoftc struct: Base reg 0x%x \n", sc->basereg); \
PRT_RINGADDR(sc);	\
printf("\n Other parameters : ",sc->unsring);	\
printf("nrmcxmt %d nsmtxmt %d nsmtrcv %d\n",sc->nrmcxmt,sc->nsmtxmt,sc->nsmtrcv); \
PRT_MLA(sc->is_dpaddr); \
}	

#define FZA_DEBUG(flags,routine,text)	\
if( fza_code_debug & (flags)) {		\
printf(text,routine); }			

#define FZA_DEBUG1(flags,routine,text,data1)  \
if( fza_code_debug & (flags)) {		\
printf(text,routine,(data1));  }		

#define FZA_DEBUG2(flags,routine,text,data1,data2)  \
if( fza_code_debug & (flags)) {		\
printf(text,routine,(data1),(data2));  }		

#define FZA_DEBUG3(flags,routine,text,data1,data2,data3)  \
if( fza_code_debug & (flags)) {		\
printf(text,routine,(data1),(data2),(data3));  }		

#define FZA_DEBUG4(flags,routine,text,data1,data2,data3,data4)  \
if( fza_code_debug & (flags)) {		\
printf(text,routine,(data1),(data2),(data3),(data4));  }		

#define FZA_DEBUG5(flags,routine,text,data1,data2,data3,data4,data5)  \
if( fza_code_debug & (flags)) {		\
printf(text,routine,(data1),(data2),(data3),(data4),(data5));  }
