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

/************************************************************************
 *
 * @(#)gq.h	4.2	(ULTRIX)	7/17/90
 *
 * 17-May-90	Sam Hsu
 *	Intr words moved.  Add masks for page count on pagein.
 *
 * 26-Feb-90	Sam Hsu
 *	Add GQ_INTR_PAUS/BUF0/BUF1 for console output sync.  Will have
 *	to move the intr words when ucode implements double buffering.
 *
 * 16-Jan-90	Sam Hsu
 *	Remove GQ_DISPLAY.  This is now GQ_DTYPE in some other file.
 *
 * 03-Jan-90	Sam Hsu
 *	Move gross SRAM layout to here.  Rest is defined by ucode/
 *	server.
 *
 * 22-Dec-89	Peter Valtin
 *	Seems like the agreement between kernel and ucode on interrupt
 *	locations should be in a common header file.  Not this one.
 *	The file would define the struct of the N10 ComArea - currently
 *	defined in N10protostr.h - and the offset from SRAM of the
 *	that structure -- currently defined in Graphics_Mem.h.  Both of 
 *	these header files are in ucode space.
 *	For the moment, it seems easier/safe to force this header file
 *	to include Graphics_Mem.h, and simply use the offsets defined
 *	therein.
 *
 * 00-Nov-89	Sam Hsu
 *	$Header: /nfs/kodak/local/4.0-12/io/tc/RCS/gq.h,v 1.15 90/05/17 22:20:26 fhsu Exp Locker: fhsu $
 *	Created.
 *
 ************************************************************************/
#ifndef _GQ_H_
#define _GQ_H_

#define DWN_(X)	(((int)(X)) & ~(CLBYTES-1))
#define RND_(X)	DWN_(((int)(X)) + CLBYTES-1)

#define GQ_RBUF_SIZE	(8*NBPG)
#define GQ_PRIV_SIZE	(sizeof(gxPriv)+GQ_RBUF_SIZE)
#define GQ_PRIV_PGBYTES	RND_(((int)gx_priv & (CLBYTES-1)) + GQ_PRIV_SIZE)

#define GQ_CPU_IDLESAMPLE	2	/* every 2/hz secs =~ 8ms */

/*
 * 128KB of SRAM on graphics accelerator option board.
 *
 * This stuff should EXACTLY correspond to the ucode layout, please...
 * We define the major organs here.  Internals are left with the ucode.
 * ANY changes here MUST be coordinated with the N10 ucode.
 */
#define GQ_RAM_SIZE		(0x20000)
#define GQ_RAM_NREQBUF		(2)	/* requests double-buffered */
#define GQ_VTLB_INDEX(V)	(((V)&0x3ff000) >> 12)

#define N10_PAGE		(0x01000)

#define N10_OUTPUT_BUFFERS_OFF	(0 * N10_PAGE)
#define N10_OUTPUT_BUFFER_SIZE	(N10_PAGE)
#define N10_OUTPUT_BUFFERS_SIZE	(GQ_RAM_NREQBUF * N10_OUTPUT_BUFFER_SIZE)
#define N10_IMAGE_BUFFER_OFF	(N10_OUTPUT_BUFFERS_OFF+N10_OUTPUT_BUFFERS_SIZE)
#define N10_IMAGE_BUFFER_SIZE	(((2*(BT459_MAXX+1)+4+7) & ~7) * sizeof(int))
#define N10_INTR_R3K_OFF	(N10_IMAGE_BUFFER_OFF+N10_IMAGE_BUFFER_SIZE)
#define N10_INTR_N10_OFF	(N10_INTR_R3K_OFF+sizeof(int))
#define N10_SAVE_OFF		(N10_INTR_N10_OFF+sizeof(int))
#define N10_VTLB_OFF		(0x1d * N10_PAGE)

typedef int gqRAMReqBuf[N10_OUTPUT_BUFFER_SIZE/sizeof(int)];
#define GQ_ramreqbuf	gqRAMReqBuf

typedef struct _gq_ram {
    /* graphics console packet area */
    gqRAMReqBuf reqbuf[GQ_RAM_NREQBUF];
    /* read/write spans image buffer + trailing WIDTH for readspans */
    int pixbuf[N10_IMAGE_BUFFER_SIZE/sizeof(int)];
    /* begin 2 interrupt words quadword aligned */
    int intr_host;
    int intr_coproc;
    /* rest is a mystery */
    char memory[GQ_RAM_SIZE - N10_SAVE_OFF];
} gqRAM;
#define GQ_ram		gqRAM

#define GQ_REQBUF(X,N)	(GQ_RAM(X)->reqbuf[(N)%GQ_RAM_NREQBUF])

/* convert a system virtual address to SRAM physical address */
#define GQ_SYS_TO_PHYS(A)	((u_long)(A) - (u_long)GQ_RAM(gqo))


/*
 * Template for accessing various fields on GQ board.
 */
typedef struct _gq_map {

    /* 0x000000: STIC polling register (initiate DMA read) */
    int		stic_poll_reg;  char __pad05[0x180000-sizeof(int)];

    /* 0x180000: STIC control registers */
    sticRegs	stic_reg;  char __pad10[0x200000-0x180000-sizeof(sticRegs)];

    /* 0x200000: SRAM */
    gqRAM	ram;  char __pad15[0x280000-0x200000-sizeof(gqRAM)];

    /*
     * low 17 bits specify where in SRAM the write will appear (see also
     * gqRAM structure definition).
     */		      char __pad20[N10_INTR_R3K_OFF];

    /* 0x..280000 N10 interrupt host / host clear interrupt */
    /* 10 100x xxxx xxxx xxxx xxxx */
    int		intr_host;  char __pad25[0x2c0000-0x280000-sizeof(int)
					-N10_INTR_R3K_OFF];

#define HST_INTR_MASK	0xfffff000
#define HST_INTR_SHFT	0x0000000c	/* 12. */

#define HST_INTR_WHAT	0x0000000f
#define HST_INTR_WSHF	0x00000000
#define HST_INTR_PGIN	0x00000001	/* pagein */
#define HST_INTR_XLAT	0x00000002	/* translate */
#define HST_INTR_VSYN	0x00000003	/* vblank sync */
#define HST_INTR_VRFY   0x00000004	/* verify valid map */
#define HST_INTR_PMSK	0x00000f00
#define HST_INTR_DRTY	0x00000800	/* pagein dirty page */
#define HST_INTR_PADD	0x00000700	/* add'l pageins */
#define HST_INTR_PSHF	0x00000010	/* 16. */
                      char __pad30[N10_INTR_N10_OFF];

    /* 0x..2c0000: host interrupt N10 / N10 clear interrupt */
    /* 10 110x xxxx xxxx xxxx xxxx */
    int 	intr_coproc;  char __pad35[0x300000-0x2c0000-sizeof(int)
					  -N10_INTR_N10_OFF];

#define GQ_INTR_TIMO	1500000		/* ~3 secs */
#define GQ_INTR_MASK	HST_INTR_MASK
#define GQ_INTR_SHFT	HST_INTR_SHFT
#define GQ_INTR_ACK	0x00000000
#define GQ_INTR_WHAT	0x000000f0
#define GQ_INTR_WSHF	0x00000008
#define GQ_INTR_INV1	0x00000010	/* invalidate 1 pte */
#define GQ_INTR_INVA	0x00000020	/* invalidate all ptes */
#define GQ_INTR_CEIL	GQ_INTR_INVA
#define GQ_INTR_PAUS	0x00000030	/* pause N10 */
#define GQ_INTR_FLSH	0x00000040	/* flush data cache */
#define GQ_INTR_BUF0	0x00000001
#define GQ_INTR_BUF1	0x00000002
#define GQ_INTR_HALT	0x000000f0	/* unused */

    /* 0x300000: Brooktree 459 VDAC */
    bt459Regs	vdac_reg;  char __pad45[0x380000-0x340000-sizeof(bt459Regs)];
    /* chip reset */
    int		vdac_reset;  char __pad40[0x340000-0x300000-sizeof(int)];

    /* start/reset N10 write-only (also ROM read-only) */
    int		start_coproc; char __pad50[0x3c0000-0x380000-sizeof(int)];
    int		reset_coproc;		/* 0x3c0000 */

} gqMap;
#define GQ_map		gqMap


#define GQ_PTPT_ENTRIES	(512)		/* max. 512 */
#define GQ_PTPT_SIZE	(GQ_PTPT_ENTRIES * sizeof(int))

typedef struct gq_ptpt {
    unsigned	vsn : 9;
    unsigned	    : 3;
    unsigned	ppn : 20;
} gqPTPT;
#define GQ_PTPT_PPN	0xfffff000	/* ptp ppn */
#define GQ_PTPT_VSN	0x000001ff	/* vsn for this ppn */
#define GQ_ptpt		gqPTPT

#define vtovsn(VA)	(((VA) >> 22) & GQ_PTPT_VSN)
/******************************************************************************
 * R3K intr N10:
 *
 ******	Invalidate one PTE (paging):
 *	intr_coproc = (vpn & GQ_INTR_MASK) | GQ_INTR_INV1;
 *	while (intr_coproc) ;
 *  N10:
 *	intr_coproc = 0;
 *	<invalidate VTLB entry>
 *
 ****** Invalidate all PTEs (swapping):
 *	<clear all PTPT entries>
 *	intr_coproc = GQ_INTR_INVA;
 *	while (intr_coproc) ;
 *  N10:
 *	intr_coproc = 0;
 *	bzero(VPTP, sizeof(VPTP));
 *	bzero(VTLB, sizeof(VTLB));
 *
 ****** PTE ceil (brk'ing):
 *	<clear PTPT entry, if necessary>
 *	intr_coproc = (vpn & GQ_INTR_MASK) | GQ_INTR_CEIL;
 *	while (intr_coproc) ;
 *  N10:
 *	intr_coproc = 0;
 *	<invalidate all VTLB/VPTP entries >= vpn>
 *	<N10 has option to collapse this into invalidate-all>
 *
 ****** Halt N10:
 *	intr_coproc = GQ_INTR_HALT;
 *	while (intr_coproc) ;
 *  N10:
 *	intr_coproc = 0;
 *	halt();
 *
 ******************************************************************************
 * N10 intr R3:
 *
 ****** PageIn (pte invalid):
 *	while (intr_host == HST_INTR_VSYN) ;
 *	intr_host = (VA & HST_INTR_MASK) | HST_INTR_PGIN;
 *	while (intr_host) ;
 *  R3K:
 *	intr_host = intr_host;
 *	signal(server);
 *	[...in server...]
 *	int tmp = *(volatile int *)(intr_host & HST_INTR_MASK);
 *	intr_host = 0;
 *
 ****** Xlate (can't complete address translation):
 *	while (intr_host == HST_INTR_VSYN) ;
 *	intr_host = tmp = (VA & HST_INTR_MASK) | HST_INTR_XLAT;
 *	while ((reg = intr_host) == tmp) ;
 *	if (reg == 0)
 *		<retry address translation>;
 *	else
 *		PA = reg | (VA & 0xfff);
 *  R3K:
 *	vpn = fill_ptpt_entry(ptpt, intr_host);
 *	pte = vtopte(server, vpn);
 *	if (pte->pg_v)
 *		intr_host = pte->pg_pfnum << GQ_INTR_SHFT;
 *	else
 *		intr_host = intr_host;
 *		signal(server);
 *	[...in server...]
 *	int tmp = *(volatile int *)(intr_host & HST_INTR_MASK);
 *	intr_host = 0;
 *
 ****** Vertical blank (install colormap):
 *	intr_host = HST_INTR_VSYN;
 *  R3K:
 *	intr_host = 0;
 *	if (new_colormap)
 *		install new colormap;
 */

/*
 * (GQ_map *) to GQ field ptr
 */
#define GQ_POLL(G)		(& (((gqMap *)G)->stic_poll_reg) )
#define GQ_VDAC(G)		(& (((gqMap *)G)->vdac_reg) )
#define GQ_VDACRESET(G)		(& (((gqMap *)G)->vdac_reset) )
#define GQ_RAM(G)		(& (((gqMap *)G)->ram) )
#define GQ_STIC(G)		(& (((gqMap *)G)->stic_reg) )
#define GQ_STAMP(G)		(((int)G) | 0xC0000)
#define GQ_INTRC(G)		(& (((gqMap *)G)->intr_coproc) )
#define GQ_INTRH(G)		(& (((gqMap *)G)->intr_host) )
#define GQ_START(G)		(& (((gqMap *)G)->start_coproc) )
#define GQ_RESET(G)		(& (((gqMap *)G)->reset_coproc) )

/*
 * user-level debug of board
 */
#ifndef KERNEL

#define GQO_POLL(X)	 GQ_POLL     (((gxInfo *)X)->gxo)
#define GQO_VDAC(X)	 GQ_VDAC     (((gxInfo *)X)->gxo)
#define GQO_VDACRESET(X) GQ_VDACRESET(((gxInfo *)X)->gxo)
#define GQO_GRAM(X)	 GQ_RAM      (((gxInfo *)X)->gxo)
#define GQO_STIC(X)	 GQ_STIC     (((gxInfo *)X)->gxo)
#define GQO_STAMP(X)	 GQ_STAMP    (((gxInfo *)X)->gxo)
#define GQO_REQBUF(X,N)  GQ_REQBUF   (((gxInfo *)X)->gxo,N)
#define GQO_RWSPAN(X)   (GQ_RAM      (((gxInfo *)X)->gxo)->pixbuf)

#define GQO_SYS_TO_SRAM(X,A) ((int)(A) - (int)GQ_RAM((X)->gxo))

#define GX_Info		gxInfo		 /* backward compat. */

#endif

#endif _GQ_H_
