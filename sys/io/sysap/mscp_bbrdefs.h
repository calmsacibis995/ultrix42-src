/*
 *	@(#)mscp_bbrdefs.h	4.1	(ULTRIX)	7/2/90
 */

/************************************************************************
 *                                                                      *
 *                      Copyright (c) 1987, 1988 by                     *
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
 *		MSCP-speaking Class Drivers
 *
 *   Abstract:	This module contains the MSCP Bad Block Replacement
 *		definitions.
 *
 *   Author:	David E. Eiche	Creation Date:	November 15, 1987
 *
 *   History:
 *
 *	27-Jul-1988	Pete Keilty
 *		Added logerr flag to the flags word & MSLG structure
 *		to struct _bbrb both used for errlogging.
 *
 */
/**/

/* BBR state stack manipulation macros.
 */
#define Push_bbr_state( new_state ) { \
    if( bbrp->stack_depth >= 6 ) \
	panic( "mscp_bbr_xxx: stack overflow\n" ); \
    bbrp->stack[ bbrp->stack_depth++ ] = rp->state; \
    rp->state = new_state; }

#define Pop_bbr_state() { \
    if( bbrp->stack_depth == 0 ) \
	panic( "mscp_bbr_xxx: stack underflow\n" ); \
    rp->state = bbrp->stack[ --bbrp->stack_depth ]; }

/* Replacement control table sector 0 format
 */
typedef struct {
    u_long		volser[2];	/* Volume serial number		     */
    u_short		flags;		/* BBR status flags		     */
    u_short			   :16;	/* Reserved			     */
    u_long		lbn;		/* LBN of replaced block	     */
    u_long		rbn;		/* Replacement block number	     */
    u_long		badrbn;		/* RBN of replaced (bad) RBN	     */
    u_char		s0_rsvd[488];	/* Reserved			     */
} RCT_SECTOR_0;

typedef struct {
    u_char		data[512];	/* Temporary data storage	     */
} RCT_SECTOR_1;

/* RBN descriptor format
 */
typedef struct {
    u_long		lbn	   :28;	/* Revectored LBN		     */
    u_long		code	   :4;	/* Descriptor code		     */
} RCT_DESC;

/* Replacement control table sector k format (where 2 <= k < rct size)
 */
typedef struct {
    RCT_DESC		desc[128];	/* RCT descriptor vector	     */
} RCT_SECTOR_K;

/* RCT sector 0 flags word definitions
 */
#define RCT_S0_FE		0x0080	/* Forced error			     */
#define RCT_S0_BR		0x2000	/* Bad replacement block	     */
#define RCT_S0_P2		0x4000	/* BBR phase 2 in progress	     */
#define RCT_S0_P1		0x8000	/* BBR phase 1 in progress	     */

/* RCT descriptor code definitions
 */
#define RCT_DS_UNALLOC		0x00	/* Unallocated RB		     */ 
#define RCT_DS_PRIMARY		0x02	/* Primary RB			     */
#define RCT_DS_NONPRIM		0x03	/* Nonprimary RB		     */
#define RCT_DS_UNUSABL		0x04	/* Unusable RB			     */
#define RCT_DS_UNUSABLALT	0x05	/* Unusable RB alt. code	     */
#define RCT_DS_NULL		0x08	/* NULL sentinel entry		     */

/* BBR status flags definitions
 */
#define BBR_FL_FE		0x0001	/* Write LBN with forced error	     */
#define BBR_FL_ERROR		0x0002	/* Error during stress testing	     */
#define BBR_FL_MATCH		0x0004	/* (S) Matching LBN seen	     */
#define BBR_FL_NONPRIM		0x0008	/* (S) Descriptor is nonprimary	     */
#define BBR_FL_EOT		0x0010	/* (S) Table end encountered	     */
#define BBR_FL_DONE		0x0020	/* (S) Reserved			     */
#define BBR_FL_P2RECOV		0x0040	/* Recovering from phase 2 error     */
#define BBR_FL_BUSY		0x0080	/* BBR active on unit		     */
#define BBR_FL_TRANS		0x0100	/* Block was not replaced	     */
#define BBR_FL_FULL		0x0200	/* RCT full			     */
#define BBR_FL_RCTCOR		0x0400	/* RCT corrupt			     */
#define BBR_FL_RECURS		0x0800	/* RCT recursion failure	     */
#define BBR_FL_RPLFL		0x1000	/* REPLACE failed		     */
#define BBR_FL_FORCE		0x2000	/* Force replacement (via radisk)    */
#define BBR_FL_MWFAIL		0x4000	/* Multi-write failed		     */
#define BBR_FL_MRFAIL		0x8000	/* Multi-read failed		     */
#define BBR_FL_RPLATT		0x10000	/* RCT was modified		     */

/* Misc Defines
 */
#define BBR_BLOCKSIZE		512

typedef struct _bbrb {
    struct {				/* Queue of requests waiting for     */
	REQB		*flink;		/*  BBR service			     */
	REQB		*blink;		/*  ...				     */
    } bbr_wq;				/*  ...				     */
    REQB		*cur_reqb;	/* Current request being serviced    */
    u_long		lbn;		/* LBN on which error occurred	     */
    u_long		rbn;		/* RBN of replacement block	     */
    union {				/* Status flags			     */
	u_long		mask;		/*    accessed as a mask	     */
	struct {			/* Accessed as individual bits	     */
	    u_long	fe	   :1;	/* Write LBN with forced error	     */
	    u_long	error	   :1;	/* Error during stress testing	     */
	    u_long	match	   :1;	/* (S) Matching LBN seen	     */
	    u_long	nonprim	   :1;	/* (S) Desdcriptor is nonprimary     */
	    u_long	eot	   :1;	/* (S) Table end encountered	     */
	    u_long	done	   :1;	/* (S) Reserved			     */
	    u_long	p2recov	   :1;	/* Recovering from phase 2 error     */
	    u_long	busy	   :1;	/* BBR active on connection	     */
	    u_long	trans	   :1;	/* Block was transient error	     */
	    u_long	full	   :1;	/* RCT full condition		     */
	    u_long	corrupt	   :1;	/* RCT corrupt			     */
	    u_long	recurs	   :1;	/* RCT recursion error		     */
	    u_long	repfail	   :1;	/* REPLACE failed		     */
	    u_long	force	   :1;	/* Force replacement (via radisk)    */
	    u_long	mwfail	   :1;	/* Failure during multi-write	     */
	    u_long	mrfail	   :1;	/* Failure during multi-read	     */
	    u_long	rplatt	   :1;	/* Replacement attempted	     */
	    u_long	logerr	   :1;	/* logging error packet to errlog */
	    u_long		   :14;	/* Reserved			     */
	} bit;
    } flags;
    char		recursion_ct;	/* Replacement recursion counter     */
    char		loop_ct1;	/* Loop/retry counter		     */
    char		loop_ct2;	/* Loop/retry counter		     */
    char		copy_ct;	/* (M) Multi-read/write copy counter */
    char		bad_copies;	/* (M) Multi-write bad copy counter  */
    u_char		*multi_buf;	/* (M) Multi-read/write buffer ptr   */
    u_long		hash_rbn;	/* (S) Primary descriptor RBN	     */
    u_long		hash_block;	/* (S) Primary descriptor RCT block  */
    u_long		hash_offset;	/* (S) Primary descriptor RCT offset */
    u_long		cur_rbn;	/* (S) Current RBN being searched    */
    u_long		cur_block;	/* (S) Current RCT block 	     */
    u_long		match_rbn;	/* (S) Allocated RB that matches LBN */
    u_long		max_host_rbn;	/* (S) Maximum host RBN		     */
    RCT_DESC		prev_desc;	/* Previous RBN descriptor	     */
    u_long		stack_depth;	/* State stack depth		     */
    u_long		stack[6];	/* State stack			     */
    u_char		buf0[512];	/* Buffer 0 - Usually RCT page 0     */
    u_char		buf1[512];	/* Buffer 1 - Usually RCT page 1     */
    u_char		buf2[512];	/* Buffer 2 - utility buffer	     */
    u_char		buf3[512];	/* Buffer 3 - utility buffer	     */
    REQB		bbr_reqb;	/* Request block for BBR operations  */
    MSLG		bbr_mslg;	/* Last mslg datagram packet      */
    struct buf		bbr_buf;	/* Buf structure for BBR operations  */
} BBRB;
