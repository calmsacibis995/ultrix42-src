#ifndef lint
static	char	*sccsid = "@(#)gvp_block.c	4.1	(ULTRIX)	7/2/90";
#endif lint

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
 *		Generic Vaxport Port Driver
 *
 *   Abstract:	This module contains Generic Vaxport Port Driver( GVP )
 *		block data communication service functions.
 *
 *   Creator:	Todd M. Katz	Creation Date:	May 6, 1985
 *
 *   Function/Routines:
 *
 *   gvp_map_buf		Map Buffer
 *   gvp_unmap_buf		Unmap Buffer
 *
 *   Modification History:
 *
 *   25 May 90 -- chet
 *	Changed gvp_map_buf() to allow unmapped addresses.
 *
 *   25-May-1989	Pete Keilty
 *	Changed Dm_buffer macro to pass in the address of buffer.
 *
 *   20-May-1989	Pete Keilty
 *	Added support for mips risc cpu's double mapped buffer & pte's
 *	into a Vaxmap of the system.
 *	CI/BVP ports need to have VAX pte's and system addresses.	
 *
 *   05-Mar-1989	Todd M. Katz		TMK0003
 *	1. Include header file ../vaxmsi/msisysap.h.
 *	2. Use the ../machine link to refer to machine specific header files.
 *
 *   02-Jun-1988     Ricky S. Palmer
 *	Removed inclusion of header file ../vaxmsi/msisysap.h
 *
 *   20-Apr-1988	Todd M. Katz		TMK0002
 *	Corrected typo in gvp_unmap_buf() which prevented zeroing of the first
 *	longword of every buffer descriptor during local buffer unmapping.
 *	The typo had the unfortunate side-effect of always leaving buffer
 *	descriptors marked valid even after unmapping of the local buffers
 *	formerly referenced by them.
 *
 *   08-Jan-1988	Todd M. Katz		TMK0001
 *	Formated module, revised comments, increased robustness, made GVP
 *	completely independent from underlying port drivers, restructured code
 *	paths, and added SMP support.
 */

/* Libraries and Include Files.
 */
#include		"../h/types.h"
#include		"../h/param.h"
#include		"../h/time.h"
#include		"../h/buf.h"
#include		"../h/ipc.h"
#include		"../h/proc.h"
#include		"../h/systm.h"
#include		"../h/vmmac.h"
#include		"../h/shm.h"
#include		"../h/errlog.h"
#include		"../h/ksched.h"
#include		"../machine/pte.h"
#ifdef vax
#include		"../machine/mtpr.h"
#endif vax
#include		"../machine/cpu.h"
#include		"../machine/common/cpuconf.h"
#include		"../io/scs/sca.h"
#include		"../io/scs/scaparam.h"
#include		"../io/ci/cippdsysap.h"
#include		"../io/ci/cisysap.h"
#include		"../io/msi/msisysap.h"
#include		"../io/bi/bvpsysap.h"
#include		"../io/gvp/gvpsysap.h"
#include		"../io/uba/uqsysap.h"
#include		"../io/sysap/sysap.h"
#include		"../io/ci/cippdscs.h"
#include		"../io/ci/ciscs.h"
#include		"../io/msi/msiscs.h"
#include		"../io/bi/bvpscs.h"
#include		"../io/gvp/gvpscs.h"
#include		"../io/uba/uqscs.h"
#include		"../io/scs/scs.h"
#include		"../io/scs/scamachmac.h"
#include		"../io/gvp/gvp.h"

/* External Variables and Routines.
 */
extern	SCSIB		lscs;
extern	struct proc	*proc;
extern	GVPBDDB		*gvp_bddb;
extern	struct pte	Sysmap[], Usrptmap[], usrpt[], *vtopte();
extern  int cpu;

/*   Name:	gvp_map_buf	- Map Buffer
 *
 *   Abstract:	This function maps a local buffer.
 *
 *		A buffer descriptor is allocated from the generic Vaxport
 *		buffer descriptor free list.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   bhp			- Buffer handle pointer
 *   gvp_bddb			- Generic Vaxport Buffer Descriptor Database
 *   pccb			- Port Command and Control Block pointer
 *   proc			- Process table
 *   sbh			- System buffer handle pointer
 *   scsid			- SCS identification number
 *   Sysmap			- Address of System page table
 *   usrpt			- Address of Process page tables
 *   Usrptmap			- Address of System ptes mapping process ptes
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   bhp			- Buffer handle pointer
 *	pd.gvp.boff		-  0
 *	pd.gvp.bname		-  Generic Vaxport buffer name
 *	scsid			-  SCS identification number
 *   gvp_bddb			- Generic Vaxport Buffer Descriptor Database
 *	free_bd			-  Free buffer descriptor list head
 *
 *   Return Values:
 *
 *   RET_SUCCESS		- Successful allocation and initialization
 *   RET_ALLOCFAIL		- Allocation of free buffer descriptor failed
 *
 *   SMP:	The generic Vaxport buffer descriptor database is locked for
 *		allocation of a buffer descriptor from the free list.
 */
u_long
gvp_map_buf( pccb, bhp, sbh, scsid )
    PCCB		*pccb;
    register BHANDLE	*bhp;        
    register struct buf	*sbh;
    u_long		scsid;
{
    register GVPBD	*bdp;
    register u_long	status = RET_SUCCESS;
    register unsigned addr = (unsigned)sbh->b_un.b_addr;
    register struct proc *rp = sbh->b_flags & B_DIRTY ? &proc[2] : sbh->b_proc;
    register u_long   	mask;

    /* Local buffer mapping is accomplished as follows:
     * 
     * 1. Lock the generic Vaxport buffer descriptor database.
     * 2. Allocate a free generic Vaxport buffer descriptor.
     * 3. Unlock the generic Vaxport buffer descriptor database.
     * 4. Map the local buffer by filling in the allocated buffer descriptor.
     * 5. Construct a handle on the local buffer.
     * 6. Return an appropriate status.
     *
     * How the local buffer is actually mapped depends upon what type of buffer
     * it is.  There currently are five types of local buffers:
     *
     * 1. System buffers( physical I/O is not being done ).  The system virtual
     *	  address of the system page table PTEs which map the buffer is
     *	  computed.
     *
     * 2. Per-process buffers involved in swap operations.  The system virtual
     *	  address of the appropriate process page table PTEs which map the
     *	  buffer is computed.  The swapper's process page table is utilized
     *	  whenever the buffer is marked dirty; otherwise, the page table of the
     *	  process associated with the buffer is used.  Buffers are never marked
     *	  dirty while in process context.
     *
     * 3. Buffers containing portions of a per-process page table which are
     *	  being paged.  The system virtual address of the system page table
     *	  PTEs which map the buffer is computed.  The portion of the system
     *	  page table which maps the per-process page tables is utilized.
     *
     * 4. Process buffers located in shared memory and are marked dirty.  The
     *	  system virtual address of the PTEs which map the buffer using shared
     *	  memory structures is computed.
     *
     * 5. Buffers which are none of the above.  A separate routine is invoked
     *	  to compute the system virtual address of the process page table PTEs
     * 	  which map the buffer.  The swappers process page table is utilized
     *	  whenever the buffer is marked dirty; otherwise, the page table of the
     *	  process associated with the buffer is used.  Buffers are never marked
     *    dirty while in process context.
     */
    Lock_gvpbd()
    if(( bdp = gvp_bddb->free_bd )) {
	gvp_bddb->free_bd = bdp->next_free_bd;
    } else {
	status = RET_ALLOCFAIL;
    }
    Unlock_gvpbd()
    if( status == RET_SUCCESS ) {
	if( pccb->lpinfo.type.hwtype == HPT_SII ) {
	    mask = PGOFSET;
	}
	else {
	    mask = VAX_PGOFSET;
	}
	U_short( *bdp ) = ( addr & mask ) | GVPBD_VALID;
	bdp->bsize = sbh->b_bcount;

	if(( sbh->b_flags & B_PHYS ) == 0 ) {
#ifdef vax
	    bdp->bpte = &Sysmap[ btop( addr - MAPPED_SYSBASE ) ];
#else   	
		/* mips KSEG Addressing */
	        if( IS_KSEG0( addr )){ 
			/* unmapped KSEG areas */
			U_short( *bdp ) |= 0x4000;
                	bdp->bpte = (struct pte *)addr;
	        } else if( IS_KSEG1( addr )){
			/* unmapped KSEG areas */
			U_short( *bdp ) |= 0x4000;
                	bdp->bpte = (struct pte *)addr;
	        } else if(IS_KSEG2( addr )){
			/* mapped KSEG areas */
	    		bdp->bpte = &Sysmap[ btop( addr - MAPPED_SYSBASE ) ];
		}
#endif /* vax */
	}
	else if( sbh->b_flags & B_UAREA ) {
	    bdp->bpte = (struct pte *)&rp->p_addr[ btop( addr ) ];
	}
	else if( sbh->b_flags & B_PAGET ) {
	    bdp->bpte = &Usrptmap[ btokmx( (struct pte *)addr ) ];
	}
	else if(( sbh->b_flags & B_SMEM ) &&
		(( sbh->b_flags & B_DIRTY ) == 0 )) {
	    bdp->bpte = ( (struct smem *)rp )->sm_ptaddr + btop( addr );
	}
	else {
	    bdp->bpte = vtopte( rp, btop( addr ) );
	}

	Dm_buffer( bdp, (bdp - gvp_bddb->bdt), addr )

	bhp->Boff = 0;
	U_long( bhp->Bname.index ) = ( bdp - gvp_bddb->bdt );
	bhp->Bname.key = bdp->key;
	bhp->scsid = scsid;
    }
    return( status );
}

/*   Name:	gvp_unmap_buf	- Unmap Buffer
 *
 *   Abstract:	This function unmaps a local buffer.
 *
 *		A buffer descriptor is deallocated to the generic Vaxport
 *		buffer descriptor free list after its key is incremented.
 *
 *		Encountering an invalid buffer handle causes a panic.
 *
 *   Inputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   bhp			- Buffer handle pointer
 *   gvp_bddb			- Generic Vaxport Buffer Descriptor Database
 *   lscs			- Local system permanent information
 *   pccb			- Port Command and Control Block pointer
 *   sbh			- System buffer handle pointer
 *
 *   Outputs:
 *
 *   IPL_SCS			- Interrupt processor level
 *   gvp_bddb			- Generic Vaxport Buffer Descriptor Database
 *	free_bd			-  Free buffer descriptor list head
 *
 *   SMP:	The generic Vaxport buffer descriptor database is locked for
 *		buffer descriptor deallocation to the free list.
 */
void
gvp_unmap_buf( pccb, bhp, sbh )
    PCCB		*pccb;
    register BHANDLE	*bhp;        
    struct buf		*sbh;
{
    register GVPBD	*bdp;
    register u_long	index, key;

    /* Local buffer unmapping is accomplished as follows:
     *
     * 1. Position to the appropriate generic Vaxport buffer descriptor.
     * 2. Verify the authenticity of the targeted buffer descriptor.
     * 3. Clear the targeted buffer descriptor.
     * 4. Lock the generic Vaxport buffer descriptor database.
     * 5. Return the targeted buffer descriptor to the generic Vaxport buffer
     *	  descriptor free list.
     * 6. Unlock the generic Vaxport buffer descriptor database.
     */

    if((( index = bhp->Bname.index ) > ( lscs.max_gvpbds - 1 )) ||
       ( bhp->Bname.key != ( key = ( bdp = gvp_bddb->bdt + index )->key ))) {
	( void )panic( GVPPANIC_BNAME );
    }
    U_long( *bdp ) = 0;
    bdp->bsize = 0;
    U_long( bdp->bpte ) = 0;
    if(( bdp->key = ++key ) == 0 ) {
	++bdp->key;
    }
    Lock_gvpbd()
    bdp->next_free_bd = gvp_bddb->free_bd;
    gvp_bddb->free_bd = bdp;
    Unlock_gvpbd()
}
