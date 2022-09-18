#ifndef lint
static	char	*sccsid = "@(#)pdma_ds5000.c	4.2	(ULTRIX)	11/13/90";
#endif lint

/************************************************************************
 *									*
 *			Copyright (c) 1988 by				*
 *		Digital Equipment Corporation, Maynard, MA		*
 *			All rights reserved.				*
 *									*
 *   This software is furnished under a license and may be used and	*
 *   copied  only  in accordance with the terms of such license and	*
 *   with the  inclusion  of  the  above  copyright  notice.   This	*
 *   software  or  any	other copies thereof may not be provided or	*
 *   otherwise made available to any other person.  No title to and	*
 *   ownership of the software is hereby transferred.			*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or	reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

/************************************************************************
 *
 * pdma_ds5000.c	11/09/89
 *
 * Pseudo DMA routines for the DS5000.
 *
 * Modification history:
 *
 * 09/07/90     Maria Vella
 *      Submitted this file into release.
 *
 * 11/22/89	Mitch McConnell
 *	Modified dma_cont to not use FIFO flags register.  This routine is 
 *	only called with both TC set and same phase.  For some reason ffr is
 *	not being cleared by the chip and it is causing us to request more 
 *	data than the target has to give (it looks like - besides, the other
 *	version of the asc driver does not use ffr in this case).  For now,
 * 	I will leave it in dma_end - although I believe that the other driver
 *	only uses it in this way when the previous phase was data out and
 *	we get an unexpected phase change.
 *
 * 11/09/89	John A. Gallant
 * Created this file to support the PDMA work in the scsi drivers for the 3max.
 *
 ************************************************************************/

/************************************************************************

   This file contains the Pseudo DMA routines needed for the DS5000.  The 
DS5000 has an NCR 53c94 SCSI controller chip, and an 128K RAM buffer to use for
the DMA of data from the SCSI bus and Main memory.  Data has to pass through
this RAM buffer.  The RAM buffer is dual ported, the 53c94 and CPU are able
to access it to deposite and remove data.  

    The pseudo DMA code for the DS5000 uses the pipe/double buffer concept
to transfer data between memory and targets.  The code sets up the RAM
buffer as 8 "pipes", one for each ID on the SCSI bus.  Each pipe contains
2 8k buffers for data xfer, each pipe has a total size of 16k.  The 2 buffers
are used interactivly, while the 53c94 is accessing one of the buffers the
CPU accessing the other.  For example in the case of a write the CPU has
prefilled both of the buffers with user data.  The 53c94 is started on the
initial buffer to move the data to the target.  Once the 53c94 is done with
that buffer the CPU switches the 53c94 to the other one.  While the 53c94 is
using the new buffer the CPU then fills the previous buffer with more data.

    The RAM buffer in the DS5000 is accessed by the 53c94 in 16 bit words,
the CPU can access the RAM buffer at byte addresses.  On the CPU side the
There are no special routines used to read/write/zero the RAM buffer.
The routines blkcopy and bzero are used.

    The usercnt and targcnt variables in the PDMA control structure are used 
differently.  Targcnt is used to count the number of bytes that have actually
been transferd over the SCSI bus.  Usercnt is used to keep track of how many
bytes have been transfered via the active pipe.  This is why with the
buffering on the data in the pipe these two numbers are used.

*************************************************************************/


#include "../data/scsi_data.c"
#include "scsi_debug.h"

#define sc_ascdmacount		sc_siidmacount
	/* used to keep track of total number of bytes transferred */

/* External functions and variables. */
extern PDMA_ENTRY pdma_entry[];		/* the entry array */

/* Local Data area. */
/* Entry structure for the pseudo DMA routines for the DS5000.  The external
declarations are needed for the forward reference. */

extern int ds5000_init();
extern int ds5000_setup();
extern int ds5000_start();
extern int ds5000_cont();
extern int ds5000_end();
extern int ds5000_flush();
extern int bcopy();
extern int bzero();

PDMA_ENTRY pdma_ds5000 =
{
    DS_5000,				/* cpu type value */
    ( 0 ),				/* MISC control flags */
    ds5000_init,
    ds5000_setup,
    ds5000_start,
    ds5000_cont,
    ds5000_end,
    bcopy,				/* kernel routines can be used */
    bcopy,
    bzero,
    ds5000_flush
};

PDMA_ENTRY pdma_ds5500 =
{
    DS_5500,				/* cpu type value */
    ( 0 ),				/* MISC control flags */
    ds5000_init,
    ds5000_setup,
    ds5000_start,
    ds5000_cont,
    ds5000_end,
    bcopy,				/* kernel routines can be used */
    bcopy,
    bzero,
    ds5000_flush
};

/************************************************************************

int pdma_init( sc )

Inputs:	sz_softc pointer

Function:
	Initialize all that is necessary for the DMA engine/code to run 
	for the particular system.  For the pipe code the pointers and
	offsets are set, one for each possible target. 
	    NOTE: for the pvax this may have to be more of a dynamic alloc
	    for the buffer #'s perhaps done at setup().
	The PDMA related fields will be cleared/initialized and one of the
	pipe buffers will be set to READY.

Return:
	Success or Failure, on the ability to initialize for the DMA.

**************************************************************************/

int
ds5000_init( sc )
    struct sz_softc *sc;
{
    int targid;			/* loop counter for targets */
    int offset;			/* for cutting up the RAM buffer */
    PDMA *pd;			/* pointer for the DMA control struct */
    int i;			/* loop counter for pipe buffers */

    PRINTD( 0xFF, 0x1, ("ds5000_init entry sc: %x\n", sc ));

    /* Setup the RAM buffer and DMA engine address pointers in the softc.  The
    base address for these and the SCSI chip has already been set in the 
    softc by the particular driver's probe code. */

    sc->pdma_rambuff = DS5000_BUF_BASE(sc);	/* ram buffer addr */
    sc->pdma_addrreg = DS5000_AR_BASE(sc);	/* engine address */

    /* For each of the target slots on a SCSI bus, this is one of the valid
    magic #'s of 8.  Setup and assign the pointers/offsets for each of the
    DMA control structures. */

    offset = 0;			/* start at the beginning of the buffer */

    for( targid = 0; targid < NDPS; targid++ )
    {
	pd = &sc->pdma_ctrl[ targid ];	/* assign the pointer */

        /* Clear the per call elements. */

	pd->pstate = PDMA_IDLE;		/* clear out misc flags */
	pd->iodir = 0;			/* clear direction */
	pd->count = 0;			/* clear total count */
	pd->data_addr = (char *)NULL;	/* clear data address */
	pd->usercnt = 0;		/* user space count */
	pd->targcnt = 0;		/* target count */

        /* Initialize the pipe/buffer related elements. */

	pd->pb_addr = (char *)((int)sc->pdma_rambuff + offset);

	PRINTD( targid, 0x40,
	    ("ds5000_init target %d offset %x\n", targid, offset ));

	for( i = 0; i < PIPE_BUF_NUM; i++ )
	{
	    /* Setup the offsets for the pipe buffer. */

	    pd->pb_offset[i] = offset;
	    offset += MAX_XFER_SIZE;		/* offset to next buffer */

	    /* Setup the counts and flags for the pipe buffer. */

	    pd->pb_count[i] = 0;
	    pd->pb_flags[i] = PB_EMPTY;

	    /* Flush, or zero out the pipe buffer. */

	    (*sc->dma_bzero)
		( sc->pdma_rambuff + RAM_ADJUST(sc, pd->pb_offset[i]),
		MAX_XFER_SIZE );
	    wbflush();
	}

/*      Question: check for offset overrun ??
		set pstate to UNAVAILABLE ?? */

    }
    PRINTD( 0xFF, 0x001, ("ds5000_init exit\n"));

    return( PDMA_SUCCESS );
}

/************************************************************************

int pdma_setup( sc, targid, dir, count, addr )

Inputs:
	sz_softc *sc;		 pointer to the softc structure 
	int targid;		 current targit ID 
	int dir;		 direction for I/O xfer 
	long count;		 number of bytes to xfer 
	char *addr;		 address for the data 

Function:
	Verify that a DMA operation can be done at this time, ie resources
	are available.
	    NOTE: Pvax all pipes may be in "service".  A wait return
	    value would be sent back.  The job would have to be tried again
	    later.
	    NOTE: Cmax all the DAT entries may be filled or there is not 
	    enough entries in a row for this transfer.  (It could be broken
	    up across DAT's, with the DMA engine being restarted.)
	All the DMA control structures will be setup.  The pipe will be
	assigned, for a static case (Pmax, 3max) it may already be done.
	For a "write" operation both of the pipe buffers will be pre-filled
	and their status set to DATA_READY.
	    NOTE: this will degenerate to a prefilled 16k buffer, if the CPU
	    is unable to keep up with the SCSI chip.  This is similiar to the
	    current static allocation scheme in Pvax and Firefox.
	This routine WILL NOT !! startup the local DMA engine.  This task will
	be left for the local driver or the call to the pdma_start() routine.

Return:
	PDMA_SUCCESS		 all is ready and ok 
	PDMA_FAIL		 a fatal(?) problem has occured 
	PDMA_RETRY(?)		 unable to setup, try again later 

**************************************************************************/

int
ds5000_setup( sc, targid, dir, count, addr )
    struct sz_softc *sc;	/* pointer to the softc structure */
    int targid;			/* current targit ID */
    int dir;			/* direction for I/O xfer */
    long count;			/* number of bytes to xfer */
    char *addr;			/* address for the data */
{

    PDMA *pd;			/* pointer for the DMA control struct */
    long lcount;		/* local counter for xfers > MAX_XFER_SIZE */
    int i;			/* loop counter */


    PRINTD( targid, 0x1,
	("ds5000_setup entry sc:%x targ:%d dir:%d cnt:%d addr:%x\n",
	sc, targid, dir, count, addr ));

    /* Setup the DMA control structure for this target. */

    pd = &sc->pdma_ctrl[ targid ];	/* assign the pointer */

    /* If this pipe is not IDLE return PDMA_RETRY. */

    if( pd->pstate != PDMA_IDLE )
    {
	return( PDMA_RETRY );
    }
    
    pd->pstate = PDMA_SETUP;		/* setup is being done */
    pd->iodir = dir;			/* load direction */
    pd->count = count;			/* load total count */
    pd->data_addr = addr;		/* load data address */
    pd->usercnt = 0;			/* clear working count */
    pd->targcnt = 0;			/* clear working count */

    /* Flush/NULL out the buffers, security reasons.  [May remove ?] */

    if( sc->dma_pflags & PDMA_PREFLUSH )
    {
	for( i = 0; i < PIPE_BUF_NUM; i++ )
	{
	    (*sc->dma_bzero)
		( sc->pdma_rambuff + RAM_ADJUST(sc, pd->pb_offset[i]),
		MAX_XFER_SIZE ); 
	    wbflush();

	    pd->pb_flags[i] = PB_READY;		/* set to ready for data */
	}
    }

   /* If the I/O direction is a READ, load the two pipe buffer counters.  By
    definition buffer 0 will take presidence over buffer 1.  Buffer 0
    will be filled first the total count will also be decremented. */

    if( dir == SZ_DMA_READ )
    {
	for( i = 0; i < PIPE_BUF_NUM; i++ )
	{
	    lcount = XFER_BUFCNT( pd->count - pd->usercnt );	/* how much */
	    pd->usercnt += lcount;	/* update count for later */
	    pd->pb_count[i] = lcount;	/* load the count expected */
	    pd->pb_flags[i] = PB_LOADED;/* loaded for a READ :-) */

	    if( pd->usercnt == pd->count )	/* stop if no more data */
	    {
		break;			/* drop out of the for loop */
	    }
	}
    }
    else
    {
      /* If the I/O direction is a WRITE, pre fill the pipe buffers.  By
	definition buffer 0 will take presidence over buffer 1.  Buffer 0
	will be filled/emptied first. */

	for( i = 0; i < PIPE_BUF_NUM; i++ )
	{
            int len;
	    len = lcount = XFER_BUFCNT( pd->count - pd->usercnt );
            if( (pd->usercnt + lcount) > sc->sc_b_bcount[targid] )  {
	         /* Calc the new len to the end. */
	         len = sc->sc_b_bcount[targid] - pd->usercnt;

	         /* Zero out the half of the RAM buffer, this will deal with
	            the zero fill requirement. */

	         (*sc->dma_bzero)
		     ( sc->pdma_rambuff + RAM_ADJUST(sc, pd->pb_offset[i]),
		       lcount); 
	         wbflush();
	    }

	    /* Move the data to write into the RAM buffer. */

	    (*sc->dma_wbcopy)( (pd->data_addr + pd->usercnt),
		(sc->pdma_rambuff + RAM_ADJUST( sc, pd->pb_offset[i] )),
		len);
	    wbflush();

	    pd->usercnt += lcount;	/* update user space count */
	    pd->pb_count[i] = lcount;	/* load the count for this buffer */
	    pd->pb_flags[i] = PB_LOADED;
	    
	    if( pd->usercnt == pd->count )	/* stop if all data moved */
	    {
		break;			/* drop out of the for loop */
	    }
	}
    }
    return( PDMA_SUCCESS );		/* ready */
}

/************************************************************************

int pdma_start( sc, targid, opcode )

Inputs:
	sz_softc *sc;		 pointer to the softc structure 
	int targid;		 current targit ID 
	int opcode;		 value to start the dma oper in the chip 

Function:
	This routine is fairly straight forward.  It is responsible for loading
	what ever is necessary into the DMA engine and the SCSI chip.  The 
	dma control structure for the target should contain all the information.
	The SZ_DID_DMA flag in the softc structure will be set.  This informs
	the driver that data xfers are taking place.
	    NOTE: Most of the SCSI chips with the RAM buffer use offsets into
	    the buffer for the location of the data.  This is the reason for
	    storing the offsets instead of pointers.
	    NOTE: An opcode argument is passed to this routine mostly for the
	    3max/NCR 53c94.  In the SII, 5380 machines the sequence is fairly
	    "canned" to start up dma for the data phases.  However the 3max
	    plans to use these routines for all information phases.


Return:
	PDMA_SUCCESS		 all is ready and ok 
	PDMA_FAIL		 a fatal(?) problem has occured 

**************************************************************************/

int
ds5000_start( sc, targid, opcode )
    struct sz_softc *sc;	/* pointer to the softc structure */
    int targid;			/* current targit ID */
    int opcode;			/* value to start the dma oper in the chip */
{
    PDMA *pd;			/* pointer for the DMA control struct */
    ASC_REG *ascaddr; 		/* pointer for the SCSI chip registers */
    DMA_AR *ascar;		/* pointer for the SCSI DMA engine register */
	
    PRINTD( targid, 0x1,
	("ds5000_start entry sc:%x targ:%d op:%x\n", sc, targid, opcode ));

    /* Initialize local variables. */

    pd = &sc->pdma_ctrl[ targid ];	/* assign the pointer */
    ascaddr = (ASCREG *)sc->sc_scsiaddr;/* get the chip addr */
    ascar = (DMAAR *)sc->pdma_addrreg;	/* engine address */

    pd->opcode = opcode;		/* save for later use */

    /* Load the information from the dma control structure for buffer 0 into
    the SCSI chip and DMA engine. */

    ascar->rambuf_dma =			/* load addr reg */
	(((pd->iodir == SZ_DMA_WRITE) ? DMA_ASC_WRITE : 0 ) | 
    		(pd->pb_offset[0] & 0x00ffffff));
    wbflush();

    ASC_LOADCNTR(ascaddr, pd->pb_count[0]);	/* load counter */
    wbflush();

    /* Start the DMA operation in the ASC. */
    /* NOTE: for the 3max all this will not be necessary, probably just load
	passed opcode ?  */

    sc->sc_asccmd = pd->opcode;   
    ascaddr->asc_cmd = pd->opcode;
    wbflush();				/* clear write buffer */

    /* Set the pstate to inform the driver that DMA is in progress. */

    pd->pstate = PDMA_ACTIVE;
    pd->pb_flags[0] = PB_DMA_ACTIVE;
    sc->sc_szflags[ targid ] |= SZ_DID_DMA;

    return( PDMA_SUCCESS );
}

/************************************************************************

int pdma_cont( sc, targid )

Inputs:
	sz_softc *sc;		 pointer to the softc structure 
	int targid;		 current targit ID 

Function:
	Handle intermediate DMA functions during the transfer.  In the case
	of pipe buffers the SCSI chip/Engine will reach terminal count
	whenever one of the buffers is filled/emptied.   The active buffer
	for a DATA_IN will have the data moved to user space.  For a DMA_OUT
	the next buffer will be given to the SCSI chip, and the previous 
	buffer will be filled to get ready for the next pdma_cont() call.
	    NOTE: An assupmtion is made that for a DMA_OUT the next buffer is
	    always ready with valid data, if necessary.
	    NOTE: In the CMAX case the SII has a limitation in the data
	    counter.  It can only xfer 8k bytes with one DMA operation.  
	    This routine would only have to kick start the SII again.
    Question:  Is it fair to assume that this routine will only get called
	on TC and not phase change?  There is no checking on the current
	counter within the SCSI chip.

Return:
	The number of bytes transfered sofar for this DMA operation.

**************************************************************************/

int
ds5000_cont( sc, targid )
    struct sz_softc *sc;	/* pointer to the softc structure */
    int targid;			/* current target ID */
{
    PDMA *pd;			/* pointer for the DMA control struct */
    ASC_REG *ascaddr; 		/* pointer for the SCSI chip registers */
    DMA_AR *ascar;		/* pointer for the SCSI DMA engine register */
    int	ffr;			/* value for the FIFO flags */
    int	tcount;			/* corrected count for the ASC counters */
    int lcount;			/* local count from the transfers */
    int active;			/* index for the current active pipe buffer */
    int next;			/* index for the next active pipe buffer */

    PRINTD( targid, 0x1,
	("ds5000_cont entry sc:%x targ:%d\n", sc, targid));

    pd = &sc->pdma_ctrl[ targid ];	/* assign the pointer */
    ascaddr = (ASCREG *)sc->sc_scsiaddr;/* get the chip addr */
    ascar = (DMAAR *)sc->pdma_addrreg;	/* engine address */

    /* Find the current active buffer in the pipe.  The offset and counts are
    needed.  The return value will not be checked. */

    get_bindex( pd, &active, &next );	/* scan the buffers for DMA_ACTIVE */

    /* Do a quick check to determine there is still data that needs to be 
    "continued".  If the Target is slow in changing phase from DATA, the
    terminal count interrupt will occur before the phase change interrupt.
    This routine could be called and later a phase change occurs.  If there is
    no more data left to transfer just return the target count.  When the
    phase change interrupt finially happens the pdma_end() routine will clean
    up. */

    if( (pd->targcnt + pd->pb_count[ active ]) == pd->count )
    {
	PRINTD(targid, 0x1, ("dma_cont: returning (no i/o), targcnt = 0x%x\n",
		pd->targcnt));
	return( pd->targcnt );			/* nothing done */
    }

    /* From the active buffers count and the SCSI chips actual xfer counter
    register value, calc how much data was transfered.  Update the working
    counter(s) for the return xfer count.  So far the SCSI chip counters
    are all down counters, making the xfer count calc a simple subtaction.
    The counters in the ASC have to be corrected for the # of bytes
    left in the FIFO but not transferred when the target changed phase.
    Q: What if SCSI cntr != 0? [pdma_end should be called ?] */

    ffr =  (int)ascaddr->asc_ffss;	/* get the fifo flags count */
    ffr &= ASC_FIFO_MSK;		/* mask off sequ step bits */

    ASC_GETCNTR(ascaddr, tcount);	/* get the counter value */

    ASC_LOADCNTR(ascaddr, 0);		/* JAG - force TC bit off */
    wbflush();

    /* debug - since we only call this routine when the phase is the same AND
	   when TC is set, I believe that both of these should be zero. */
    if ((tcount != 0) || (ffr != 0))  {
	PRINTD(targid, 0x8000, ("ds5000_cont: tcount = %x, ffr = %x\n",
		tcount, ffr));
	}

    if( pd->iodir == SZ_DMA_WRITE )
        tcount += ffr;	/* update count by whats in FIFO on writes */

    lcount = pd->pb_count[ active ] - tcount;

    /* keep track of total number of bytes actually transferred */
    sc->sc_ascdmacount[targid] += lcount;

    /* Check the direction flag to determine what has to be done with the pipe
    buffers. */

    if( pd->iodir == SZ_DMA_READ )
    {
        /* If the operation is a DMA_IN: swap the buffers if there is more data
	that has to come in, startup the DMA, and then move the data from the
	now idle buffer to user space. */

        /* Load up the SCSI chip with the address and count for the next
	buffer. */

	/* Load addr reg, no need to set the write bit. */
	ascar->rambuf_dma = (pd->pb_offset[ next ] & 0x00ffffff);
	wbflush();

	ASC_LOADCNTR(ascaddr, pd->pb_count[ next ]);	/* load counter */
	wbflush();

	PRINTD(targid, 0x8000, 
	  ("ds5000_cont: (READ) starting new dma %x bytes\n", 
		pd->pb_count[next]));

        /* Start the DMA operation in the ASC. */

	sc->sc_asccmd = pd->opcode;   
	ascaddr->asc_cmd = pd->opcode;
	wbflush();				/* clear write buffer */

        /* Set the flag to inform the driver that DMA is in progress. */

	pd->pb_flags[ next ] = PB_DMA_ACTIVE;
	sc->sc_szflags[ targid ] |= SZ_DID_DMA;

        /* Switch the "active" pipe buffer from DMA_ACTIVE to CPU_ACTIVE, the
	SCSI chip is done with it.  Move the data into user space. */

	pd->pb_flags[ active ] = PB_CPU_ACTIVE;

	PRINTD(targid, 0x8000,
	  ("ds5000_cont: (READ) bcopying %x bytes to user space\n", 
		pd->pb_count[active]));

	if( pd->pb_count[active] != 0 )	/* don't bother if no data */
	{
	    (*sc->dma_rbcopy)			/* move the data */
		((sc->pdma_rambuff + RAM_ADJUST( sc, pd->pb_offset[ active ] )),
		(pd->data_addr + pd->targcnt), pd->pb_count[ active ] );
	    wbflush();
	}

	pd->targcnt += pd->pb_count[ active ];	/* update target count */
	pd->pb_flags[ active ] = PB_FINISHED;

        /* If there is still more data that will be transfered setup the old
	active buffer for the next interrupt. */

	if( (pd->count - pd->usercnt) != 0 )	/* still more data */
	{
	    /* Setup the count for the PDMA transfer. */

	    lcount = XFER_BUFCNT( pd->count - pd->usercnt );	/* how much */

	    pd->pb_count[ active ] = lcount;
	    pd->usercnt += lcount;		/* update count for later */
	    pd->pb_flags[ active ] = PB_READY;	/* set to ready for data */
	    PRINTD(targid, 0x8000,
	    ("ds5000_cont: (READ) more data, new pb_count = %x, usercnt = %x\n",
		lcount, pd->usercnt));
	}

    } /* end of (pd->iodir == SZ_DMA_READ) */
    else
    {
        /* If the operation is a DMA_OUT: the next buffer has valid data in it
        ready for the SCSI chip to access.  The DMA is started on the next
	buffer. The "active" buffer, which is now CPU_ACTIVE, is filled 
	if there is still more data to be moved from user space. */

        /* Load up the SCSI chip with the address and count for the next
	buffer. */

	/* Load addr reg, set the write direction bit. */
	ascar->rambuf_dma =
	    (DMA_ASC_WRITE | (pd->pb_offset[ next ] & 0x00ffffff));
	wbflush();

	ASC_LOADCNTR(ascaddr, pd->pb_count[ next ]);	/* load counter */
	wbflush();

        /* Start the DMA operation in the ASC. */

	sc->sc_asccmd = pd->opcode;   
	ascaddr->asc_cmd = pd->opcode;
	wbflush();				/* clear write buffer */

        /* Set the flag to inform the driver that DMA is in progress. */

	pd->pb_flags[ next ] = PB_DMA_ACTIVE;
	sc->sc_szflags[ targid ] |= SZ_DID_DMA;

        /* Update the target side counter.  The data has been moved to the
	target from the RAM buffer. */

	pd->targcnt +=  pd->pb_count[ active ]; /* update target count */

        /* Switch the "active" pipe buffer from DMA_ACTIVE to CPU_ACTIVE, the
	SCSI chip is done with it. */

	pd->pb_flags[ active ] = PB_CPU_ACTIVE;

        /* Check the count value, if there is more data to be preloaded into
	the now CPU_ACTIVE "active" buffer move it in and set the flags. */

	if( (pd->count - pd->usercnt) != 0 )		/* still more data */
	{
	    /* Setup the count for the PDMA transfer. */

            int len;
	    len = lcount = XFER_BUFCNT( pd->count - pd->usercnt );

	    if( (pd->usercnt + lcount) > sc->sc_b_bcount[targid] )  {
	        /* Calc the new len to the end. */
	        len = sc->sc_b_bcount[targid] - pd->usercnt;

	        /* Zero out the half of the RAM buffer, this will deal with
	           the zero fill requirement. */

	        (*sc->dma_bzero)
		   ( sc->pdma_rambuff + RAM_ADJUST(sc, pd->pb_offset[active]),
		    lcount); 
	        wbflush();
	  }

	  /* Move the data to write into the RAM buffer. */

	  (*sc->dma_wbcopy)( (pd->data_addr + pd->usercnt),
		(sc->pdma_rambuff + RAM_ADJUST(sc, pd->pb_offset[active])),
		len);
	  wbflush();

	  pd->usercnt += lcount;		/* update user count */
	  pd->pb_count[ active ] = lcount;
	  pd->pb_flags[ active ] = PB_LOADED;
	    
	}
	else
	{
	    pd->pb_flags[ active ] = PB_FINISHED;
	}

    } /* end of else on (pd->iodir == SZ_DMA_READ) */

    return( pd->targcnt );	/* return current progress */
}

/************************************************************************

int pdma_end( sc, targid )

Inputs:
	sz_softc *sc;		 pointer to the softc structure 
	int targid;		 current targit ID 

Function:
	Handle the completion of the DMA operation.  Free up the necessary
	DMA resources that were allocated in dma_setup().  Handle the final
	move of the data to user space if the operation was a read.

Return:
	The number of bytes transfered over the entire DMA operation.

**************************************************************************/

int
ds5000_end( sc, targid )
    struct sz_softc *sc;	/* pointer to the softc structure */
    int targid;			/* current targit ID */
{
    PDMA *pd;			/* pointer for the DMA control struct */
    ASC_REG *ascaddr; 		/* pointer for the SCSI chip registers */
    int	ffr;			/* value for the FIFO flags */
    int	tcount;			/* corrected count for the ASC counters */
    int lcount;			/* local count from the transfers */
    int active;			/* index for the current active pipe buffer */
    int next;			/* index for the next pipe buffer */
    int i;			/* buffer loop counter */
    int len;

    PRINTD( targid, 0x1,
	("ds5000_end entry sc:%x targ:%d\n", sc, targid));


    pd = &sc->pdma_ctrl[ targid ];	/* assign the pointer */
    ascaddr = (ASCREG *)sc->sc_scsiaddr;/* get the chip addr */

    /* Find the current active buffer in the pipe.  The offset and counts are
    needed.  The return value will not be checked. */

    get_bindex( pd, &active, &next );	/* scan the buffers for DMA_ACTIVE */

    /* From the active buffers count and the SCSI chips actual xfer counter
    register value, calc how much data was transfered.  Update the working
    counter(s) for the return xfer count.  So far the SCSI chip counters
    are all down counters, making the xfer count calc a simple subtaction.
    The counters in the ASC have to be corrected for the # of bytes
    left in the FIFO but not transferred when the target changed phase. */

    ffr =  (int)ascaddr->asc_ffss;	/* get the fifo flags count */
    ffr &= ASC_FIFO_MSK;		/* mask off sequ step bits */

    ASC_GETCNTR(ascaddr, tcount);	/* get the counter value */

    ASC_LOADCNTR(ascaddr, 0);		/* JAG - force TC bit off */
    wbflush(); 

    if ((tcount != 0) || (ffr != 0)) {
	  PRINTD(targid, 0x8000, ("ds5000_end: ffr = 0x%x\n", ffr));
    }

    tcount += ffr;			/* update count by whats in FIFO */

    lcount = pd->pb_count[ active ] - tcount;

    /* keep track of total number of bytes actually transferred */
    sc->sc_ascdmacount[targid] += lcount;

    /* If the last operation was a SZ_DMA_READ, move the data to user space. */

    if( pd->iodir == SZ_DMA_READ )
    {
      /* Switch the pipe buffer from DMA_ACTIVE to CPU_ACTIVE, the SCSI
	chip is done with it. */

	pd->pb_flags[ active ] = PB_CPU_ACTIVE;

	PRINTD(targid, 0x8000, ("ds5000_end: (READ) bcopy %x bytes\n", lcount));

        /* Move the READ data to user space. */
	if( lcount != 0 )		/* don't bother if no data */
	{
	    len = lcount;
	    /* Make sure we don't go beyond user's buffer */
            if( sc->sc_ascdmacount[targid] > sc->sc_b_bcount[targid] )  {
	         len = lcount - 
			(sc->sc_ascdmacount[targid] - sc->sc_b_bcount[targid]);
	    }
	    (*sc->dma_rbcopy)		/* move to user space */
		((sc->pdma_rambuff + RAM_ADJUST( sc, pd->pb_offset[ active ] )),
		(pd->data_addr + pd->targcnt), len);
	    wbflush();
	}
    }

    /* Update the target count, the data has moved between the RAM buffer and
    target. */

    pd->targcnt += lcount;		/* update working count */

    /* Clean up after ourselves.  Deallocate the pipe [for dynamic alloc].  Set
    all the flags to finished.  Clear the SZ_DID_DMA flag. [What else ?] */

    sc->sc_szflags[ targid ] &= ~SZ_DID_DMA;	/* DID_DMA is done */

    for( i = 0; i < PIPE_BUF_NUM; i++ )
    {
	pd->pb_flags[i] = PB_FINISHED;		/* just for completion */
    }
    pd->pstate = PDMA_IDLE;			/* all done */

    /* Return the accumulated working count to the driver.  No checking has
    been done to verify that all the data has been transfered. */

    return( pd->targcnt );
}

/************************************************************************

/* Support routines. */

/************************************************************************

/* This routine will return through the "active/next" pointers the
index of the pipe buffers that are currently "active" and which one is
"next".  The currently active buffer will have the PB_DMA_ACTIVE flag
set in it's flags field.  The "next" buffer will be the next buffer in
line.  There will be no checking on the state of the "next" buffer,
this routine can be called for a DMA_IN or DMA_OUT phase.  This routine
is very tightly bound to the number of buffers in the pipe.  At this
time the implementation is based on 2, PIPE_BUF_NUM, buffers per pipe.
If the number of buffers per pipe changes this routine will have to
change to reflect the change.
    NOTE: one possible idea for multiple buffers is to find the first
    one that has PB_DMA_ACTIVE and for the next index increment to the
    following buffer checking for wrap around.

**************************************************************************/

int get_bindex( pd, active, next )
    PDMA *pd;			/* ptr for the control structure */
    int *active;		/* holder for the active index */
    int *next;			/* holder for the active index */
{
  /* Find the current active buffer in the pipe.  The offset and counts are
    needed.  A cascadding if/else sequence is used here, there is no reason
    why there cannot be more than 2 buffers w/in the pipe.  For just 2 buffers
    this quick. */

    if( pd->pb_flags[0] == PB_DMA_ACTIVE )
    {
	*active = 0;
	*next = 1;
    }
    else if( pd->pb_flags[1] = PB_DMA_ACTIVE )
    {
	*active = 1;
	*next = 0;
    }
    else
    {
	*active = 0;			/* default error fall through */
	*next = 1;
	return( PDMA_FAIL );		/* just in case */
    }
    return( PDMA_SUCCESS );		/* all done and selected */ 
}

/************************************************************************/

int
ds5000_flush( sc, targid )
    struct sz_softc *sc;	/* pointer to the softc structure */
    int targid;			/* current targit ID */
{
    PDMA *pd;			/* pointer for the DMA control struct */
    int i;			/* buffer loop counter */

    PRINTD( targid, 0x1,
	("ds5000_flush entry sc:%x targ:%d\n", sc, targid));

    pd = &sc->pdma_ctrl[ targid ];	/* assign the pointer */
    for( i = 0; i < PIPE_BUF_NUM; i++ )
    {
      /* Flush, or zero out the pipe buffer. */

	(*sc->dma_bzero)
	    ( sc->pdma_rambuff + RAM_ADJUST(sc, pd->pb_offset[i]),
	    MAX_XFER_SIZE );
	wbflush();
    }
    return( PDMA_SUCCESS );		/* all done */ 
}

/************************************************************************/
