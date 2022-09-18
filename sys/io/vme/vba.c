#ifndef lint
static char *sccsid = "@(#)vba.c	4.8	(ULTRIX)	2/28/91";
#endif lint

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
 ************************************************************************
 * Abstract:
 *	This module contains the general support routines 
 *	for the VMEbus.
 *
 * Modification History:
 *
 * 25-Feb-91 -- Mark Parenti
 *	Fixed problem which occurred if run out of map registers.
 *	Need to try again to get map registers.
 *
 * 19-Feb-91 -- Mark Parenti
 *	Add vhp parameter to vme_rmw.
 *
 * 22-Jan-91 -- Mark Parenti
 *	Fix initialization of DMA Page Map Registers.
 *	Fix computation of number of PMR's required.
 *	Modify address computation to take into account address space 
 *	selection.  The DMA PMRs can be mapped into either the 1st or 2nd
 *	GB of VMEbus address space.
 *	Return error if A24 DMA is requested and the 2nd GB mapping is 
 *	enabled.
 *
 * 18-Dec-90 -- Mark Parenti
 *	Removed MVIA support.
 *	Lint fixes.
 *
 * 12-Oct-90 -- Mark Parenti
 *	Add support for requesting a hard-wired VME address.
 *	Remove CMAX support
 *
 * 31-Aug-90 -- Mark Parenti
 *	Fix map register computation to make it more generic.
 *
 * 06-Jun-90 -- Paul Grist
 *      Added vme_rmw as a primitive for device drivers to do
 *      interlocked transactions to VME memory space.
 *
 * 16-May-90 -- Mark Parenti
 *	Change interface to vbarelse() to remove flags.
 *
 * 10-Apr-90 -- Mark Parenti
 *	Use vba_hd pointer as argument.
 *	Setup byte swap information.
 *
 * 08-Mar-90 -- Mark Parenti
 *	Fix map register addressing problems.
 *	Add debug printing support.
 *
 * 08-Mar-90 -- Paul Grist
 *      removed stub routines for error handling calls, all error handling
 *      and logging code lives in vba_errors.c
 *
 * 14 Nov 89 -- map (Mark A. Parenti)
 *	Original Version
 */

#include "../machine/pte.h"

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/map.h"
#include "../h/vmmac.h"
#include "../h/buf.h"
#include "../h/vm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/conf.h"
#include "../h/dk.h"
#include "../h/kernel.h"
#include "../h/clist.h"
#include "../h/ipc.h"
#include "../h/shm.h"
#include "../h/errlog.h"

#include "../machine/cpu.h"
#include "../machine/common/cpuconf.h"

#ifdef vax
#include "../machine/mtpr.h"
#endif /* vax */
#ifdef mips
#include "../machine/ssc.h"
#endif /* mips */

#include "../machine/nexus.h"
#include "../io/xmi/xbireg.h"
#include "../io/vme/xviareg.h"
#include "../io/vme/xvibreg.h"
#include "../io/vme/vbareg.h"
#include "../io/vme/vbavar.h"
#include "../io/uba/ubavar.h"

#ifdef mips
#define WBFLUSH	wbflush()		/* Flush write buffer on mips */
#else
#define WBFLUSH ;			/* NOP for vax	*/
#endif /* mips */


extern	struct vbadata vbadata;
extern	int	cpu;

int	dmadebug = 0;
/* Define debugging stuff.
 */
#define DEBUG
#ifdef DEBUG
#define Cprintf if(dmadebug)cprintf
#define Dprintf if( dmadebug >= 2 )cprintf
#else
#define Cprintf ;
#define Dprintf ;
#endif

/*
 * Allocate and setup DMA map registers.
 * Flags indicate which address space is requested.
 *
 * Return value:
 *	4K Page Size:
 *		Bits 0-11	Byte offset
 *		Bits 12-26	Start map reg. no.
 *		Bits 27-31	Rsvd
 *
 *	8K Page Size:
 *		Bits 0-12	Byte offset
 *		Bits 13-27	Start map reg. no.
 *		Bits 28-31	Rsvd
 */

unsigned int
vbasetup(vhp, bp, flags, addr)
	struct vba_hd *vhp;
	struct buf *bp;
	long flags;
	u_long addr;
{
	register int dmareg, tempreg;
	int reg;
	unsigned v, valid, vmeaddr;
	volatile int *io;
	register struct pte *pte;
	struct proc *rp;
	int pfn, start_pmr;
	int a, vbinfo, offset, npmr, shift, dma_shift, byte_swap;

	if ( (flags & VME_ADD_MASK) > VME_A32)
		panic("vba: Invalid VME Address Space");
	/*
	 * If reservation request, reserve VME address space
	 */
	if (flags & VME_RESERV) {
		a = spl6();
		vmeaddr = rmalloc(vhp->vba_map[flags & VME_ADD_MASK],
				  (long)bp->b_bcount);
		splx(a);
		return(vmeaddr);
	}

	v = btop(bp->b_un.b_addr);
	/* offset within a page */
	offset = (int)bp->b_un.b_addr & PGOFSET;

	/* 
	 * Number of pages spanned by the buffer + 1(hence number of
	 * map registers needed since we need one guard page)
	 */
	a = spl6();

	/*
	 * If DMA request then allocate DMA PMR's
	 */
	if (flags & VME_DMA) {
		/* Check if we have a map for this address space */
		if(vhp->dma_map[flags & VME_ADD_MASK] == 0) {
			splx(a);
			return(0);
		}
		/* If A24 space requested and 2nd GB mapping is selected */
		/* then return error as the 2nd GB is not addressable in */
		/* the A24 space					 */

		if (((flags & VME_ADD_MASK) == VME_A24) &&
		    vbadata.asc) {
			splx(a);
			return(0);
		}
	        npmr = ((bp->b_bcount + offset + vhp->nbyte_dmapmr - 1) /
			vhp->nbyte_dmapmr) + 1;

		/* If addr is non-zero then attempt to allocate at the
		 * requested VME address.
		 */
		if(addr) {
			/* Address must be on a 4K boundary */
			if(addr & 0xFFF) {
				splx(a);
				return(0);
			}
			start_pmr = (addr/vhp->nbyte_dmapmr) + 1;
			if( (dmareg = rmget(vhp->dma_map[flags & VME_ADD_MASK],
				      (long)npmr, start_pmr) ) == 0 ) {
				splx(a);
				return(0);
			}
		}
		else {
		   while( (dmareg = rmalloc(vhp->dma_map[flags & VME_ADD_MASK],
				(long)npmr)) == 0) {

			if (flags & VME_CANTWAIT) {
				splx(a);
				return (0);
			}
			vhp->vba_vmewant++;
			sleep((caddr_t)&vhp->vba_vmewant, PSWP);
		   }
	   }
	}
	else {
		splx(a);
		return(0);
	}

	splx(a);
	Cprintf("vbasetup: dmareg = %d, npmr = %d\n", dmareg-1, npmr);
	dmareg--;
	tempreg = dmareg;
	rp = bp->b_flags&B_DIRTY ? &proc[2] : bp->b_proc;
	pte = 0;
	if ((bp->b_flags & B_PHYS) == 0) {
#ifdef vax
		pte = &Sysmap[btop(((int)bp->b_un.b_addr)&0x7fffffff)];
#else   	
		/* mips KSEG Addressing */
	        if(IS_KSEG0(bp->b_un.b_addr)){ 
			/* unmapped KSEG areas - no ptes */
                	pfn = btop(K0_TO_PHYS(bp->b_un.b_addr));
	        } else if(IS_KSEG1(bp->b_un.b_addr)){
			/* unmapped KSEG areas - no ptes */
                	pfn = btop(K1_TO_PHYS(bp->b_un.b_addr));
	        } else if(IS_KSEG2(bp->b_un.b_addr)){
			/* mapped KSEG areas */
	                pte = &Sysmap[btop(bp->b_un.b_addr - K2BASE)];
        	}
#endif /* vax */
	}
	else if (bp->b_flags & B_UAREA)
		pte = &rp->p_addr[v];
	else if (bp->b_flags & B_PAGET)
		pte = &Usrptmap[btokmx((struct pte *)bp->b_un.b_addr)];
	else if ((bp->b_flags & B_SMEM)  &&	/* SHMEM */
					((bp->b_flags & B_DIRTY) == 0))
		pte = ((struct smem *)rp)->sm_ptaddr + v;
	else
		pte = vtopte(rp, v);

        byte_swap = (flags & VME_BS_MASK) >> VME_BS_SHIFT;
	switch (vhp->vba_type) {

	      case VBA_3VIA:
		/* get address of starting map register */
		io = (int *)(Xviaregs.dma_pmr) + dmareg;
		valid = (XVIA_DMA_VALID | XVIA_DMA_SU | XVIA_DMA_2432);
		valid |= byte_swap << XVIA_DMA_BS_SHIFT;
		shift = XVIA_DMAPMR_SHIFT;
		dma_shift = XVIA_DMA_ADD_SHIFT;
		break;

	      case VBA_XBIA:
		/* get address of starting map register */
		io = (int *)(Xvibregs.dma_pmr) + dmareg;
		valid = XBIAP_DMA_VALID;
		shift = XVME_DMAPMR_SHIFT;
		dma_shift = 0;
		break;
	}
        /* Loops to load all the map registers with pte information.
	 *
	 * On a VAX and DS5000  we have npmr map registers to fill 
	 * and each map register corresponds to a single page.
	 *
	 * We have two loops here because there are no pte's for KSEG0 and
	 * KSEG1.  One loop will increment the pte, used for KSEG2, all
	 * VAX kernel space and user space.  The other loop will increment 
	 * the pfn, used for KSEG0 and KSEG1 space.
	 */
	if (pte) {
		while (--npmr != 0) {
			if (pte->pg_pfnum == 0)
				panic("vba: zero page frame number");
			Cprintf("vbasetup(pte): pfn = 0x%x\n", pte->pg_pfnum);
			switch (vhp->vba_type) {

			      case VBA_XBIA:
				/* LOAD BYTE SWAP RAM	*/
				*Xvibregs.vbsr = (VBSR_WRITE |
					   (tempreg++ << XVME_DMAPMR_SHIFT) |
					   byte_swap);  

			      /* FALL THROUGH */

			      case VBA_3VIA:
				*(int *)io++ = (pte++->pg_pfnum << dma_shift)| 
					valid;
				break;
			}
		}
	}

	/* Only have a pfn because it's KSEG0 or KSEG1. 	*/
	else {
		Cprintf("vbasetup(pfn): pfn = 0x%x\n", pfn);
		while (--npmr != 0) {
			if (pfn == 0)
				panic("vba: zero page frame number");
			*(int *)io++ = (pfn << dma_shift) | valid;
			if( vhp->vba_type == VBA_XBIA) {
				/* LOAD BYTE SWAP RAM	*/
				*Xvibregs.vbsr = (VBSR_WRITE |
					   (tempreg++ << XVME_DMAPMR_SHIFT) |
					   byte_swap);  

			}
			pfn++;

		}
	}

	*(int *)io++ = 0;	/* Set up fire wall */
	vbinfo =   (dmareg << shift) | offset;
	/* If PMR's mapped into 2nd GB then OR in offset*/
	if (vbadata.asc)
		vbinfo |= XVIA_DMA_UDMA;
	WBFLUSH;
        return (vbinfo);
}

/*
 * Non buffer setup interface... set up a buffer and call vbasetup.
 */
unsigned int
vballoc(vhp, addr, bcnt, flags, vmeaddr)
	struct vba_hd *vhp;
	caddr_t addr;
	int bcnt, flags;
	u_long vmeaddr;
{
	struct buf vbabuf;

	vbabuf.b_un.b_addr = addr;
	vbabuf.b_flags = B_BUSY;
	vbabuf.b_bcount = bcnt;
	/* that's all the fields vbasetup() needs */
	return (vbasetup(vhp, &vbabuf, flags, vmeaddr));
}
 
/*
 * Release resources on vba vban, and then unblock resource waiters.
 * The map register parameter is by value since we need to block
 * against uba resets on 11/780's.
 */
vbarelse(vhp, mr)
	struct vba_hd *vhp;
	int mr;
{
	register int reg, npf, s, shift, map;
	volatile int *io;
 
	/*
	 * Traverse the map registers until the firewall is reached. This
	 * serves two purposes: First, the number of map registers to
	 * free is determined, second, all registers are cleared to prevent
	 * inadvertent accesses from succeeding.
	 *
	 * Put back the registers in the resource map.
	 * The map code must not be reentered, so we do this
	 * at high ipl.
	 */
	Cprintf("vbarelse: mr = 0x%x\n", mr);
	switch(vhp->vba_type) {
	      case VBA_3VIA:
		shift = XVIA_DMAPMR_SHIFT;
		/* If 2nd GB mapping is selected then mask off upper bits.*/
		if (vbadata.asc)
			mr = mr & ~XVIA_DMA_MASK;
		break;

	      case VBA_XBIA:
		shift = XVME_DMAPMR_SHIFT;
		break;
	}
	reg = ((mr >> shift) );
	Cprintf("vbarelse: mr = 0x%x, reg = %d\n", mr, reg);
	/* get address of starting map register */
	switch(vhp->vba_type) {
	      case VBA_3VIA:
		io = (int *)(Xviaregs.dma_pmr) + reg;
		break;

	      case VBA_XBIA:
		io = (int *)(Xvibregs.dma_pmr) + reg;
		break;
	}
	if (*(int *)io == 0)
		panic("vba: Invalid DMA map register");
	npf = 1; /* Include firewall */
	while (*(int *)io != 0) {
		npf++;
		*(int *)io++ = 0;	/* Clear entry */
	}
		
	if (reg < vhp->n24dmapmr)
		map = VME_A24;
	else
		map = VME_A32;
	s = spl6();
	reg++;
	rmfree(vhp->dma_map[map], (long)npf, (long)reg);
	splx(s);

	/*
	 * Wakeup sleepers for map registers,
	 */
	if (vhp->vba_vmewant) {
		vhp->vba_vmewant = 0;
		wakeup((caddr_t)&vhp->vba_vmewant);
	}
}
/****************************************************************************
 *
 * vme_rmw(vhp,address_ptr,data,mask)
 *
 *   PARAMETERS
 *		  vhp	      - pointer to vba_hd structure
 *                address_ptr - pointer to data to be modified
 *                data        - new data to be written
 *                mask        - what bit(s) to check for locked data
 *
 *   RETURNS
 *            0   for success
 *           -1   for failure (data locked)
 *
 *   ------------------------------------------------------------------------
 *
 *   Perform a RMW to VME-side memory using the MVIB adapter,
 *   which will emulate a RMW cycle upon our request. 
 *   This is simply an interlock primitive which can be used
 *   by device drivers to suit their individual needs. It allows
 *   a form of bit-test instruction to be accomplished.
 *
 *   The address_ptr points to the location that someone wishes to
 *   modify, if the data is not locked, then the new data will be written
 *   to the location.  The mask value tells us what bit(s) to check in
 *   order to see if the data is locked.
 *
 *     if (the data pointed to by address_ptr & mask)
 *         { data is locked; old data remains; return failure }
 *     else
 *         { data not locked; write new data; return success }
 *
 *   As the write occurs the MVIB will emulate a RMW to the VME-side
 *   memory while we block interrupts, thus insuring that no one else
 *   can jeapordize the data.
 *
 *   EXAMPLE:    a device driver using its own format for data
 *               to provide a locking mechinism.
 *
 *              31               7           0
 *               +--------------+------------+
 *               |      data    | lock field |
 *               +--------------+------------+
 *
 *   To control modifying data the driver provides the ability to
 *   change this data only thru the vme_rmw routine which
 *   it can burry or use to its own liking. For example it could  provide
 *   some type of ioctl which uses the primitive to do interlocked
 *   instructions to VME memory in blocks of 1024 words.
 *
 *   PLEASE NOTE: This capability is only provided for writes to VME memory
 *
 ****************************************************************************/

vme_rmw(vhp,address_ptr,data,mask)

struct vba_hd *vhp;    /* pointer to vba_hd structure */
u_int  *address_ptr;   /* points to old data */
u_int  data;           /* new data */
u_int  mask;           /* lock bit(s) mask */

{
  int s;

  /*
   * block interrupts and tell MVIB interlock xaction is on the way
   */

  s = splhigh();
  *Xviaregs.csr |= CSR_INT_XACT;

  /*
   * if it is locked we will still have to perform a write, so need to
   * write the old data back, restore interrrupts, clear csr bit, 
   * and return failure
   */

  if ( *address_ptr & mask ) 
    {
      *address_ptr = *address_ptr ;
      *Xviaregs.csr &= CSR_CLR_INT_XACT; 
      splx(s);
      return(-1);
    }

  /*
   * data is not locked, write new data out, restore interrupts, clear 
   * csr bit, and return success
   */

  else
    {
      *address_ptr = data;
      *Xviaregs.csr &= CSR_CLR_INT_XACT; 
      splx(s);
      return(0);
    }

}



