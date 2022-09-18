#ifndef _PDMA_
#define _PDMA_

/*
 * @(#)pdma.h	4.2	(ULTRIX)	11/15/90
 */

/************************************************************************
 *									*
 *			Copyright (c) 1989 by				*
 *		Digital Equipment Corporation, Maynard, MA		*
 *			All rights reserved.				*
 *									*
 *   This software is furnished under a license and may be used and	*
 *   copied  only  in accordance with the terms of such license and	*
 *   with the  inclusion  of  the  above  copyright  notice.   This	*
 *   software  or  any  other copies thereof may not be provided or	*
 *   otherwise made available to any other person.  No title to and	*
 *   ownership of the software is hereby transferred.			*
 *                                                                      *
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

/************************************************************************
 * pdma.h	11/07/89
 *
 * 10/11/89	John A. Gallant
 * Created this file for the pseudo DMA support code in the SCSI drivers.
 *
 * 10/17/89	John A. Gallant
 * Added a second counter to the PDMA control structure, now have one counter
 * for bytes xfered for the target and one for bytes xfered for user space.
 *
 * 10/25/89	John A. Gallant
 * Added PIPE_PREFLUSH flag define.  Added the XFER_BUFCNT() macro.
 * Added the pdma_flush() entries into the function structure.
 *
 * 11/07/89	John A. Gallant
 * Added pstate in the PDMA structure for state information, pflags is used
 * for general operation flow control.
 *
 ************************************************************************/

/* ---------------------------------------------------------------------- */

/* Constants for the PDMA routines. */

#define MAX_XFER_SIZE		0x2000		/* current buffers are 8k */
#define PIPE_BUF_NUM		2		/* only 2 buffers per pipe */

/* Defines for the Success/Fail/Retry return values. */

#define	PDMA_SUCCESS		0x0000		/* all went well */
#define	PDMA_FAIL		0x0001		/* the attempt failed */
#define	PDMA_RETRY		0x0002		/* try again later */
#define	PDMA_ALLOC_FAIL		0x0004		/* no match for cuptype */

/* Defines for the bits in the PDMA state field, pstate. */

#define	PDMA_IDLE		0x1000		/* the PDMA code is idle */
#define	PDMA_SETUP		0x1001		/* the PDMA code is setup */
#define	PDMA_ACTIVE		0x1002		/* the PDMA code is active */

/* Defines for the bits in the PDMA flags field, pflags. */

#define PDMA_ODDBYTE		0x0001		/* odd byte RAM/data align */

/* Defines for the bits in the PDMA control flags field, pdma_pflags. */

#define PDMA_16BIT		0x0001		/* RAM buffer: 16/32 boundry */
#define PDMA_DMAENGINE		0x0002		/* a seperate DMA engine */
#define PDMA_PREFLUSH		0x0004		/* preflush bufs before use */

/* Defines for the bits in the PDMA buffer flags field, pb_flags. */

#define	PB_EMPTY		0x2000		/* the buffer is empty */
#define	PB_READY		0x2001		/* buffer is ready for data */
#define	PB_LOADED		0x2002		/* buffer is loaded w/data */
#define	PB_DMA_ACTIVE		0x2004		/* buffer <-> SCSI inuse */
#define	PB_CPU_ACTIVE		0x2008		/* buffer <-> CPU inuse */
#define PB_FINISHED		0x2010		/* buffer all done */

/* ---------------------------------------------------------------------- */

/* Data structures: for the PDMA control data. */

typedef struct 			/* PDMA control struct per target */
{
    int pstate;			/* state information for the PDMA operation */
    int pflags;			/* general purpose flags for DMA control */
    int iodir;			/* direction of xfer DMA_IN/DMA_OUT */
    long count;			/* total count for the xfer, decremented */
    char *data_addr;		/* address for the users data */
    long usercnt;		/* user space xfer count */
    long targcnt;		/* target data xfer count */
    int opcode;			/* stored opcode from the pdma_start() call */
    char *pb_addr;		/* address for the pipe */
    int pb_offset[PIPE_BUF_NUM];/* offsets for the pipe buffers */
    int pb_count[PIPE_BUF_NUM];	/* data count for each of the pipe buffers */
    int pb_flags[PIPE_BUF_NUM];	/* flags for each of the pipe buffers */
    char *dat_addr;       /* pointer to dat table */
    int dat_index;        /* index into dat table */
    char *frag_bufp;       /* pointer to fragment buffer */
} PDMA;

/* Structure for the entry points with in each of the files. */

typedef struct			/* PDMA routine entry points */
{
    int pdma_cputype;		/* what CPU these routines are for */
    int pdma_pflags;		/* general flags for these routines */
    int (*pdma_init)();		/* pointer to the init routine */
    int (*pdma_setup)();	/* pointer to the setup routine */
    int (*pdma_start)();	/* pointer to the start routine */
    int (*pdma_cont)();		/* pointer to the cont routine */
    int (*pdma_end)();		/* pointer to the end routine */
    int (*pdma_rbcopy)();	/* pointer to the DATA-IN transfer routine */
    int (*pdma_wbcopy)();	/* pointer to the DATA-OUT transfer routine */
    int (*pdma_bzero)();	/* pointer to the zero fill routine */
    int (*pdma_flush)();	/* pointer to the pipe flush routine */
} PDMA_ENTRY;

/* ---------------------------------------------------------------------- */

/* Macro used to load the RAM buffer offset address into the SII's two 
address registers.  The address low register is a full 16 bits.  The address
high register only has the lower 2 bits valid.  The complete address is
18 bits long. */

#define SII_LOAD_ADDR( siiaddr, offset )				\
{									\
    siiaddr->sii_dmaddrl = (offset & 0xFFFF);				\
    siiaddr->sii_dmaddrh = ((offset >> 16) & 0x0003);			\
}

/* Used to handle the addressing differences for the RAM buffer in the pmax.
The RAM buffer is different than in the VAX and 3max systems.  The RAM is 
not contigious on the CPU side.  Each 16 bit word in the RAM buffer is 
aligned on a 32 bit boundry.  So there is a 16 bit hole after each 16 bit
word. This macro handles the shifting of the offset value by 1 to take into
account the holes.  Using this macro the only time that the offsets have to
be shifted is on CPU accesses. */

#define RAM_ADJUST( sc, offset )					\
    ((sc->dma_pflags & PDMA_16BIT) ? (offset * 2) :(offset) )

/* A macro for determing what the local count should be for the particular
buffers.  The argument is compared to MAX_XFER_SIZE.  If the argument count
is greater than MAX, MAX is returned, else the argument count is returned.
This will be used to determine how much data will be xfered for the pipe 
buffer. */

#define XFER_BUFCNT( X )						\
    (( (X) > MAX_XFER_SIZE ) ? ( MAX_XFER_SIZE ) : ((X)) )

/* These two macros are used to load the address of the RAM buffer and the
DMA engine into the sz_softc structure.  The driver probe code is responsible
for loading the base address for the SCSI hardware.  This way the probe code
does not have to "know" about how the data is moved. */
/* NOTE: TUNED TO PMAX  will want make specific for pmax/3max/ff/pvax */

/* These Macros have to be used AFTER !! the softc structure initialization
in the particular routines.  They use the sc pointer. */

/* RAM buffer and address register for the DS3100. */
#define DS3100_BUF_BASE( sc )	(char *)((int)sc->sc_scsiaddr + 0x1000000)
#define DS3100_AR_BASE( sc )	(char *)((int)sc->sc_scsiaddr + 0x0000000)

/* RAM buffer and address register for the DS5000. */
#define DS5000_BUF_BASE( sc )	(char *)((int)(sc->sc_scsiaddr + 0x80000))
#define DS5000_AR_BASE( sc )	(char *)((int)(sc->sc_scsiaddr + 0x40000))


/* ---------------------------------------------------------------------- */

#endif _PDMA_
