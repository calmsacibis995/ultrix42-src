/*
	@(#)bvpport.h	4.1  (ULTRIX)        7/2/90 
*/
/************************************************************************
 *									*
 *			Copyright (c) 1988 by				*
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
 ************************************************************************
 *
 * Revision History:
 *
 * 15-Aug-1988		Todd M. Katz
 *	1. All former BVP SSP local port crash codes are now defined as severe
 *	   error events.  The local port crash attribute is applied by
 *	   bvp_crash_lport() itself.
 *	2. GVP event codes no longer occupy their own name space.  They now
 *	   occupy part of and are defined with each ( appropriate ) port
 *	   drivers' name spaces.  Therefore, add the following former GVP
 *	   severe error event codes: ICMDQ0, ICMDQ1, IDFREEQ, IMFREEQ, RRSPQ,
 *	   RDFREEQ, RMFREEQ.
 *
 * 08-Jan-1988		Todd M. Katz
 *	1. Add the LPC_NOPATH( path does not exist ) local port crash code.
 *	2. Re-define all LPC codes according to the updated SCA error code
 *	   bit definitions( ../vaxscs/sca.h ).
 *
 * 17-Dec-1987		Todd M. Katz
 *	Fix Flushq macro by fully parenthesizing it.
 */
/**/
/* BVP Constants
 */

/*	Port Response failure types
 */
#define	T_OTHER		7		/* Other - see status error subtype */

/*	Port Response failure sub-types
 */

#define	ST_URCMD	3		/* Unrecognized port command	    */
#define	ST_UICMD	7		/* Unimplemented port command	    */
#define	ST_IVLP		8		/* Invalid FLAGS or STATUS field    */


					/* BVP SSP Severe Error Event Codes  */
					/*  All can have ESM_LPC applied     */
#define	SE_ICMDQ0     ( PDSE | 0x01 )	/* CMDQ0 interlocked on insertion    */
#define	SE_ICMDQ1     ( PDSE | 0x02 )	/* CMDQ1 interlocked on insertion    */
#define	SE_IDFREEQ    ( PDSE | 0x03 )	/* DFREEQ interlocked on insertion   */
#define	SE_IMFREEQ    ( PDSE | 0x04 )	/* MFREEQ interlocked on insertion   */
#define	SE_RRSPQ      ( PDSE | 0x05 )	/* RSPQ interlocked on removal	     */
#define	SE_RDFREEQ    ( PDSE | 0x06 )	/* DFREEQ interlocked on removal     */
#define	SE_RMFREEQ    ( PDSE | 0x07 )	/* MFREEQ interlocked on removal     */
#define	SE_NOPATH     ( PDSE | 0x08 )	/* Path does not exist		    */
#define	SE_UNKCMD     ( PDSE | 0x09 )	/* Unknown local port command	    */
#define	SE_UNKSTATUS  ( PDSE | 0x0A )	/* Unknown status in packet	    */
#define	SE_UNKOPCODE  ( PDSE | 0x0B )	/* Unknown opcode in packet	    */
#define	SE_INVOPCODE  ( PDSE | 0x0C )	/* Invalid opcode in packet	    */

						/* Shorthand notations */
#define	Lpinfo		pccb->lpinfo.pd.gvp.type.bvp
#define	Vpinfo		pccb->lpinfo.pd.gvp
#define	Pccb		pccb->pd.gvp.type.bvp
#define Bh		bhp->pd.gvp.bh
#define	Bvpqb		pccb->Pqb.type.bvp
#define Vpccb		pccb->pd.gvp
#define	Vpqb		pccb->Pqb


/*
 *	Status word in uba_ctlr structure
 *	Used for adapter locking
 *	NOTE:
 *		This word should only be accessed at IPL 17
 */

#define	um_status	um_cmd



/*	BVP Command queueing
 */

#define	MFREEQ		0
#define	DFREEQ		1
#define	CMDQ0		2
#define	CMDQ1		3
#define	CMDQ2		4
#define	CMDQ3		5

#define	M_freeq		1<<MFREEQ
#define	D_freeq		1<<DFREEQ
#define	C_cmdq_0	1<<CMDQ0
#define	C_cmdq_1	1<<CMDQ1
#define	C_cmdq_2	1<<CMDQ2
#define	C_cmdq_3	1<<CMDQ3

/*
 *	Queue numbers for port commands
 */

#define	BVPQ0		0
#define	BVPQ1		0x100
#define	BVPQ2		0x200
#define	BVPQ3		0x300

/*	Miscellaneous
 */
#define	Q_LOCKED	0x00000001	/* Queue interlock bit		    */
#define BVP_ADAP_LOCK	0x00000001	/* Adapter lock bit		    */

/*	Macros
 */
					/* Macros for port crashing	     */
#define	Flushq( pccb, q ) {						\
	if ( q->flink != ( gvpbq * )NULL ) {				\
	    register GVPH	*bvpbp;					\
	    do	{							\
		bvpbp = ( GVPH * )remqhi( q, gvp_queue_retry );		\
		if (( long )bvpbp < 0 ) {				\
		    KM_FREE( bvpbp, KM_SCABUF );			\
		} else if ( bvpbp != ( GVPH * )NULL ) {			\
	 	    q->flink = ( gvpbq * )NULL;				\
	 	    q->blink = ( gvpbq * )NULL;				\
		    bvpbp = ( GVPH * )NULL;				\
		    }							\
	    } while( bvpbp != ( GVPH * )NULL );				\
	    }								\
	}

		/* Wait for ownership bit to clear */
#define Wait_own( bvrg ) {						\
	int	t;							\
	t = 0;								\
	while ( t < DELAYONE ) { /* Wait for at most 1 second	*/	\
		if ( (bvrg->bvp_pc & BVP_PC_OWN) == 0) 			\
			break;						\
		DELAY(200000)						\
		t++;							\
	}								\
	if ( t == DELAYONE ) {						\
		cprintf("Wait_own: Wait_for_own failed\n");		\
		return( 0 );  /* Wait failed */				\
	}								\
}


		/* Wait for ownership bit to clear - no return value */
#define Wait_own_nr( bvrg ) {						\
	int	t;							\
	t = 0;								\
	while ( t < DELAYONE ) { /* Wait for at most 1 second	*/	\
		if ( (bvrg->bvp_pc & BVP_PC_OWN) == 0) 			\
			break;						\
		DELAY(200000)						\
		t++;							\
	}								\
	if ( t == DELAYONE ) {						\
		cprintf("Wait_own: Wait_for_own failed\n");		\
		return;  /* Wait failed */				\
	}								\
}
