/*
 * @(#)ga.h	4.2	(ULTRIX)	7/17/90
 */
/************************************************************************
 *									*
 *			Copyright (c) 1990 by				*
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
#ifndef _GA_H_
/*
 *	$Header: /local/4.0-12/io/tc/RCS/ga.h,v 1.15 90/05/07 11:25:09 fhsu Exp Locker: jensen $
 */
#define _GA_H_

/* define ONLY ONE of these, please... */
#undef  GA_ALLOC_KM			/* KM_ALLOC */
#define GA_ALLOC_DT			/* static data */
#undef  GA_ALLOC_TX			/* text */

#define _4K	(1<<12)
#define _8K 	(1<<13)
#define _32K 	(1<<15)
#define _64K 	(1<<16)
#define _96K 	(_32K +_64K)
#define _128K 	(1<<17)

#define GA_CONSIZ _8K

/*
 * Interrupt status bits: these bits are set in in gx_info 'intr_status'
 * field.
 *
 * INTR_ACTIVE is set by the server when it submits a packet to the STIC.
 * As long as it is set it means that there is at least one packet which
 * has not yet completed (and thus that the interrupt service routine can
 * be expected to be entered at some point in the future.  This bit
 * is cleared by the isr when it becomes blocked or idle.
 *
 * INTR_BLOCKED is set by the isr when it encounters a microcode packet
 * which it cannot deal with (e.g. one which must be emulated by the
 * server).  It is cleared by the server immediately prior to queueing
 * a packet which the isr can deal with.
 * 
 * INTR_CLIP is set by the isr when it is sending off a packet which
 * has specified a cliplist.
 *
 * INTR_ERR is set by the isr when it detects an error.
 *
 *   Value of intr_status	Meaning
 *   --------------------	-------
 *	       0                Idle.  No packet completions pending.
 *             1                A packet will complete in the future.
 *             2                Idle because isr can't emulate ucode pkt.
 *             3                ILLEGAL.
 */
#define GA_INTR_ACTIVE	(1<<0)
#define GA_INTR_BLOCKED (1<<1)
#define GA_INTR_CLIP    (1<<2)
#define GA_INTR_ERR     (1<<3)
#define GA_INTR_NEEDSIG (1<<4)

#define GA_RBUF_SIZE		_96K	/* ring buffer size in bytes */

/*
 * Template for accessing various fields on the 2DA board.
 */
typedef struct _ga_map {
    int		stic_poll_reg;		/* 0x000000 */
    char	__pad1[0x180000-sizeof(int)];
    sticRegs	stic_reg;		/* 0x180000 */
    char	__pad6[0x200000-0x180000-sizeof(sticRegs)];
    bt459Regs	vdac_reg;		/* 0x200000 */
    char	__pad3[0x300000-0x200000-sizeof(bt459Regs)];
    int		rom[1<<18];		/* 0x300000 */
} gaMap;
#define GA_map		gaMap


/*
 * (GA_map *) to GA field ptr
 */
#define GA_POLL(G)		(& (((gaMap *)G)->stic_poll_reg) )
#define GA_STIC(G)		(& (((gaMap *)G)->stic_reg) )
#define GA_STAMP(G)		(((int)G) | 0xC0000)
#define GA_VDAC(G)		(& (((gaMap *)G)->vdac_reg) )
#define GA_ROM(G)		(((gaMap *)G)->rom)

#ifndef KERNEL

#define GAO_POLL(X)	GA_POLL (((gxInfo *)X)->gxo)
#define GAO_VDAC(X)	GA_VDAC (((gxInfo *)X)->gxo)
#define GAO_STIC(X)	GA_STIC (((gxInfo *)X)->gxo)
#define GAO_STAMP(X)	GA_STAMP(((gxInfo *)X)->gxo)
#define GAO_REQBUF(X,N)	(((gxInfo *)X)->rb_addr +3 +((GA_CMDBUF0_SIZE>>2)*(N)))
#define GAO_RWSPAN(X)	(((gxInfo *)X)->rb_addr +3 +(GA_IMAGE_BUFFER_OFF>>2))

#define GAO_SYS_TO_PHYS(X,A) \
    (((gxInfo *)X)->rb_phys \
     +((int)(A) -(3*sizeof(int)) -(int)(((gxInfo *)X)->rb_addr)))

#define GX_Info		gxInfo		/* backward compat. */
#define GX_POLL(X)	GAO_POLL(X)	/* should go away??? */
#define GX_VDAC(X)	GAO_VDAC(X)
#define GX_STIC(X)	GAO_STIC(X)
#define GX_STAMP(G)	GAO_STAMP(X)

#endif !kernel

/******************************
 * All definitions from here to EOF are related to servicing
 * packet-done interrupts
 ******************************/

/* sizes in bytes */
#define  WORD		(0x4)
#define  PAGE		(0x1000)
#define  HALF_PAGE	(PAGE >> 1)
#define  MEG		(0x100000)

#define  GA_CMDBUF0_OFF			  (0 * PAGE)
#define  GA_CMDBUF0_SIZE		(1 * PAGE)
#define  GA_CMDBUF1_OFF			  (1 * PAGE)
#define  GA_CMDBUF1_SIZE		(1 * PAGE)
#define  GA_IMAGE_BUFFER_OFF		  (2 * PAGE + 12)
#define  GA_IMAGE_BUFFER_SIZE		(3 * PAGE - 12)
#define  GA_COMAREA_SRVCOM_OFF		  (5 * PAGE + 4)
#define  GA_COMAREA_SRVCOM_SIZE		HALF_PAGE
#define  GA_2DCOM_OFF			  (5 * PAGE + HALF_PAGE)
#define  GA_2DCOM_SIZE			HALF_PAGE
#define  GA_CLIPLIST_OFF		  (6 * PAGE)
#define  GA_CLIPLIST_SIZE		(2 * PAGE)
#define  GA_INTR_BUFFER_OFF		  (8 * PAGE)
#define  GA_INTR_BUFFER_SIZE		(GA_QUEUE_PACKETS * PAGE)

#ifdef KERNEL
#define N_FLUSH               1
#define N_PASSPACKET          5
#define N_MAX_CLIPRECTS       8
#define N_MAX_CLIPLISTS      16
#define N_NO_CLIPLIST        (N_MAX_CLIPLISTS)
#define NUM_REQUIRED_CONTEXT  4
#define LINE_WIDTH_PER_PKT    1
#define XYMASK_PER_PKT        1
#endif KERNEL

/* 
 * Given the address of a PixelStamp packet, return the 
 * index of the first word of the 2-word cliprect field
 */
#ifdef N_MAX_CLIPRECTS			/* if kernel or server, then */
					/* include all this stuff... */
#ifdef notdef
#define DECODE_CLIP_INDEX(p)  ( NUM_REQUIRED_CONTEXT + \
	(((sticCmd *)(p))->LineWidthPer == LINE_WIDTH_PER_PKT) + \
       ((((sticCmd *)(p))->XYMask == XYMASK_PER_PKT) << 3) )
#endif notdef

#define DECODE_CLIP_INDEX(p)  ( NUM_REQUIRED_CONTEXT + \
	((((*p) & (1<<8))) >> 5) + ((((*p) & (1<<10))) >> 10) )

typedef struct _stic_cmd {
        unsigned Op                     :4;
        unsigned RGBMode                :2;
        unsigned ZMode                  :2;
        unsigned XYMask                 :2;
        unsigned LineWidthPer           :2;
        unsigned Reserved0              :7;
        unsigned ClipRect               :1;
        unsigned Reserved1              :1;
        unsigned Mesh                   :1;
        unsigned Reserved2              :1;
        unsigned AALine                 :1;
        unsigned Reserved3              :7;
        unsigned HSEquals               :1;
} sticCmd;

typedef struct {
    unsigned long minval;
    unsigned long maxval;
} gaStampClipRect;

typedef struct {
        long		numClipRects;
        long		refCount;
        gaStampClipRect	clipRects[N_MAX_CLIPRECTS];
} gaClipList;

#define GA_QUEUE_PACKETS 16
#define GA_LAST_QPACKET (GA_QUEUE_PACKETS-1)
#ifdef notdef /* varaiable depth for debugging only */
#    define GA_QDEPTH(p) ((p)->qDepth)
#    define GA_QLAST(p) ((p)->qDepth-1)
#else
#    define GA_QDEPTH(p) GA_QUEUE_PACKETS
#    define GA_QLAST(p) GA_LAST_QPACKET
#endif notdef

#ifdef notdef /* debug */
#define NEXT_BUF(p2d, i) ( ((i) == GA_LAST_PACKET) ? 0 : (i)+1 )
#define PREV_BUF(p2d, i) ( ((i) == 0 ) ? GA_LAST_PACKET : (i)-1 )
#define ONELESSTHAN(i1, i2, p2d) (((i2) == 0) ? (i1) == GA_LAST_PACKET : \
				        (i1) == (i2) - 1 )
#else
#define NEXT_BUF(p2d, i) ( ((i) == GA_QLAST(p2d)) ? 0 : (i)+1 )
#define PREV_BUF(p2d, i) ( ((i) == 0 ) ? GA_QLAST(p2d) : (i)-1 )
#define ONELESSTHAN(p2d, i1, i2) ( ((i2) == 0) ? (i1) == GA_QLAST(p2d) : \
				        (i1) == (i2) - 1 )
#endif notdef

/* defined here mainly to avoid dragging in N10 include files */
typedef struct _ga_Packet {
     union {
	struct {
	    long opcode;
	} un;
	struct {
	    long opcode;
	    long word_count;
	    long cliplist_sync;
	    long data[1];
	} PassPacket;
	struct {
	    long opcode;
	    long word_count;
	    long sram_phys_addr;
	    long r3000_virt_addr;
	} ReadSram;
	struct {
	    long opcode;
	    long word_count;
	    long sram_phys_addr;
	    long r3000_virt_addr;
	} WriteSram;
	struct {
	    long opcode;
	    long sram_phys_addr;
	    long data;
	} PutData;
	struct {
	    long opcode;
	    long cliplist_number;
	    long cliprect_count;
	    gaStampClipRect rect[1];
	} LoadClipList;
     } un;
} ga_Packet, *ga_PacketPtr;

typedef struct _Com2d {
    long pad1[3];			/* get to aligned boundary */
    long NoOp[10];			/* No-op packet area */
    long Stic_NoOp[10];			/* Store Stic NoOp */
    long Video_NoOp[4];			/* Store Video NoOp */
    volatile long intr_status;		/* interrupt status */
    volatile long lastRead;
    volatile long lastWritten;
    volatile long qDepth;
    gaStampClipRect *pCliprect;
    long numCliprect;
    gaStampClipRect *fixCliprect;
    volatile long *srv_qpoll[GA_QUEUE_PACKETS];
    volatile long *intr_qpoll[GA_QUEUE_PACKETS];
    volatile long *save_region[GA_QUEUE_PACKETS];
} Com2d, *Com2dPtr;

typedef struct ga_ComArea {
    /*
     * Request Buffer 0
     */
    long CmdBuf0[ GA_CMDBUF0_SIZE >> 2 ];
    /*
     * Request Buffer 1
     */
    long CmdBuf1[ GA_CMDBUF1_SIZE >> 2 ];
    /*
     * Image Buffer (pad0 to quad align; leave word at end for count value)
     */
    char pad0[12];
    long image_buf[ (GA_IMAGE_BUFFER_SIZE+4) >> 2 ];
    /*
     * X Server Common Area (take away word to make up for count value)
     */
    long SRVCom[ (GA_COMAREA_SRVCOM_SIZE-4) >> 2 ];
    /*
     * 2d Server Common Area
     */
    Com2d SRV2DCom;
    char pad1[GA_2DCOM_SIZE-sizeof(Com2d)];
    /*
     * Cliplist
     */
    gaClipList ClipL[ N_MAX_CLIPLISTS ];
    char pad2[GA_CLIPLIST_SIZE-sizeof(gaClipList)*N_MAX_CLIPLISTS];
    /*
     * Interrupt-driven request buffers
     */
    long IntrBuf[GA_QUEUE_PACKETS][GA_CMDBUF0_SIZE >> 2];
} ga_ComArea, *ga_ComAreaPtr;

#endif n_max_cliprects

#endif _GA_H_
