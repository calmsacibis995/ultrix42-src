/*
 *	@(#)vbavar.h	4.2	(ULTRIX)	12/20/90
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
/*
 * Per-vba structure.
 *
 *
 * This structure holds the interrupt vector for the uba,
 * and its address in physical and virtual space.  At boot time
 * we determine the devices attached to the uba's and their
 * interrupt vectors, filling in uh_vec.  We free the map
 * register and bdp resources of the uba into the structures
 * defined here.
 *
 * During normal operation, resources are allocated and returned
 * to the structures here.  We watch the number of passive releases
 * on each uba, and if the number is excessive may reset the uba.
 * 
 * When uba resources are needed and not available, or if a device
 * which can tolerate no other uba activity (rk07) gets on the bus,
 * then device drivers may have to wait to get to the bus and are
 * queued here.  It is also possible for processes to block in
 * the unibus driver in resource wait (mrwant, bdpwant); these
 * wait states are also recorded here.
 */
 struct	vba_hd {
	struct vba_hd *next;		/* pointer to next in list */
	int	vba_type;		/* see defines below. */
	int	vbanum;			/* vba number		*/
	int	adptnum;		/* Adapter number	*/
	caddr_t vbavirt;		/* virt addr of vba */
	caddr_t vbaphys;		/* phys addr of vba */
	caddr_t pio_base;		/* base of PIO mapped space */
	struct	vbadata	*vbadata;	/* data structure	*/
	int	(**intr_vec)();		/* interrupt vectors for DS5000 */
	int 	(**vbavec_page)();	/* interrupt vectors for others */
	int	(*vba_err)();		/* Error routine for this adapter */
	short	vba_vmewant;		/* someone is waiting for VME space */
#define	VBAMSIZ	200
#define	VME_NMAPS 3			/* 3 VME Address spaces		*/
	struct	map *vba_map[VME_NMAPS]; /* VME Address Space maps	*/
#define	VME_DMASIZ	100
	struct	map *dma_map[VME_NMAPS]; /* VME DMA PMR maps		*/
#define	VME_PIOSIZ	100
	struct	map *pio_map;		/* VME PIO PMR map		*/
	int	n32dmapmr;		/* Number of A32 DMA PMR's	*/
	int	n24dmapmr;		/* Number of A24 DMA PMR's	*/
	int	n16dmapmr;		/* Number of A16 DMA PMR's	*/
	int	nbyte_dmapmr;		/* Number of bytes per DMA PMR	*/
	int	npiopmr;		/* Number of PIO PMR's		*/
	int	nbyte_piopmr;		/* Number of bytes per PIO PMR	*/
	int	a16reg[3];		/* A16 PIO registers		*/
	                                /* One per VME data type	*/
	union	{
		struct	_xvia_reg xvia_regs;
		struct _xvib_reg  xvib_reg;
	}adapt_regs;
	union	{
		struct _xbia_info xbia_info;
	}adapt_info;
};

#define	VBA_3VIA	0x1
#define	VBA_XBIA	0x3


/* Macros 	*/

#define	Xviaregs	vhp->adapt_regs.xvia_regs
#define	Xvibregs	vhp->adapt_regs.xvib_reg















