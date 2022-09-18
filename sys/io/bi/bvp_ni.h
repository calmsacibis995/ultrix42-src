/*	@(#)bvp_ni.h	4.1	ULTRIX	7/2/90	*/
/************************************************************************
 *									*
 *			Copyright (c) 1986,87 by			*
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
 *
 * 15-Jan-88 	lp
 *	Couple of changes caused by rewrite of if_ni.c for large
 *	mbufs.
 *
 * 12-11-87	Robin L. and Larry C.
 *	Added portclass support to the system and KMALLOC changes.
 *
 * 09-Dec-87   -- templin@decvax
 *			Added Larry Palmer's fix to change the size of
 *			the text portion of the ni_msg struct to comply
 *			with the workings of the new kernel memory allocator.
 *
 * 25-Aug-87   -- templin@decvax
 *			Added the PS_FQE bit definition to mask off the
 *			Free Queue Exhausted bit.
 * 07-July-87  -- templin@decvax
 *			Changed fields of the "ni_counters" struct from
 *			"unsigned long" to "unsigned short field_name[2]"
 *			to fix a word-ordering problem.
 */

struct _gvpbq {
	unsigned long flink;
	unsigned long blink;
};
/*
 * Free queue block - Message, xmit datagram, rec. datagram,
 *	immediate datagram (for debug only).
 */
struct _fqb {
	long mfreeq_size;
	unsigned :32;
	struct _gvpbq mfreeq;
	long dfreeq_size;
	unsigned :32;
	struct _gvpbq dfreeq;
	long rfreeq_size;
	unsigned :32;
	struct _gvpbq rfreeq;
#ifdef notdef
	long ifreeq_size;
	unsigned :32;
	struct _gvpbq ifreeq;
#endif
};

/* 
 * ni specific portion of VaxPort data structure.
 */
struct _nipqb {
	struct piv {
		unsigned level:4;
		unsigned vector:12;
		unsigned bi_node:16;
	} piv;
	unsigned long	num_freeq;
	struct _fqb *vfqb_base;
	unsigned long reserved1[39];
	unsigned long	bvp_level;
	caddr_t	vpqb_base;
	caddr_t bdt_base;
	unsigned bdt_len:16;
	unsigned :16;
	unsigned long spt_base;
	unsigned spt_len:22;
	unsigned :10;
	unsigned long gpt_base;
	unsigned gpt_len:22;
	unsigned :10;
	unsigned long func_mask;
	unsigned long reserved2[6];
	unsigned short ad_max_dg;
	unsigned short ad_max_msg;
	long  ad_sw_type;
	long  ad_sw_vers;
	long  ad_hw_type;
	long  ad_hw_vers[3];
	long qe_logout[54];
};

/* 
 * BVP buffer descriptor
 */
struct _bd {
	unsigned offset:9;
	unsigned :3;
	unsigned ac:1;
	unsigned am:2;
	unsigned valid:1;
	unsigned key:16;
	unsigned long buf_len;
	struct pte *pt_addr;
	unsigned :32;
};

/*
 * how queues are referenced
 */
#define freeq0 ni_pqb->ni.vfqb_base->mfreeq
#define freeq1 ni_pqb->ni.vfqb_base->dfreeq
#define freeq2 ni_pqb->ni.vfqb_base->rfreeq
#ifdef notdef
#define freeq3 ni_pqb->ni.vfqb_base->ifreeq
#endif
#define comq0  ni_pqb->cmdq0
#define respq  ni_pqb->rspq
 
/*
 * Sizes of various queues
 */

/* Number of xmit + rcv buffers */
#define NI_NBUF 84
#define NI_NFREEQ 3
#define NI_FREEQ_0 4 /* message queue entries */
#define NI_FREEQ_1 NI_NBUF /* data queue entries - errors*/
#define NI_FREEQ_2 NI_NBUF/2 /* All other rcv datagrams */
#ifdef notdef
#define NI_FREEQ_3 NI_NBUF /* All other rcv datagrams */
#endif
#define NI_NUMBUF 12	/* # of buffer names/dgsnd */
#define NI_RBUF 2	/* # of buffer names/dgrec */
#define NI_NRECV NI_FREEQ_1+NI_FREEQ_2
#define NI_MAXPACKETSZ	1560-sizeof(struct ether_header)
#define NI_IPL 0x17

#define NI_MAXITRY 100000 /* How many times to try an insqti */

/*
 * Message
 */
struct ni_msg {
	unsigned long flink;
	unsigned long blink;
	unsigned :32;
	unsigned :8;
	unsigned status:8;
	unsigned opcode:8;
	unsigned R:1;
	unsigned :7;
	unsigned msg_len:16;
	unsigned ni_opcode:8;
	unsigned ni_status:8;
	unsigned :32;
	char text[128]; /* Total here must be <= AD_MAX_MSG */
};

/*
 * Datagram 
 */
struct ni_data {
	unsigned long flink;
	unsigned long blink;
	unsigned :32;
	unsigned :8;
	unsigned status:8;
	unsigned opcode:8;
	unsigned R:1;
	unsigned :7;
	unsigned dg_len:16;
	unsigned dg_status:16;
	unsigned mbuf_tofree;
	unsigned long dg_ptdb_index;
	struct chbufs {
	unsigned offset:16;
	unsigned s_len:16;
	unsigned bdt_index:15;
	unsigned chain:1;	
	unsigned buffer_key:16;
	} cbufs[NI_NUMBUF];
};

/*
 * Immediate datagram 
 */
struct ni_dgi {
	unsigned long flink;
	unsigned long blink;
	unsigned :32;
	unsigned :8;
	unsigned status:8;
	unsigned opcode:8;
	unsigned R:1;
	unsigned :7;
	unsigned dg_len:16;
	unsigned :16;
	unsigned :32;
	unsigned long dg_ptdb_index;
	char text[100]; /* Total here must be <= AD_MAX_MSG */
};
/* Status bits for data & msg */
#define PCK_FAIL 	1
#define NMULTI 		12

/*
 * Protocol type definition block 
 */
struct ptdb {
	unsigned ptt:16;
	unsigned fq_index:8;
	unsigned :2;
	unsigned flags:6;
	unsigned long ptdb_index;
	unsigned short adr_len;
	unsigned short info_802;
	struct multi {
		u_char addr[8];
	} multi[NMULTI];
};

/* Flags in ptdb struct */
#define PTDB_UNQ  0x01
#define PTDB_PROM 0x02
#define PTDB_802  0x04
#define PTDB_BDC  0x08
#define PTDB_UNK  0x10
#define PTDB_AMC  0x20


#define NI_MQSIZE sizeof(struct ni_msg)
#define NI_DQSIZE sizeof(struct ni_data)
#define NI_MQHEAD 18 /* 4 longs+2 make up the header */
#define NI_DQHEAD NI_MQHEAD
#define NI_DGRLEN 10+8*NI_RBUF 

/*
 * PC status bits
 */
#define PS_STATEMASK    0x00070000
#define PS_UNDEFINED 	0x00010000
#define PS_INITIALIZED 	0x00020000
#define PS_ENABLED 	0x00040000
#define PS_STOPPED 	0x00060000
#define PS_MAINT 	0x00070000
#define PS_OWN 		0x80000000
#define PS_FQE 		0x02000000
#define PS_RSQ 		0x00000080
#define PS_SUME		0x00000040

/*
 * PC status bits
 */
#define PC_NULL 	0x00000000
#define PC_INIT 	0x00000001
#define PC_ENABLE 	0x00000002
#define PC_READPIV 	0x00000003
#define PC_SHUTDOWN 	0x00000004
#define PC_MAINT 	0x00000005
#define PC_CMDQNE 	0x00000006
#define PC_FREEQNE 	0x00000007
#define PC_PSQ 		0x00000008
#define PC_ENAERR 	0x00000009
#define PC_DISERR 	0x0000000a
#define PC_RESTART 	0x0000000b
#define PC_OWN 		0x00000080
#define PC_DATAMASK 	0xffffff00
#define PC_MFREEQ 	0x00000000
#define PC_DFREEQ 	0x00000100
#define PC_RFREEQ 	0x00000200
#define PC_IFREEQ 	0x00000300
#define PC_CMDQ0  	0x00000000
#define PC_CMDQ1  	0x00000100
#define PC_CMDQ2  	0x00000200
#define PC_BUSY		0x00000001

/*
 * BVP opcodes
 */
#define SNDDG  0x01
#define SNDMSG 0x02
#define SNDDGI 0x03
#define DGSNT  0x01
#define MSGSNT 0x02
#define DGISNT 0x03
#define DGREC  33
#define MSGREC 34
#define DGIREC 35

/*
 * NI opcodes
 */
#define NIOP_NOP	0
#define NIOP_WSYSID	1
#define NIOP_RSYSID	2
#define NIOP_WPARAM	3
#define NIOP_RPARAM	4
#define NIOP_RCCNTR	5
#define NIOP_RDCNTR	6
#define NIOP_STPTDB	7
#define NIOP_CLPTDB	8

/*
 * ni parameter structure
 */
struct ni_param {
	unsigned char dpa[8];
	unsigned char apa[8];
	unsigned char lsa[8];
	unsigned char bvc[8];
	unsigned short cur_adr;
	unsigned short max_adr;
	unsigned short cur_ptt;
	unsigned short max_ptt;
	unsigned short cur_fq;
	unsigned short max_fq;
	unsigned long sid_interval;
	unsigned long mop_stuff;
	unsigned flags:8;
	unsigned :24;
	unsigned rec_timeo;
	unsigned xmit_timeo;
};

/* param flags */
#define NI_ECT	0x01
#define NI_PAD	0x02
#define NI_BOO	0x04
#define NI_CAR	0x08
#define NI_ILP  0x10
#define NI_ELP	0x20
#define NI_DCRC	0x40
#define NI_THRU 0x80

/*
 * Where PC,PS,PE,PD are for ni
 */
#define NI_NI_ADDR	0x204

/*
 * Structure for counters
 */
struct ni_counters {
	unsigned short   last_zero;		/* Last zeroed */
	unsigned short	bytes_rec[2];		/* bytes recieved */
	unsigned short	bytes_snt[2];		/* bytes sent */
	unsigned short	frame_rec[2];		/* frames recieved */
	unsigned short	frame_snt[2];		/* frames sent */
	unsigned short	mbytes_rec[2];		/* multicast bytes rec. */
	unsigned short	mframe_rec[2];		/* multicast frames rec. */
	unsigned short	mbytes_snt[2];		/* multicast bytes sent */
	unsigned short	mframe_snt[2];		/* multicast frames sent */
	unsigned short	fs_def[2];		/* frames sent initially deferred */
	unsigned short	fs_sc[2];		/* "	  "    single collision */
	unsigned short	fs_mc[2];		/* "      "    multiple " */
	unsigned short	sfail;		/* send failures */
	unsigned short  sfbm;		/* send failures BM */
	unsigned short  rfail;		/* recv failures */
	unsigned short	rfbm;		/* recv failures BM */
	unsigned short	unrec;		/* unrecog destination */
	unsigned short	datao;		/* data overrun */
	unsigned short	sbu;		/* system buffer unavail. */
	unsigned short	ubu;		/* user buffer unavail. */
	unsigned short	ctf;		/* collision test failed */
	unsigned short  junk;
	unsigned long lance[16];			/* lance info */
};

/* What inline returns if a queue transition happened */
#define QEMPTY 0


/*
 *  Lifted from Todd's vpscs.h header file. This part is temporary
 *
 *   Facility:	Systems Communication Architecture
 *		Generic Vaxport Port Driver
 *
 *   Abstract:	This module contains all of the generic Vaxport port driver
 *		data structure definitions visible to the SCS layer of SCA.
 *
 *   Creator:	Todd M. Katz	Creation Date:	November 20, 1985
 *
 *   History:
 *
 */

/* Generic Vaxport Data Structure Definitions.
 */
typedef struct _gvppqb	{		/* Vaxport Port Queue Block	     */
    struct _gvpbq cmdq0;		/* Command queue head priority 0     */
    struct _gvpbq cmdq1;		/* Command queue head priority 1     */
    struct _gvpbq cmdq2;		/* Command queue head priority 2     */
    struct _gvpbq cmdq3;		/* Command queue head priority 3     */
    struct _gvpbq rspq;			/* Response queue head		     */
#ifdef notdefyet
    union	{			/* Implementation dependent fields   */
	struct _cipqb	ci;		/*  CI specific fields of PQB	     */
#endif
	struct _nipqb   ni;		/* NI specific fields of PQB          */
#ifdef notdefyet
	} type;	
#endif
    } GVPPQB;
/* End of temporary part */
