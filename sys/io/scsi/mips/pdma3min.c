#ifndef lint
static char *sccsid = "@(#)pdma3min.c	4.6      (ULTRIX)  4/4/91";
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
 * pdma_3min.c
 *
 * Pseudo DMA routines for the PMAZ-BA a.k.a. 3min.
 *
 * Modification history:
 *
 * 15 Aug 1990  Robert Scott
 * Created this file to implement PDMA code for 3min hardware.  Based upon
 * 3max PDMA code.
 *
 ************************************************************************/
/* ---------------------------------------------------------------------- */

#define ENTRYEXIT	0x00010000
#define REGLOAD		0x00020000
#define DETAILDAT	0x00040000
#define INTTRACE	0x00080000
#define	PADTRACK	0x00100000	
#define DUMPHEX		0x00200000
#define PASS2		0x00400000
#define FRAG		0x00800000

#include "../data/scsi_data.c"
#include "../io/tc/tc.h"
#include "scsi_debug.h"
#include "pdma3min.h"

/* ---------------------------------------------------------------------- */
/* External functions and variables. */

extern int scsidebug;
extern PDMA_ENTRY pdma_entry[];		/* the entry array */

/* ---------------------------------------------------------------------- */
/* Local Data area. */

/* Entry structure for the pseudo DMA routines for the PMAZBA.  The external
declarations are needed for the forward reference. */

extern int pmaz_ba_init();
extern int pmaz_ba_setup();
extern int pmaz_ba_start();
extern int pmaz_ba_cont();
extern int pmaz_ba_end();
extern int pmaz_ba_flush();
extern int bcopy();
extern int bzero();

PDMA_ENTRY pdma_ds5000_100 =
{
    DS_5000_100,				/* cpu type value */
    ( 0 ),				/* MISC control flags */
    pmaz_ba_init,
    pmaz_ba_setup,
    pmaz_ba_start,
    pmaz_ba_cont,
    pmaz_ba_end,
    bcopy,				/* kernel routines can be used */
    bcopy,
    bzero,
    pmaz_ba_flush
};

int targmatch = 9;
int targtmp;

/* ---------------------------------------------------------------------- */
/*
unsigned *ioa_addrcvt( addr )

Inputs:
	char *addr;		 K2SEG address for the data 

Function:
 	Convert input virtual address to physical which, in turn, must
        be modified to meet IOASIC format requirements.
Return:
	Physical memory address in IOASIC format.
*/

unsigned *ioa_addrcvt ( addr )
char *addr;
    {
    unsigned long a = (unsigned long) addr;
    unsigned long p;

    if ( IS_KUSEG( a ) )
        {
        printf( "ioa_addrcvt: KUSEG address passwd in!\n" );
        p = a;
	return 0;
        }
    p = svtophy( a );
    if ( p == 0 )
        {
        printstate |= PANICPRINT;
        printf(  "ioa_addrcvt: Invalid return from svtophy().\n" );
        printf( "ioa_addrcvt: Address 0x%x maps to physical address 0.\n",a );
	return 0;
        }
    a = ( p & 0x1ffffffc ) << 3; 
    return (unsigned *) a; 
    }

/* ---------------------------------------------------------------------- */
/*
void *backcvt( addr )

Inputs:
	void *addr;		 IOASIC format physical address 

Function:
        Convert an IOASIC format physical address into normal physical
        address format.

Return:
	Physical memory address in standard format.
*/

void *backcvt( void *addr )
    {
    unsigned long a = (unsigned long) addr;
    unsigned long p;
    p = a >> 3;
    return (void *)( PHYS_TO_K0( p ));
    }

/* ---------------------------------------------------------------------- */
/*
int pdma_init( sc )

Inputs:	sz_softc pointer

Function:
	Initialize all that is necessary for the DMA engine/code to run 
	for the particular system.  For the 3min, DAT table space and
        fragment buffer space must be allocated.

	The PDMA related fields will be cleared/initialized and one of the
	pipe buffers will be set to READY.

Return:
	PDMA_SUCCESS		 all is ready and ok 
	PDMA_FAIL		 a fatal(?) problem has occured 
*/

int pmaz_ba_init( sc )
struct sz_softc *sc;
    {
    int targid;			/* loop counter for targets */
    PDMA *pd;			/* pointer for the DMA control struct */
    int *slotp;			/* IOASIC SCSI DMA slot register pointer */
    static char *fbp=0;                /* Fragment buffer pool pointer */

/*  Setup the DMA engine address pointers in the softc.  The
    base address for these and the SCSI chip should have already been 
    set in the softc by the particular driver's probe code. */

/*  In contradiction with the previous comment, this line should be done 
    somewhere in scsi_asc.c? */
    sc->ioasicp =  (char *) PHYS_TO_K1( BASE_IOASIC );

  /* The IOASIC SCSI DMA slot register MUST be initialized before any
    communication with the '94 is attempted.  */

    slotp = (int *) ( (unsigned)sc->ioasicp + SCSI_DMASLOT_O);  

    *slotp = SCSI_SLOT_DATA;	/* magic cookie as found in v1.6 of spec */
                           /* FIX: This may change with PMAZ-BA's */

  /* For each of the target slots on a SCSI bus, this is one of the valid
    magic #'s of 8.  Setup and assign the pointers/offsets for each of the
    DMA control structures. */

  /* Allocate a chunk of memory for the fragment buffers and DAT table 
     for each target */

    if ( !fbp )
        KM_ALLOC( fbp, char *, (sizeof( FRAGBUF )+DATTBL_SIZ)*8, KM_DEVBUF,
            KM_NOW_CL_CO_CA|KM_NOCACHE );

    for( targid = 0; targid < NDPS; targid++ )
        {
	pd = &sc->pdma_ctrl[ targid ];	/* assign the pointer */

      /* Clear the per call elements. */
	pd->pstate = PDMA_IDLE;		/* clear out misc flags */
	pd->iodir = 0;			/* clear direction */
	pd->count = 0;			/* clear total count */
	pd->data_addr = (char *)NULL;	/* clear data address */

     /* The DAT table and fragment buffer for a given target must be
        extracted from the large buffer kmalloc'ed above.  One might
        think of the large buffer as being an array (of length 8) of
        structures containing a DAT table followed by a fragment 
        buffer for each target.  Then again, one might not.
     */
        pd->dat_addr = (char *) ( fbp +
            targid * ( DATTBL_SIZ + sizeof( FRAGBUF ) ) );
        pd->frag_bufp = (char *) ( fbp + 
            targid * ( DATTBL_SIZ + sizeof( FRAGBUF ) ) +
            DATTBL_SIZ );
#ifdef PDMADEBUG
        PRINTD( targid, DETAILDAT, 
            ("init:  pd->dat_addr = 0x%x\n", pd->dat_addr ));
        PRINTD( targid, DETAILDAT, 
            ("init:  pd->frag_bufp = 0x%x\n", pd->frag_bufp ));
#endif

	pd->usercnt = 0;		/* user space count */
	pd->targcnt = 0;		/* target count */

	}
    return( PDMA_SUCCESS );
    }

/* ---------------------------------------------------------------------- */
/*
int pdma_setup( sc, targid, dir, count, addr )

Inputs:
	sz_softc *sc;		 pointer to the softc structure 
	int targid;		 current target ID 
	int dir;		 direction for I/O xfer 
	long count;		 number of bytes to xfer 
	char *addr;		 address for the data 

Function:
Return:
	PDMA_SUCCESS		 all is ready and ok 
	PDMA_FAIL		 a fatal(?) problem has occured 
	PDMA_RETRY(?)		 unable to setup, try again later 
*/

int pmaz_ba_setup( sc, targid, dir, count, addr )
struct sz_softc *sc;	/* pointer to the softc structure */
int targid;			/* current target ID */
int dir;			/* direction for I/O xfer */
long count;			/* number of bytes to xfer */
char *addr;			/* address for the data */
    {
    PDMA *pd;			/* pointer for the DMA control struct */
    int i;			/* loop counter */

#ifdef PDMADEBUG
    PRINTD( targid, ENTRYEXIT,
        ("pmaz_ba_setup: entry sc=0x%x targ=%d dir=%d cnt=%d addr=0x%x\n  bcount=%d dmaxfer=%d\n",
        sc, targid, dir, count, addr, sc->sc_b_bcount[targid], 
        sc->sc_dmaxfer[targid] ));
#endif

  /* Setup the DMA control structure for this target. */
    pd = &sc->pdma_ctrl[ targid ];	/* assign the pointer */
    targtmp = targid;			/* used for debugging purposes only */

  /* If this pipe is not IDLE return PDMA_RETRY. */
    if( pd->pstate != PDMA_IDLE )
        {
        PRINTD( targid, ENTRYEXIT, ("pmaz_ba_setup: exit - PDMA_RETRY\n"));
	return( PDMA_RETRY );
        }
    
    pd->pstate = PDMA_SETUP;		/* setup is being done */
    pd->iodir = dir;			/* load direction */

    /* must save difference between requested xfer count and real count */
    /* in order to pad out the xfer with the '94 */
    pd->count = sc->sc_b_bcount[targid] - sc->sc_dmaxfer[targid];
    /* OK, so this doesn't have anything to do with the sii, we don't 
       need another counter in the softc structure and I don't like
       using #defines to mask things.  
    */
    sc->sc_siidmacount[targid] = count - pd->count;
#ifdef PDMADEBUG
    if ( scsidebug & PADTRACK && targmatch == targid )
        {
        if ( sc->sc_siidmacount[targid] )
        cprintf("setup: Padding %d -> %d : %d\n", count, pd->count,
            sc->sc_siidmacount[targid] );
        }
#endif

    pd->data_addr = addr;		/* load data address */
    pd->usercnt = 0;			/* clear working count */
    pd->targcnt = 0;			/* clear working count */


  /* Build the DAT table for this transfer. */
    if ( blddattbl( targid, pd, pd->count, addr, dir ) == PDMA_FAIL )
        {
        printf( "setup: dat table build failure\n" );
        printf( "  target=%d dat_addr=0x%x pd->count=%d addr=0x%x dir=%d\n", 
            targid, pd->dat_addr, pd->count, addr, dir );
        dumptbl( (DTENT *)pd->dat_addr );
        return( PDMA_FAIL );
        }

  /* The DAT indexing scheme always begins with 1 as entry 0 is reserved
     in case 'backing-up' is necessary.  This could occur due to a
     disconnect which leaves a segment of less than a double word in
     length to be transferred.  In such a case, the remainder of the 
     segment interrupted would be broken into two portions:  A transfer of
     less than a double word into a local buffer followed by a normal DMA
     transfer.
  */
    pd->dat_index = 1;

#ifdef PDMADEBUG
    if ( scsidebug & DETAILDAT && ( targid==targmatch ) ) 
        dumptbl( (DTENT *)pd->dat_addr );
#endif
  /* If the setup is for a write and there the table begins with a 'special
     case' transfer from a local buffer, fill that buffer right now.
  */
    if ( ( dir == SZ_DMA_WRITE ) && is_local_xfer( pd ) )
        frag_buf_load( pd );

#ifdef PDMADEBUG
    PRINTD( targid, ENTRYEXIT, ("pmaz_ba_setup: exit\n"));
#endif
    return( PDMA_SUCCESS );		/* ready */
    }

/* ---------------------------------------------------------------------- */
/* 
int pdma_start( sc, targid, opcode )

Inputs:
	sz_softc *sc;		 pointer to the softc structure 
	int targid;		 current target ID 
	int opcode;		 value to start the dma oper in the chip 

Function:
	This routine is fairly straight forward.  It is responsible for loading
	what ever is necessary into the DMA engine and the SCSI chip.  The 
	dma control structure for the target should contain all the information.
	The SZ_DID_DMA flag in the softc structure will be set.  This informs
	the driver that data xfers are taking place.
	    NOTE: An opcode argument is passed to this routine mostly for the
	    NCR 53c94.  

Return:
	PDMA_SUCCESS		 all is ready and ok 
	PDMA_FAIL		 a fatal(?) problem has occured 
*/

int pmaz_ba_start( sc, targid, opcode )
struct sz_softc *sc;	/* pointer to the softc structure */
int targid;			/* current target ID */
int opcode;			/* value to start the dma oper in the chip */
    {
    PDMA *pd;			/* pointer for the DMA control struct */
    ASC_REG *ascaddr; 		/* pointer for the SCSI chip registers */
    DMA_AR *ioasicp;		/* pointer for the SCSI DMA engine register */
    DTENT *datp, *datep;	/* pointer to DAT table and individual entry */

#ifdef PDMADEBUG
    PRINTD( targid, ENTRYEXIT, 
        ("pmaz_ba_start: entry sc=0x%x targ=%d op=0x%x, ",sc, targid, opcode ));
#endif
  /* Initialize local variables. */

    pd = &sc->pdma_ctrl[ targid ];		/* assign the pointer */
#ifdef PDMADEBUG
    PRINTD( targid, DETAILDAT, ("    dat_index=%d\n", pd->dat_index ));
#endif
    ascaddr = (ASCREG *)sc->sc_scsiaddr;	/* get the chip addr */
    ioasicp = (DMA_AR *)sc->ioasicp;	/* engine address */

    pd->opcode = opcode;		/* save for later use */

  /* Load the information from the dma control structure for buffer 0 into
    the SCSI chip and DMA engine. */

    datp = (DTENT *)pd->dat_addr;
    datep = (DTENT *)&(datp[pd->dat_index]);
#ifdef PDMADEBUG
    if ( scsidebug & DETAILDAT && ( targid==targmatch ) ) 
        dumpent( datep );
#endif

    if ( datep->length==0 )	/* handle special case of padded xfer */
	{
        ASC_LOADCNTR(ascaddr, sc->sc_siidmacount[targid]);	/* load counter */
        wbflush();				/* clear write buffer */
        sc->sc_asccmd = ASC_DMA | ASC_XPAD ; 
        ascaddr->asc_cmd = sc->sc_asccmd;
        sc->sc_szflags[ targid ] |= SZ_DID_DMA;
        wbflush();				/* clear write buffer */
        return PDMA_SUCCESS;
	}

    if ( datep->addr == 0 )
        {
        printf( "start: Invalid DAT entry.\n" );
        dumpent( datep );
        }

    if ( is_local_xfer( pd ) && datep->dir == IOASIC_WRITE )
        {
        pd->pstate = PDMA_ACTIVE;
        sc->sc_szflags[ targid ] |= SZ_DID_DMA | SZ_PIO_INTR;
        asc_FIFOsenddata (sc, ASC_XINFO, datep->uadr, datep->length );
        }
    else
        {
	setscsictrl( sc, 0 );
        if ( dmapload( sc, targid, datep->iadr ) == PDMA_FAIL )
            {
            printf( "pdma_start: Failure return from dmapload\n" );
            return PDMA_FAIL;
            }
        ssrdmaon( sc, datep->dir );
    
#ifdef PDMADEBUG
        PRINTD( targid, REGLOAD, 
            ("    start: loading ASC counter (base 0x%x) with length (%d)\n",
            ascaddr, datep->length ));
#endif
        ASC_LOADCNTR(ascaddr, datep->length);	
    
        wbflush();

        sc->sc_asccmd = pd->opcode;   
        ascaddr->asc_cmd = pd->opcode;
        wbflush();				/* clear write buffer */
        }

  /* Set the pstate to inform the driver that DMA is in progress. */

    pd->pstate = PDMA_ACTIVE;
    sc->sc_szflags[ targid ] |= SZ_DID_DMA;
#ifdef PDMADEBUG
    PRINTD( targid, ENTRYEXIT, ("pdma_start: exit\n"));
#endif
    return PDMA_SUCCESS;
    }

/* ---------------------------------------------------------------------- */
/*
int pdma_cont( sc, targid )

Inputs:
	sz_softc *sc;		 pointer to the softc structure 
	int targid;		 current target ID 

Function:
	Handle intermediate DMA functions during the transfer.  

Return:
	The number of bytes transfered sofar for this DMA operation.
*/

int pmaz_ba_cont( sc, targid )
struct sz_softc *sc;	/* pointer to the softc structure */
int targid;			/* current target ID */
    {
    PDMA *pd;			/* pointer for the DMA control struct */
    ASC_REG *ascaddr; 		/* pointer for the SCSI chip registers */
    DMA_AR *ioasicp;		/* pointer for the SCSI DMA engine register */
    DTENT *datp, *datep;	/* pointer to DAT table and individual entry */

    char *newbufp;		/* buffer pointer for use in DAT table adj.  */
    int ecount;			/* used in DAT table rebuilding */
    int tidx;			/* temporary index variable in DAT rebuild */
    int	ffr;			/* value for the FIFO flags */
    int dbcount;		/* number of bytes in IOASIC data buffer */
    int	tcount;			/* corrected count for the ASC counters */
    int lcount;			/* local count from the transfers */
    int tmp1;

    pd = &sc->pdma_ctrl[ targid ];	/* assign the pointer */
#ifdef PDMADEBUG
    PRINTD( targid, ENTRYEXIT, 
        ("pmaz_ba_cont: entry sc:%x targ:%d\n", sc, targid));
    PRINTD( targid, ENTRYEXIT, ("    dat_index:%d\n", pd->dat_index ));
#endif
    ascaddr = (ASCREG *)sc->sc_scsiaddr;/* get the chip addr */
    ioasicp = (DMAAR *)sc->ioasicp;	/* engine address */

  /* First pull out the DAT table entry corresponding to this xfer */
    datp = (DTENT *)pd->dat_addr;
    datep = (DTENT *)&(datp[pd->dat_index]);
#ifdef PDMADEBUG
    if ( scsidebug & DETAILDAT && ( targid==targmatch ) ) 
        dumpent( datep );
#endif

    if ( !(datep->length) )	/* handle special case of padded xfer */
	{
        sc->sc_szflags[ targid ] |= SZ_DID_DMA;
        PRINTD( targid, PADTRACK, ("    cont:  padded xfer return: %d bytes\n",
                pd->targcnt + sc->sc_siidmacount[targid] ));
        return pd->targcnt + sc->sc_siidmacount[targid];
	}

  /* From the active buffers count and the SCSI chips actual xfer counter
    register value, calc how much data was transfered.  Update the working
    counter(s) for the return xfer count.  So far the SCSI chip counters
    are all down counters, making the xfer count calc a simple subtaction.
    The counters in the ASC have to be corrected for the # of bytes
    left in the FIFO but not transferred when the target changed phase.

    It should be noted that the 3min system (or possibly the softc struct)
    exhibits a situation which should not occur; the TC bit in the status
    register is set but the dma xfer counter is non-zero.
  */
    ssrdmaoff( sc );			/* turn off DMA */
    ffr =  (int)ascaddr->asc_ffss;	/* get the fifo flags count */
    ffr &= ASC_FIFO_MSK;		/* mask off sequ step bits */

    ASC_GETCNTR(ascaddr, tcount);	/* get the counter value */
    ASC_LOADCNTR(ascaddr, 0);	             /* JAG - force TC bit off */
    wbflush();

    dbcount = getdbcount( sc, datep->dir );  /* bytes stuck in IOASIC? */

    if ( datep->dir==IOASIC_WRITE )  /* for RDAT bug */
        lcount = datep->length - ffr;
    else
        lcount = datep->length;

    if ( (lcount & 1) && dbcount )	/* if odd, then dbcount is off by 1 */
        dbcount--;			/* RPS 03/28/91 */

    if ( is_local_xfer( pd ) )
        newbufp = datep->uadr + lcount;
    else
        newbufp = datep->addr + lcount;

    if( pd->iodir == SZ_DMA_READ )
        {
        if ( dbcount )			/* data was left in the IOASIC     */
            flushdb( sc, dbcount );
#ifdef PDMADEBUG
        if ( scsidebug & DUMPHEX && targid == targmatch )
            {
            if ( lcount > 0x40 )
                {
                dumphex(datep->addr, 0x20 );
                dumphex(datep->addr+lcount-0x20, 0x20 );
                }
            else 
                dumphex( datep->addr, lcount );
            }
#endif
      /* If the operation is a DMA_IN: check for fragment buffer usage,
         clean out that data if necessary, then determine if new DAT table
         entries must be built or modified before continuing */
        if ( is_local_xfer( pd ))	/* a local buffer was in addr?     */
            {
            flush_fragbuf( pd, lcount );
            }
        else				/* completed normally */
            {
            if ( IS_KUSEG( datep->addr ) )
                panic("pdmacont(2): KUSEG address passwd in!\n" );
            else
                clean_dcache( PHYS_TO_K0( svtophy( datep->addr ) ), 
                    datep->length );
            datep->completed = 1;	/* tag the entry complete */
            pd->targcnt += datep->length;  /* bump transfer counter */
            pd->dat_index++;
            }

      /* Load up the IOASIC chip with the address and '94 with the count 
         for the next buffer. */
        datep = (DTENT *)&(datp[pd->dat_index]);
#ifdef PDMADEBUG
        if ( scsidebug & DETAILDAT && ( targid==targmatch ) ) 
            {
            PRINTD( targid, DETAILDAT, 
                ("    cont: dat index=%d\n", pd->dat_index ));
            dumpent( datep );
            }
#endif
        if ( datep->length != 0 )
            {
	    setscsictrl( sc, 0 );
            if ( dmapload( sc, targid, datep->iadr ) == PDMA_FAIL )
                {
                printf( "pdma_start: Failure return from dmapload\n" );
                return PDMA_FAIL;
                }
            ssrdmaon( sc, datep->dir );
#ifdef PDMADEBUG
            PRINTD( targid, REGLOAD, 
                ("    cont: loading ASC counter (base 0x%x) with length (%d)\n",
                ascaddr, datep->length ));
#endif
            ASC_LOADCNTR(ascaddr, datep->length);	/* load counter */
            wbflush();

      /* Start the DMA operation in the ASC. */
            sc->sc_asccmd = pd->opcode;   
            ascaddr->asc_cmd = pd->opcode;
            wbflush();				/* clear write buffer */

      /* Set the flag to inform the driver that DMA is in progress. */
            sc->sc_szflags[ targid ] |= SZ_DID_DMA;
            } /* end of (datep->length != 0 ) */
        else
            {  /* time to pad out an xfer */
            ASC_LOADCNTR(ascaddr, sc->sc_siidmacount[targid]);	/* load counter */
            wbflush();				/* clear write buffer */
            sc->sc_asccmd = ASC_DMA | ASC_XPAD ; 
            ascaddr->asc_cmd = sc->sc_asccmd;
            sc->sc_szflags[ targid ] |= SZ_DID_DMA;
            wbflush();				/* clear write buffer */
            }
        } /* end of (pd->iodir == SZ_DMA_READ) */
    else
        {                              /* Write case now */
        datep->completed = 1;          /* tag the entry complete */
        pd->targcnt += datep->length;  /* bump transfer counter */
        pd->dat_index++;
        datep = (DTENT *)&(datp[pd->dat_index]);
        if ( is_local_xfer( pd ) )	/* is this a fragment? */
            {
            if ( frag_buf_load( pd ) == PDMA_FAIL )
                {
                printf( "pmaz_ba_cont(3min)(12): (WRITE(2)) frag buffer load, dat_addr=0x%x index= %d\n",
                    pd->dat_addr, pd->dat_index );
                return( PDMA_FAIL );
                }
            }

#ifdef PDMADEBUG
        if ( scsidebug & DETAILDAT && ( targid==targmatch ) ) 
            {
            PRINTD( targid, DETAILDAT, 
                ("    cont: dat index=%d\n", pd->dat_index ));
            dumpent( datep );
            }
#endif
        if ( is_local_xfer( pd ) && datep->dir == IOASIC_WRITE )
            {
            sc->sc_szflags[ targid ] |= SZ_DID_DMA | SZ_PIO_INTR;
            asc_FIFOsenddata (sc, ASC_XINFO, datep->uadr, datep->length );
            }
        else
            {
            if ( datep->length != 0 )
                {
                /* Load addr reg, set the write direction bit. */
        	    setscsictrl( sc, 0 );
                if ( dmapload( sc, targid, datep->iadr ) == PDMA_FAIL )
                    {
                    printf( "pdma_start: Failure return from dmapload\n" );
                    return PDMA_FAIL;
                    }
                ssrdmaon( sc, datep->dir );
#ifdef PDMADEBUG
                PRINTD( targid, REGLOAD, 
                    ("    cont: loading ASC counter (base 0x%x) with length (%d)\n",
                    ascaddr, datep->length ));
#endif
                ASC_LOADCNTR(ascaddr, datep->length);	/* load counter */
                wbflush();
    
          /* Start the DMA operation in the ASC. */
                sc->sc_asccmd = pd->opcode;   
                ascaddr->asc_cmd = pd->opcode;
                wbflush();				/* clear write buffer */

          /* Set the flag to inform the driver that DMA is in progress. */
    
                sc->sc_szflags[ targid ] |= SZ_DID_DMA;
                } /* end of if ( datep->length != 0 ) */
            else
                {  /* time to pad out an xfer */
                ASC_LOADCNTR(ascaddr, sc->sc_siidmacount[targid]);	/* load counter */
                wbflush();				/* clear write buffer */
                sc->sc_asccmd = ASC_DMA | ASC_XPAD ; 
                ascaddr->asc_cmd = sc->sc_asccmd;
                sc->sc_szflags[ targid ] |= SZ_DID_DMA;
                wbflush();				/* clear write buffer */
                }
            }
        } /* end of else on (pd->iodir == SZ_DMA_READ) */
#ifdef PDMADEBUG
    PRINTD( targid, ENTRYEXIT, ("pmaz_ba_cont:  exit\n" ));
#endif
    return pd->targcnt;	/* return current progress */
    }

/* ---------------------------------------------------------------------- */
/*

int pdma_end( sc, targid )

Inputs:
	sz_softc *sc;		 pointer to the softc structure 
	int targid;		 current target ID 

Function:
	Handle the completion of the DMA operation.  Free up the necessary
	DMA resources that were allocated in dma_setup().  Handle the final
	move of the data to user space if the operation was a read.

Return:
	The number of bytes transfered over the entire DMA operation.
*/

int pmaz_ba_end( sc, targid )
struct sz_softc *sc;            /* pointer to the softc structure           */
int targid;			/* current target ID                        */
    {
    PDMA *pd;			/* pointer for the DMA control struct       */
    ASC_REG *ascaddr; 		/* pointer for the SCSI chip registers      */
    DTENT *datp, *datep;	/* pointer to DAT table and individual entry */
    int	ffr;			/* value for the FIFO flags                 */
    int dbcount;		/* number of bytes in IOASIC data buffer    */
    int	tcount;			/* corrected count for the ASC counters     */
    int lcount;			/* local count from the transfers           */
    int i;			/* buffer loop counter                      */
    int tmp1;
    DMA_AR *ioasicp;		/* pointer for the SCSI DMA engine register */

    pd = &sc->pdma_ctrl[ targid ];	/* assign the pointer */
    ioasicp = (DMAAR *)sc->ioasicp;	/* engine address */
#ifdef PDMADEBUG
    PRINTD( targid, ENTRYEXIT, 
        ("pmaz_ba_end: entry sc=0x%x targ=%d\n", sc, targid));
    PRINTD( targid, DETAILDAT, ("    dat_index=%d\n", pd->dat_index ));
#endif

    ascaddr = (ASCREG *)sc->sc_scsiaddr;/* get the chip addr */

  /* From the active buffers count and the SCSI chips actual xfer counter
    register value, calc how much data was transfered.  Update the working
    counter(s) for the return xfer count.  So far the SCSI chip counters
    are all down counters, making the xfer count calc a simple subtaction.
    The counters in the ASC have to be corrected for the # of bytes
    left in the FIFO but not transferred when the target changed phase. */

    ssrdmaoff( sc );			/* turn off DMA */
    ffr =  (int)ascaddr->asc_ffss;	/* get the fifo flags count */
    ffr &= ASC_FIFO_MSK;		/* mask off sequ step bits */

    ASC_GETCNTR(ascaddr, tcount);	/* get the counter value */

    ASC_LOADCNTR(ascaddr, 0);		/* JAG - force TC bit off */
    wbflush();

  /* Pull out the DAT table entry corresponding to this xfer */
    datp = (DTENT *)pd->dat_addr;
    datep = (DTENT *)&(datp[pd->dat_index]);

    dbcount = getdbcount( sc, datep->dir );  /* bytes stuck in IOASIC? */

    datep->completed = 1;	/* tag the entry complete */
    if ( !datep->length )		/* take care of empty last DAT */
        {
        sc->sc_szflags[ targid ] &= ~SZ_DID_DMA;	/* DID_DMA is done */
        sc->sc_szflags[ targid ] &= ~SZ_PIO_INTR;	/* DID_DMA is done */

        pd->pstate = PDMA_IDLE;			/* all done */
        return( pd->targcnt + sc->sc_siidmacount[targid] );
        }

#ifdef PDMADEBUG
    if ( scsidebug & DETAILDAT && ( targid==targmatch ) ) 
        dumpent( datep );
#endif

    if ( (sc->sc_asc_sr&ASC_TC) && (tcount != 0) )	/* shouldn't happen */
        {
        if ( datep->dir == IOASIC_WRITE ) 
            tcount = 0;
	}

    lcount = datep->length;		/* Assume all transferred          */

    if ( sc->sc_asc_sr & ASC_TC )	/* terminal count is not set?      */
        tcount = 0;
    else
        lcount -= tcount;		/* then subtract remainder         */

    if ( datep->dir == IOASIC_WRITE )	/* in the case of writes...        */
        lcount -= ffr;		/* don't forget the fifo flags reg */

  /* If the last operation was a SZ_DMA_READ, move the data to user space. */

    if ( (lcount & 1) && dbcount )	/* if odd, then dbcount is off by 1 */
        dbcount--;			/* RPS 03/28/91 */

    if( pd->iodir == SZ_DMA_READ )
        {
        if ( dbcount )			/* data was left in the IOASIC     */
            flushdb( sc, dbcount );
#ifdef PDMADEBUG
        if ( scsidebug & DUMPHEX && targid == targmatch )
            {
            if ( lcount > 0x40 )
                {
                dumphex(datep->addr, 0x20 );
                dumphex(datep->addr+lcount-0x20, 0x20 );
                }
            else 
                dumphex( datep->addr, 0x20 );
            }
#endif
        if ( is_local_xfer( pd ))	/* a local buffer was in addr?     */
            {
	/* MOVE THE FRAGMENT BUFFER BYTES TO USER BUFFER HERE */
            if( lcount != 0 )	/* don't bother if no data */
                {	/* Change this to a flush_frag_buf() */
                flush_fragbuf( pd, lcount );
                wbflush();
                pd->targcnt += lcount;	/* update target count */
                }
            }
        else 
            {
            clean_dcache( PHYS_TO_K0( svtophy( datep->addr ) ), datep->length );
            pd->targcnt += datep->length - tcount;
            }
        }			/* end of if read */
    else
        {	/* write case */
        pd->targcnt += lcount;	/* update target count */
        }
  /* Clean up after ourselves.  Clear the SZ_DID_DMA flag. */

    sc->sc_szflags[ targid ] &= ~SZ_DID_DMA;	/* DID_DMA is done */
    sc->sc_szflags[ targid ] &= ~SZ_PIO_INTR;	/* DID_DMA is done */

    pd->pstate = PDMA_IDLE;			/* all done */

  /* Return the accumulated working count to the driver.  No checking has
    been done to verify that all the data has been transfered. */
#ifdef PDMADEBUG
    PRINTD( targid, ENTRYEXIT, ("pmaz_ba_end: exit\n"));
#endif
    return( pd->targcnt );
    }

int pmaz_ba_flush( sc, targid )
struct sz_softc *sc;	/* pointer to the softc structure */
int targid;			/* current targit ID */
    {
    PDMA *pd;			/* pointer for the DMA control struct */
    DTENT *datp, *datep;	/* pointer to DAT table and individual entry */

    pd = &sc->pdma_ctrl[ targid ];	/* assign the pointer */
#ifdef PDMADEBUG
    PRINTD( targid, ENTRYEXIT,
        ("pmaz_ba_flush: enter, target= %d, dat_index=%d\n", 
        targid, pd->dat_addr ));
#endif

  /* Pull out the DAT table entry corresponding to this xfer */
    datp = (DTENT *)pd->dat_addr;
    datep = (DTENT *)&(datp[pd->dat_index]);
    if ( datep->dir == IOASIC_READ )
        {
        (*sc->dma_rbcopy)			/* move the data */
            ( datep->addr, datep->uadr, datep->length );
        }

    return( PDMA_SUCCESS );		/* all done */ 
    }

/* ---------------------------------------------------------------------- */
/* Support routines. */

/* ---------------------------------------------------------------------- */

/* Dumps the contents of the PDMA structure to the console. */

int
dump_pdma( pd )
    PDMA *pd;
{
    int i;

    printf( "Dumping pd: 0x%x\n", pd );
    printf( "    pstate 0x%x  ", pd->pstate );
    printf( "    pflags 0x%x  ", pd->pflags );
    printf( "    iodir 0x%x\n", pd->iodir );
    printf( "    dat_addr    : 0x%x\n", pd->dat_addr );
    printf( "    dat_index   : 0x%x\n", pd->dat_index );
    printf( "    data_addr   : 0x%x\n", pd->data_addr );
    printf( "    usercnt     : 0x%x\n", pd->usercnt );
    printf( "    targcnt     : 0x%x\n", pd->targcnt );
    printf( "    opcode      : 0x%x\n", pd->opcode );
    }

int frag_buf_load( pd )
PDMA *pd;
    {
    DTENT *datp, *datep;    /* pointer to DAT table and individual entry */
    char *sadr, *dadr;

  /* Pull out the DAT table entry corresponding to this xfer */
    datp = (DTENT *)pd->dat_addr;
    datep = (DTENT *)&(datp[pd->dat_index]);
    bcopy( datep->uadr, datep->addr, datep->length );
    return PDMA_SUCCESS;
    }

flushfifo( sc )
struct sz_softc *sc;
    {
    ASC_REG *ascaddr;

    ascaddr = (ASCREG *)sc->sc_scsiaddr;	/* get the chip addr */
    ascaddr->asc_cmd = ASC_FLUSH;
    wbflush();				/* clear write buffer */
    }
 
ioasicint( sc, targid, cntlr )
struct sz_softc *sc;
int targid;
int cntlr;
    {
    unsigned **dmap;         /* pointer to IOASIC DMA Ptr. reg. */
    unsigned long *sir;
    unsigned long sirp, resetval;
    int retval;
    ASC_REG *ascaddr = ASC_REG_ADDR;
    DMA_AR *ioasicp;		/* pointer for the SCSI DMA engine register */
    caddr_t pa;
    struct tc_memerr_status status;

    ioasicp = (DMAAR *)sc->ioasicp;	/* engine address */

    sir = (unsigned long *) ( (unsigned) ioasicp + SIR_O );

    resetval = 0xffffffff;
    sirp = *sir;
    retval = 1;

    if ( sirp & SCSI_DBPL )             /* has the DMA buffer pointer */
        {                              /* been loaded?               */
        resetval &= ~SCSI_DBPL;
        }

    if ( sirp & SCSI_OERR )          /* has an overrun error occurred? */
        {
        resetval &= ~SCSI_OERR;
        }

    if ( sirp & SCSI_MERR )          /* memory read error? */
        {
        dmap = (unsigned **) ( ( (unsigned) ioasicp ) + IOA_S_DMAP_O );
        pa = (char *) backcvt( (void *)*dmap ); /* Calc. the user buf address */
        printf("scsi%d: dma memory read error \n", cntlr);
        status.pa = pa;
        status.va = 0;
        status.log = TC_LOG_MEMERR;
        status.blocksize = 4;
        tc_isolate_memerr(&status);
        resetval &= ~SCSI_MERR;
        }

#ifdef NOTNEEDED
    if ( sirp & SCSI_DRDY )
        {
        retval = 0;			
        printf("ioasicint: Unexpected 53C94 data ready interrupt.\n" );
        resetval &= ~SCSI_DRDY;
        }
#endif

    if ( sirp & SCSI_C94 )
        {
        retval = 0;			/* go through '94 code */
        resetval &= ~SCSI_C94;
        }

    *sir = resetval;

    return retval;
    }

int is_local_xfer( p )
PDMA *p;			/* pointer for the DMA control struct */
    {
    int i;
    DTENT *t, *e;

    t = (DTENT *) p->dat_addr; /* grab the address of a table */
    i = p->dat_index;         /* and the index into the table */
    e = &t[i];

    if ( e->uadr )            /* if user buffer address is defined, */
        {                     /* then a local fragment buffer is being */
        return 1;             /* used. */
        }
    else                      /* Yes, this could be a ?: statement but I */
        {                     /* detest those */
        return 0;
        }
    }

dumptbl( tent )
DTENT *tent;
    {
    int k;
    int so = 0;

    printf( "Table dump\n" );
    for( k = 0; k < DATTBL_SIZ; k++ )
        {
        printf( " index:%d  ", k );
  
        if ( !dumpent( &(tent[k]) ) && so )
            k = DATTBL_SIZ;
        else
            so = 1;
        }
    }

dumpent( tent )
DTENT *tent;
    {
    printf( "len=%d addr=0x%x uadr=0x%x iadr=0x%x, ", 
    tent->length, tent->addr, tent->uadr, tent->iadr );
    if ( !tent->completed )
        printf( "not ");
    printf( "completed, " );
    if ( tent->dir == IOASIC_READ )
        printf( "read\n" );
    else if ( tent->dir == IOASIC_WRITE )
        printf( "write\n" );
    else 
        cprintf( "unknown\n" );
    return( tent->length );
    }
 
blddatent( tent, ecount, addr, uadr, dir )
DTENT *tent; 
unsigned long ecount; 
char *addr, *uadr;
int dir;
    {
    if ( !tent )
	return( PDMA_FAIL );
    tent->length = ecount;
    tent->addr = addr;
    tent->uadr = uadr;
    tent->completed = 0;
    if ( dir == SZ_DMA_READ )
        dir = IOASIC_READ;
    else if ( dir == SZ_DMA_WRITE )
        dir = IOASIC_WRITE;
    else 
        dir = IOASIC_UNDEF;
    tent->dir = dir;
    if ( addr )
        {
        tent->iadr = (unsigned *) ioa_addrcvt( addr );
        if ( tent->iadr == 0 )
            {
            dumpent( tent );
	    return( PDMA_FAIL );
            }
        }
    else
        tent->iadr = 0;
    return( PDMA_SUCCESS );
    }

/*  blddattbl - Builds a table of DMA buffers suitable for use with
		the 3min.  Users buffer address is broken up both at
		page boundaries and, when buffer areas are less than 8
		bytes in length, fixed buffer areas allocated by the
		driver are used instead.

    Inputs:	int controller - controller number
                DTENT *table -	A pointer to the table to be filled in.

		long count -	Length of the user's buffer.

		char *addr -	Pointer to the user's buffer.

    Return:	PDMA_FAIL on failure
*/
blddattbl( controller, pd, count, addr, dir ) 
int controller; 
PDMA *pd;			/* pointer for the DMA control struct */
unsigned long count; 
char *addr;
int dir;
    {
    DTENT *table; 
    char *frag;
    int index = 0;
    unsigned ecount;	
    int rem;
    char *eaddr, *uadr;

    table = (DTENT *) pd->dat_addr;
    frag = pd->frag_bufp;

    /* build null 0th entry */
    if ( blddatent( &table[index++], NULL, NULL, NULL, NULL ) == PDMA_FAIL )
        return( PDMA_FAIL );

    while( count )
        {
        if ( !(ecount = caldatent( addr, count )) )
            {
            printf( "blddattbl: Illegal return value (0) from caldatent().\n" );
            return( PDMA_FAIL );
            }
        if ( ecount < 8 )
            {
            eaddr = frag;
            uadr = addr;
            }
        else 
            {
            eaddr = addr;
            uadr = 0;
            }
	if ( blddatent( &(table[index++]), ecount, eaddr, uadr, dir ) ) 
	    {
            printstate |= PANICPRINT;
	    printf("eaddr=0x%x ecount=%d\n ", eaddr, ecount);
	    dumptbl( table );
            panic("blddatbl:  Invalid IOASIC physical address .\n");
	    }
        count -= ecount;
        addr += ecount;
        }
    if ( blddatent( &table[index++], NULL, NULL, NULL, NULL ) == PDMA_FAIL )
        return( PDMA_FAIL );
    return( PDMA_SUCCESS );
    }

/* rps - original 4K page params - should use other kernel defines */
#define STRPW  0xfffffff8
#define PAGS   0x00001000
#define SEGMSK 0x00000fff
#define	FRAGSZ 0x00000008
#define FRAGM  0x00000007

int caldatent( addr, count )
char *addr;
long count;
    {
    unsigned long rem, rem2;
    unsigned long ecount;

    rem = ((unsigned long)addr) & FRAGM;
    rem2 = ((unsigned long)addr) & SEGMSK;
    if ( count < FRAGSZ )
        ecount = count;
    else if ( rem ) /* not octabyte aligned? */
        ecount = FRAGSZ - rem;
    else if ( rem2 )	/* not page aligned? */
        {
        ecount = PAGS - rem2;
        if ( count < ecount )	
            ecount = ( ( ((unsigned long)addr) + count ) & STRPW ) - ((unsigned long)addr);
        }
    else				/* a 4k page */
        {
        ecount = PAGS;
        if ( count < ecount )
            ecount = ( ( ((unsigned long)addr) + count ) & STRPW ) - ((unsigned long)addr);
        }
    return( ecount );
    }

flush_fragbuf( pd, lcount )
PDMA *pd;
unsigned long lcount;
    {
    DTENT *datp, *datep;    /* pointer to DAT table and individual entry */
    char *tp;

    if ( lcount > 7 )
        printf("flush_fragbuf: pd=0x%x, lcount=%d\n", pd, lcount );

  /* First pull out the DAT table entry corresponding to this xfer */
    datp = (DTENT *)pd->dat_addr;
    datep = (DTENT *)&(datp[pd->dat_index]);

    if( lcount > 0 )	/* don't bother if no data */
        {
        bcopy( datep->addr, datep->uadr, lcount );
        pd->targcnt += lcount;	/* update target count */
        datep->length -= lcount;	/* adjust DAT entry length */  
        datep->uadr += lcount;
        if ( datep->length == 0 )
            pd->dat_index++;
        wbflush();
        }
    }

dmapload( sc, targid, addr )
struct sz_softc *sc;
int targid;
unsigned int *addr;
    {
    unsigned **dmap;         /* pointer to IOASIC DMA Ptr. reg. */
    DMA_AR *ioasicp;		/* pointer for the SCSI DMA engine register */
    
    PDMA *pd;
    DTENT *datp, *datep;    /* pointer to DAT table and individual entry */

    pd = &sc->pdma_ctrl[ targid ];		/* assign the pointer */
    datp = (DTENT *)pd->dat_addr;
    datep = (DTENT *)&(datp[pd->dat_index]);

    ioasicp = (DMAAR *)sc->ioasicp;	/* engine address */
    dmap = (unsigned **) ( ( (unsigned) ioasicp ) + IOA_S_DMAP_O );

    if ( addr == 0 )
        {
        printstate |= PANICPRINT;
        dumptbl( (DTENT *)pd->dat_addr );
        printf("dat_index = %d\n", pd->dat_index );
        dumpent( datep );
        printf( "dmapload:  Null IOASIC (SCSI) DMA address pointer.\n" );
	return PDMA_FAIL;
        }
    *dmap = addr;
    return PDMA_SUCCESS;
    }

ssrdmaon( sc, dir )
struct sz_softc *sc;
int dir;
    {
    unsigned *ssrp;          /* pointer to IOASIC SSR */
    DMA_AR *ioasicp;		/* pointer for the SCSI DMA engine register */

    ioasicp = (DMAAR *)sc->ioasicp;	/* engine address */

    ssrp = (unsigned *)((unsigned)ioasicp + SSR_O);

    if ( dir )		/* read */
	*ssrp |= ( SSR_DMADIR | SSR_DMAENB );
    else
        {
        *ssrp &= ( ~SSR_DMADIR );
        *ssrp |= SSR_DMAENB;
        }
    wbflush();
    }

ssrdmaoff( sc )
struct sz_softc *sc;
    {
    unsigned *ssrp;          /* pointer to IOASIC SSR */
    DMA_AR *ioasicp;		/* pointer for the SCSI DMA engine register */
    unsigned int ssr;

    ioasicp = (DMAAR *)sc->ioasicp;	/* engine address */

    ssrp = (unsigned *)((unsigned)ioasicp + SSR_O);
#ifdef PDMADEBUG
    if ( scsidebug & PASS2 && targtmp == targmatch )
        {
        ssr = *ssrp;
        if ( ssr & SSR_DMAENB )	
    	    printf("ssrdmaoff:  DMA enabled\n");
        else
            printf("ssrdmaoff:  DMA disabled\n");
        }
#endif
    *ssrp &= ( ~SSR_DMAENB );
    wbflush();
    }

dumphex( ptr, len )
char *ptr;
unsigned len;
    {
    int i,j,index;

    printf("\nDump of 0x%x, length %d\n\n", ptr, len );
    for( i=0; i<len; i+=16 )
        {
        printf("  %05x: ", i );
        for( j=0; j<16; j++ )
            {
            index = i+j;
            if (index >= len )
                break;
            printf( "%2x ", ptr[index] );
            }
        printf( "\n" );
        }
    }

/* ------------------------------------------------------------------------ */
/*	getdbuffer( sc, bufp )		Copies the IOASIC data buffers into */
/*					a user buffer.  User buffer must be */
/*					at least 8 bytes long!              */
/*	Inputs:                                                             */
/*	    sc				Pointer to soft_c structure         */
/*	    bufp			Pointer to user buffer.             */
/*                                                                          */
/*	Return value:			Number of valid data bytes or -1 on */
/*					error.                              */
/* ------------------------------------------------------------------------ */

int 
getdbuffer( sc, bufp )
struct sz_softc *sc;
void *bufp;
    {
    unsigned long *dbufp, *ubufp;
    unsigned int *cregp, creg;
    DMA_AR *ioasicp;		/* pointer for the SCSI DMA engine register */

    if ( !bufp )			/* check for invalid user address */
	return -1;

    ubufp = (unsigned long *)bufp;	/* address user buffer using 32's */

    ioasicp = (DMAAR *)sc->ioasicp;	/* engine address */

    dbufp = (unsigned long *)((unsigned)ioasicp + SCSI_DATA0_O );
    *ubufp++ = *dbufp;			/* Copy word 1 */

    dbufp = (unsigned long *)((unsigned)ioasicp + SCSI_DATA1_O );
    *ubufp = *dbufp;			/* Copy word 2 */

    cregp = (unsigned int *)((unsigned)ioasicp + SCSI_CTRL_O );
    creg = *cregp;			/* grab the scsi control register */

    if ( creg & CREG_DMA_M )		/* DMA operation in progress? */
	return 0;			/* nothing to read if a write */

    creg &= CREG_BUSG_M;		/* mask of to byte usage count */

#ifdef PDMADEBUG
if ( scsidebug & ENTRYEXIT && targtmp == targmatch )
    printf( "getdbuffer:  exit(%x)\n", creg<<1 );
#endif
    return creg<<1;			/* multiply hword's to get bytes */
    }

/* ------------------------------------------------------------------------ */
/*	getdbcount( sc, dir )		Returns the number of bytes filled  */
/*					in the IOASIC data buffers.         */
/*	Inputs:                                                             */
/*	    sc				Pointer to soft_c structure         */
/*	    dir				IO Direction (IOASIC_READ or _WRITE)*/
/*                                                                          */
/*	Return value:			Number of valid data bytes or -1 on */
/*					error.                              */
/* ------------------------------------------------------------------------ */

int 
getdbcount( sc, dir )
struct sz_softc *sc;
int dir;
    {
    unsigned int *cregp, creg;		/* Control pointer and content.    */
    DMA_AR *ioasicp;			/* SCSI DMA engine register        */
    int bcnt;				

    ioasicp = (DMAAR *)sc->ioasicp;	/* Engine address                  */

    cregp = (unsigned int *)((unsigned)ioasicp + SCSI_CTRL_O );
    creg = *cregp;			/* Grab the scsi control register. */

    bcnt = creg & CREG_BUSG_M;		/* Mask off to byte usage count.   */

    if ( dir == IOASIC_WRITE )		/* During writes, the sense of the */
        {				/* DMA bit is inverted.            */
        if ( creg & CREG_DMA_M )	/* Buffer not empty?               */
	    return (4-bcnt)<<1;		/* IOASIC counts in half-words (16)*/
	return 0;			/* bcnt is invalid                 */
        }				/* We've handled all WRITE cases.  */

    if ( creg & CREG_DMA_M )		/* DMA in progress?                */
        return 0;
#ifdef PDMADEBUG
if ( scsidebug & ENTRYEXIT && targtmp == targmatch )
    printf( "getdbcount:  exit(%x)\n", bcnt<<1 );
#endif
    return bcnt<<1;			/* Multiply hword's to get bytes.  */
    }

void
setscsictrl( sc, val )
struct sz_softc *sc;
int val;
    {
    unsigned int *cregp, creg;		/* Control pointer and content.    */
    DMA_AR *ioasicp;			/* SCSI DMA engine register        */
    int bcnt;				

    ssrdmaoff( sc );			/* JIC, turn DMA OFF */
    ioasicp = (DMAAR *)sc->ioasicp;	/* Engine address                  */

    cregp = (unsigned int *)((unsigned)ioasicp + SCSI_CTRL_O );
    *cregp = val;
    wbflush();
    }

int 
flushdb( sc, cnt )
struct sz_softc *sc;
int cnt;
    {
    unsigned int *cregp, creg;		/* Control pointer and content.    */
    DMA_AR *ioasicp;			/* SCSI DMA engine register        */
    unsigned int *dmap;	                /* pointer to IOASIC DMA Ptr. reg. */
    char *padr;                         /* physical address of user buffer */
    char lbuf[32];			/* Local data buffer               */
    int bcnt;				

    ioasicp = (DMAAR *)sc->ioasicp;	/* Engine address                  */

    dmap = (unsigned int *) ( (unsigned) ioasicp + IOA_S_DMAP_O );

    padr = (char *) backcvt( (void *)*dmap ); /* Calc. the user buf address */

    bcnt = getdbuffer( sc, lbuf );	/* grab data bytes */
    if ( cnt != bcnt )
        {
        printf("flushdb: input count (%d) doesn't match buffer count (%d)\n",
            cnt, bcnt );
        }
    bcopy( lbuf, padr, cnt );
    return cnt;
    }
