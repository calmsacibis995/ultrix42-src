/*
 *	@(#)gvp.h	4.2	(ULTRIX)	10/16/90
 */

/************************************************************************
 *                                                                      *
 *                      Copyright (c) 1988 by                           *
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
 *		constants, macros, and data structure definitions.
 *
 *   Creator:	Todd M. Katz	Creation Date:	November 20, 1985
 *
 *   Modification History:
 *
 *   16-Oct-1990	Pete Keilty
 *	Changed smp lock lk_cidevice. It is no longer temporary,
 *	added new macro in ciscs.h Lock_cidevice, Unlock_cidevice.
 *
 *   06-Feb-1990	Pete Keilty
 *	Added temporary smp lock lk_cidevice for CIXCD XMOV bug.
 *
 *   06-Feb-1990	Pete Keilty
 *	Remove Get_pgrp macro from gvp.h and move to ciport.h.
 *
 *   08-Dec-1989	Pete Keilty
 *	1. Added new macro Get_pgrp() to get port number until full 
 *	   subnode addressing is done.
 *	2. Zero reserved field in ci port header.
 *
 *   19-Sep-1989	Pete Keilty
 *	Added new explicit command address format to GVP.
 *	Support for XCD, DASH, SHAC & Subnode addressing.
 *
 *   06-Apr-1989	Pete Keilty
 *	Change gvpbddb lock to new lock_t struct lk_gvpbddb.
 *	Added new smp lock routines.
 *
 *   15-Aug-1988	Todd M. Katz
 *	1. Eliminate the masks for building specific condition codes.
 *	2. GVP events codes no longer occupy their own name space.  They now
 *	   occupy part of and defined within each ( appropriate ) port
 *	   drivers' name spaces.  Delete all GVP local port crash codes.
 *	3. Modify the macro Remqhi_rspq() to use a PCCB cached error event code
 *	   when crashing the local port because of failure to obtain the port
 *	   response queue interlock.
 *	4. Add shorthand notation Rspq_remerr.
 *
 *   08-Jan-1988	Todd M. Katz
 *	Formated module, revised comments, increased robustness, made GVP
 *	completely independent from underlying port drivers, restructured code
 *	paths, and added SMP support.
 */

/* Generic Vaxport Constants.
 */

					/* GVP Panic Strings		     */
#define	GVPPANIC_BNAME	"gvp\t- illegal buffer name\n"

/* Generic Vaxport Data Structure Definitions.
 */

/* NOTE: Changes to this structure must be reflected within ../msi/msiport.h.
 */
typedef	struct	_gvpbd	{		/* Generic Vaxport Buffer Descriptor */
    u_short	  boff	: 9;		/* Offset into page of buffer        */
    u_short		: 6;		/* MBZ	 			     */
    u_short	  valid	: 1;		/* Buffer Descriptor valid flag      */
#define	GVPBD_VALID	0x8000
    u_short	  key;			/* Key ( sequence number )	     */
    u_long	  bsize;		/* Size of buffer in bytes	     */
    struct pte	  *bpte;		/* Virtual address of buffer PTE     */
    struct _gvpbd *next_free_bd;	/* Next free buffer descriptor	     */
} GVPBD;

/* NOTE: Changes to this structure must be reflected within ../msi/msiport.h.
 */
					/* Generic Vaxport Buffer Descriptor */
typedef	struct	_gvpbddb {		/*  Database Header		     */
    GVPBD	*free_bd;		/* Free GVP BD list head	     */
    GVPBD	*bdt;			/* Buffer Descriptor table pointer   */
    u_short	size;			/* Size of data structure	     */
    u_char	type;			/* Structure type		     */
    u_char		:  8;
    u_long		: 32;		/* Align on octaword boundary MUSTBE */
} GVPBDDB;

struct lock_t lk_gvpbddb;		/* GVP BD database lock structure    */

typedef struct _gvph	 {		/* Generic Vaxport Port Buffer Header*/
    struct _gvpbq *flink;		/* Port queue pointers		     */
    struct _gvpbq *blink;
    u_short	  size;			/* Size of command packet	     */
    u_char	  type;			/* Structure type		     */
    u_char	  opt;			/* Optional field 		     */
#define	GVPH_FREE	-1		/*  Free datagram/message buffer     */
    u_char	  port;			/* Destination port( CI only )	     */
    struct	{			/* Status			     */
	u_char	failure		:  1;	/*  Failure bit			     */
	u_char	subtype		:  4;	/*  Error subtype		     */
	u_char	type		:  3;	/*  Error type			     */
    } status;
    u_char	  opcode;		/* Port command operation code	     */
#define	SNDDG	         1		/*  Send datagram		     */
#define	DGSNT		 1		/*  Datagram sent		     */
#define	SNDMSG		 2		/*  Send message		     */
#define	MSGSNT		 2		/*  Message sent		     */
#define	DGREC		33		/*  Datagram received		     */
#define	MSGREC		34		/*  Message received		     */
    struct	{			/* Port command operation modifiers  */
	u_char	rsp		:  1;	/*  Response requested bit	     */
	u_char			:  7;
    } flags;
    u_short	dstport;		/* New Explicit Addr. Format Adapters*/ 
    u_short	srcport;		/* New Explicit Addr. Format Adapters*/ 
    u_long	reserv;			/* Reserved for test		     */
} GVPH;

#define Gvph_size_imp	16
#define Gvph_size_exp	24

					/* Generic Vaxport Port-to-Port      */
typedef	struct _gvpppdh {		/*  Driver Buffer Header	     */
    u_short	length;			/* Length of message/datagram	     */
    u_short	mtype;			/* PPD message type		     */
} GVPPPDH;

/* Generic Vaxport Macros.
 */
					/* GVP Header Positioning and
					 *  Manipulation Macros
				         */
#define	Appl_size( gvpppdbp )		((( GVPPPDH * )gvpppdbp )->length - 2 )
#define	Format_gvph( pccb, gvpbp, opc, port_addr, dispose ) {		\
    (( GVPH * )gvpbp )->opt = 0;					\
    U_long((( GVPH * )gvpbp )->port ) = dispose;			\
    (( GVPH * )gvpbp )->opcode = opc;					\
    if( pccb->lpinfo.flags.expl ) {					\
         (( GVPH * )gvpbp )->port = 0;					\
         (( GVPH * )gvpbp )->dstport = port_addr << 8;			\
         (( GVPH * )gvpbp )->srcport = 0;				\
         (( GVPH * )gvpbp )->reserv = 0;				\
    } else {								\
         (( GVPH * )gvpbp )->port = port_addr;				\
    }									\
}
#define	Format_gvpppdh( gvpppdbp, type, size ) {			\
    U_long((( GVPPPDH * )gvpppdbp)->length ) = (( type << 16 ) | ( size + 2));\
}
#define	Pd_to_ppd( gvpbp, pccb )					\
    (( GVPPPDH * )(( u_char * )gvpbp + pccb->lpinfo.Ovhd_pd ))
#define	Pd_to_scs( gvpbp, pccb )					\
    (( SCSH * )(( u_char * )gvpbp + pccb->lpinfo.Ovhd ))
#define	Ppd_to_pd( gvpppdbp, pccb )					\
    (( GVPH * )(( u_char * )gvpppdbp - pccb->lpinfo.Ovhd_pd ))
#define	Ppd_to_scs( gvpppdbp )	(( SCSH * )(( GVPPPDH * )gvpppdbp + 1 ))
#define	Scs_to_ppd( scsbp )	(( GVPPPDH * )scsbp - 1 )
#define	Scs_to_pd( scsbp, pccb ) 					\
    (( GVPH * )(( u_char * )scsbp - pccb->lpinfo.Ovhd ))

					/* Queuing Macros
					 */
/* SMP: PCCB must be either locked or lockable for all queuing macros.
 */
#define	Insqti_block( gvpbp, pccb ) {					\
    Insqti( pccb, gvpbp, &pccb->Pblockq )				\
}
#define	Insqti_communication( gvpbp, pccb ) {				\
    Insqti( pccb, gvpbp, &pccb->Pcommq )				\
}
#define	Insqti_control( gvpbp, pccb ) {					\
    Insqti( pccb, gvpbp, &pccb->Pcontrolq )				\
}
#define	Insqti_dfreeq( gvpbp, pccb ) {					\
    Insqti( pccb, gvpbp, &pccb->Pdfreeq )				\
}
#define	Insqti_maintenance( gvpbp, pccb ) {				\
    Insqti( pccb, gvpbp, &pccb->Pmaintq )				\
}
#define	Insqti_mfreeq( gvpbp, pccb ) {					\
    Insqti( pccb, gvpbp, &pccb->Pmfreeq )				\
}
#define	Insqti( pccb, gvpbp, pqi ) {					\
    register int	status;						\
    if(( status = insqti( gvpbp, ( pqi )->header, gvp_queue_retry )) >= 0 ) { \
	if( status == 0 ) {						\
	    if( pccb->Qtransition == NULL ) {				\
		Lock_cidevice( pccb ) 					\
		*(( pqi )->creg ) = ( pqi )->cmask;			\
		Unlock_cidevice( pccb )					\
	    } else {							\
		( void )( *pccb->Qtransition )( pccb, ( pqi )->cmask );	\
	    }								\
	} else {							\
	    ( void )( *pccb->Crash_lport )( pccb,			\
					    ( pqi )->error,		\
					    Pd_to_scs( gvpbp, pccb ));	\
	}								\
    }									\
}
#define	Remqhi_dfreeq( pccb, gvpbp ) {					\
    Remqhi( pccb, gvpbp, pccb->Pdfreeq.header, pccb->Dfreeq_remerr )	\
}
#define	Remqhi_mfreeq( pccb, gvpbp ) {					\
    Remqhi( pccb, gvpbp, pccb->Pmfreeq.header, pccb->Mfreeq_remerr )	\
}
#define	Remqhi_rspq( pccb, gvpbp ) {					\
    Remqhi( pccb, gvpbp, &pccb->Pqb.rspq, pccb->Rspq_remerr )		\
}
#define	Remqhi( pccb, gvpbp, q, error )	{				\
    if(( long )( gvpbp = ( GVPH * )remqhi( q, gvp_queue_retry )) >= 0 ) { \
	if( gvpbp ) {							\
	    ( void )( *pccb->Crash_lport )( pccb, error, NULL );	\
	    gvpbp = NULL;						\
	}								\
    }									\
}

/* SMP Locking Macros
 */
#define	Init_gvpbd_lock() {						\
    lockinit( &lk_gvpbddb, &lock_gvpbddb_d );				\
}
#define	Lock_gvpbd() {							\
    smp_lock( &lk_gvpbddb, LK_RETRY );					\
}
#define	Unlock_gvpbd() {						\
    smp_unlock( &lk_gvpbddb );						\
}

					/* Shorthand Notations
					 */
#define	Bname			pd.gvp.bname
#define	Boff			pd.gvp.boff
#define	Dfreeq_remerr		pd.gvp.dfreeq_remerr
#define	Dg_size			pd.gvp.dg_size
#define	Mfreeq_remerr		pd.gvp.mfreeq_remerr
#define	Msg_size		pd.gvp.msg_size
#define	Ovhd_pd			pd.gvp.ovhd_pd
#define	Ovhd			pd.gvp.ovhd
#define	Pblockq			pd.gvp.pblockq
#define	Pcommq			pd.gvp.pcommq
#define	Pcontrolq		pd.gvp.pcontrolq
#define	Pdfreeq			pd.gvp.pdfreeq
#define	Pmaintq			pd.gvp.pmaintq
#define	Pmfreeq			pd.gvp.pmfreeq
#define	Qtransition		pd.gvp.qtransition
#define	Rspq_remerr		pd.gvp.rspq_remerr
#define	Pqb			pd.gvp.pqb
