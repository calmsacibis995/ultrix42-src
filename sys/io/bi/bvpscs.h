/*
	@(#)bvpscs.h	4.3	(ULTRIX)	10/11/90	
*/

/************************************************************************
 *									*
 *			Copyright (c) 1987 by				*
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
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/
/**/

typedef struct _bvp_ssppccb {		/* BVP SSP specific fields of PCCB */

	struct	_gvpbq	dfreeq;		/* Datagram free queue head	*/
	struct	_gvpbq	mfreeq;		/* Message free queue head	*/
	struct	bvpregs	*port_regs;	/* Pointer to Port Registers	*/
	struct biic_regs *nxv;		/* VA of Adapter BIIC Regs	*/
	struct	_pb	*pb;		/* Pointer to Path Block	*/
	u_long		bvp_ctlr;	/* Controller number		*/
	int		binumber;	/* BI number			*/
	int		binode;		/* BI node number		*/
	struct	bidata	*bidata;	/* Pointer to bidata structure	*/
	u_long		cmd_pend;	/* Port command pending vector	*/
	u_long		ivec;		/* Interrupt vector		*/
	int		poll_rate;	/* Timer poll rate		*/
	u_long		port_state;	/* Current port state		*/
	int		rip;		/* Recovery in progress ind.	*/
	int		incarn;		/* Port incarnation number	*/
	} BVPSSPPCCB;



typedef struct _bvp_ssppqb {		/* BVP SSP specific fields of PQB */

	struct _gvpbq	*dfreeq_hdr;	/* Datagram free queue head pointer */
	struct _gvpbq	*mfreeq_hdr; 	/* Message free queue head pointer  */
	u_long		dqe_len	: 16;	/* Datagram queue entry length	*/
	u_long			: 16;	/* MBZ				*/
	u_long		mqe_len	: 16;	/* Message queue entry length	*/
	u_long			: 16;	/* MBZ				*/
	struct _gvppqb	*vpqb_base;	/* PQB system virtual address	*/
	struct _gvpbd	*bdt_base;	/* BDT system virtual address	*/
	u_short		bdt_len;	/* BDT octaword length		*/
	u_short			: 16;	/* MBZ				*/
	struct pte	*spt_base;	/* System page table physical address*/
	u_long		spt_len	: 22;	/* SPT longword length		*/
	u_long			: 10;	/* MBZ				*/
	struct pte	*gpt_base;	/* Global page table physical address*/
	u_long		gpt_len	: 22;	/* GPT longword length		*/
	u_long			: 10;	/* MBZ				*/
	u_long		keep_alive;	/* Keep-alive timer		*/
	u_long		function_mask;	/* Port charateristics mask	*/
	u_long		piv;		/* Port Interrupt Vector	*/
	u_long		bvp_level;	/* BVP funtionality level	*/
	u_char		reserved1[ 32 ];/* Reserved			*/
	u_long		pd_prtvrs  :8;	/* Port driver PPD version	*/
	u_long		reserved2  :24;	/* Reserved			*/
	u_long		pd_max_dg  :16;	/* Maximum datagram size - port	*/
	u_long		pd_max_msg :16;	/* Maximum message size - port	*/
	u_long		pd_sw_type;	/* Operating system "U-32" 	*/
	u_long		pd_sw_version;	/* Operating system version	*/
	u_long		pd_hw_type;	/* Port hardware type		*/
	u_dodec		pd_hw_version;	/* Port hardware version	*/
	u_quad		pd_cur_time;	/* Current time			*/
	u_char		reserved4[ 24 ];/* Reserved			*/
	u_long		ad_prtvrs  :8;	/* Adapter PPD protocol version	*/
	u_long		ad_type	   :8;	/* Adapter port type		*/
	u_long		reserved3  :16;	/* Reserved			*/
	u_long		reserved6;	/* Reserved			*/
	u_long		ad_max_dg  :16;	/* Maximum datagram size - adap	*/
	u_long		ad_max_msg :16;	/* Maximum message size - adap	*/
	u_long		ad_sw_type;	/* Adapter software type	*/
	u_long		ad_sw_version;	/* Adapter software version	*/
	u_long		ad_hw_type;	/* Adapter hardware type	*/
	u_dodec		ad_hw_version;	/* Adapter harware version	*/
	u_char		reserved5[ 24 ];/* Reserved			*/
	struct	_gvph	*qe_logout[ BVP_NOLOG ];/* Queue Entry logout area */
	} BVPSSPPQB;

