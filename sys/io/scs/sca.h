/*
 *	@(#)sca.h	4.1	(ULTRIX)	7/2/90
 */

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
 *
 *   Abstract:	This module contains generic data types, constants, data
 *		structure definitions, and macros common to all SCA
 *		components.
 *
 *   Creator:	Todd M. Katz	Creation Date:	March 23, 1985
 *
 *   Modification History:
 *
 *   06-Jun-1990	Pete Keilty
 * 	Modified Ctrl_from_name() to check for three character names.
 *
 *   08-Dec-1989	Pete Keilty
 *	Modified Ctrl_from_name() added check for '\0'.
 *
 *   26-Apr-1989	Kong
 *	Modified Ctrl_from_num() to so that regardless of the address
 *	of the quoted string given, the result is naturally aligned.
 *	Added macro Align_name().  In SCS we frequently store the first
 *	four bytes of a quoted string into data structures.  Some hardware
 *	platforms require natural alignment.
 *
 *   18-Apr-1989	Pete Keilty
 *	Remove tmp. smp lock define.
 *
 *   09-Dec-1988	Todd M. Katz		TMK0006
 *	1. Add msibq and siibq structure definitions.
 *	2. Modify Ctrl_from_num() to be able to construct names containing
 *	   three letter base local port names( eg. - msi9 ).
 *	3. Some hardware platforms require field alignments and access types
 *	   to match( ie- only longword aligned entities may be longword
 *	   accessed ).  Structure fields of type c_scaaddr present a potential
 *	   problem because they are 6 bytes long but are often treated as a
 *	   longword and a word.  Such fields have been longword aligned
 *	   wherever possible.  It is also essential to change how they are
 *	   accessed for those cases when such alignment changes are no longer
 *	   feasible( eg- error log buffer formats ).  Therefore, the following
 *	   changes have been made:
 *		1) The structure of type scaaddr has been changed to consist of
 *		   three words( instead of one longword and one word ).
 *		2) The macro Scaaddr_lol() has been renamed to Scaaddr_low().
 *		3) The macro Scaaddr_hos() has been renamed to Scaaddr_hi().
 *		4) The macro Scaaddr_mid() has been created.
 *		5) All  macros which manipulate structures of type c_scaaddr(
 *		   Comp_scaaddr(), Move_scaaddr(), Test_scaaddr(),
 *		   Zero_scaaddr()) have been changed to make only word
 *		   accesses.
 *
 *   15-Aug-1988	Todd M. Katz		TMK0005
 *	1. Refer to error codes as event codes.
 *	2. Completely redesign sca event code format including:
 *		1) Increasing the size of ESEVERITY by 1 bit.
 *		2) Redefining the sca event severity codes.
 *		3) Defining a new 2 bit field ESEVMOD( sca event
 *		   severity modifier codes ) and an initial set of
 *		   event modifiers.
 *		4) Changing the bit positions of ECLASS and ESUBCLASS.
 *		5) Deleting the GVP ESUBCLASS sca event subclass code.
 *		6) Reducing the size of ESUBCLASS by 1 bit
 *		7) Redefining the remaining sca event subclass codes.
 *		8. Define a new 1 bit field ECLALWAYS( event console
 *		   logging filter override ).
 *	3. Add all combinations of severity-class-subclass event code bit mask
 *	   definitions.
 *	4. Update existing event manipulation macros and add the following new
 *	   ones: Esevmod(), Mask_esevmod(), Set_lpc_event(), Set_pc_event(),
 *	   Test_always(), Test_scs_event(), Test_pd_event(), Test_ppd_event(),
 *	   Test_lpc_event(), Test_pc_event(), Test_spc_event().
 *	5. Moved definition of structures CLFTAB and CLSTAB from
 *	   ../vaxci/cippd.h to here.
 *
 *   16-May-1988	Todd M. Katz		TMK0004
 *	1. Redo macro Ctrl_from_num() to be able to construct names containing
 *	   two digit controller numbers( e.g. - uq12 ).
 *	2. Rename the macros Copy_name() -> Move_name() and Copy_data() ->
 *	   Move_data().
 *
 *   23-Mar-1988	Todd M. Katz		TMK0003
 *	Add macro Ctrl_from_num().
 *
 *   23-Mar-1988	Todd M. Katz		TMK0002
 *	Move definition of SCSPC to ../vaxscs/scs.h.
 *
 *   08-Jan-1988	Todd M. Katz		TMK0001
 *	Formated module, revised comments, increased robustness, and added
 *	SMP support.
 */

/* SCA Event Code Bit Definitions.
 *
 *  3 3 2 2 2 2 2 2 2 2 2 2 1 2 1 1 1 1 1 1 1 1 
 *  1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 * +-+-+-+---+-----+-----------------------------------------------+
 * |E|E| |   |  E  |                                               |
 * |C|S|E| E |  S  |                                               |
 * |L|U|C| S |  E  |                                               |
 * |A|B|L| E |  V  |                                               |
 * |L|C|A| V |  E  |                    ECODE                      |
 * |W|L|S| M |  R  |                                               |
 * |A|A|S| O |  I  |                                               |
 * |Y|S| | D |  T  |                                               |
 * |S|S| |   |  Y  |                                               |
 * +-+-+-+---+-----+-----------------------------------------------+
 *		
 *  Bits		     		Function
 * -----		-----------------------------------------
 *  0-23		ECODE     - Event Code Number
 * 24-26		ESEVERITY - Event Severity Codes
 *		            Informational	0x00
 *			    Warning		0x01
 *			    Remote Error	0x02
 *			    Error		0x03
 * 		            Severe Error	0x04
 * 		            Fatal Error		0x05
 *			    RSVD( Future Use )	0x06
 *			    RSVD( Future Use )	0x07
 * 27-28		ESEVMOD	  - Event Severity Modifier Codes
 *			    None		0x00
 *			    Path Crash		0x01
 *			    Local Port Crash	0x02
 *			    RSVD( Future Use )	0x03
 *    29		ECLASS	  - Event Class Code
 *			    SCS			0x00
 *			    PD			0x01
 *    30		ESUBCLASS - Event Subclass Code( PD dependent )
 *			    PD			0x00( MSI/CI/BVP/UQ )
 *			    PPD			0x01( MSI/CI )
 *    31		ECLALWAYS - Event Console Loggging Filter Override
 *		
 * Event codes( ECODE ) are densely assigned for each possible combination of
 * ESEVERITY, ECLASS, and ESUBCLASS.
 *
 *			Definition of Severity Conditions
 *			---------------------------------
 *
 * Informational:		Notifies of a fully successful event.
 *				Notifies of a purely informative event.
 *				Does NOT increment any error counters.
 *
 * Warning:			Warns of possible problems associated with an
 *				 otherwise successful event.
 *				Does NOT increment any error counters.
 *
 * Remote Error:		Notifies of the occurrence of a remote error.
 *				Does NOT increment any error counters.
 *
 * Error:			Notifies of the occurrence of a local
 *				 recoverable error associated with a specific
 *				 path or local port.
 *				May have the path crash severity modifier
 *				 applied.
 *				Increments the number of errors associated with
 *				 the appropriate local port.
 *
 * Severe Error:		Notifies of the occurrence of a severe( but
 *				 still recoverable ) local error associated
 *				 with a specific path or local port.
 *				May have the path or local port crash severity
 *				 modifier applied.
 *				Increments the number of errors associated with
 *				 the appropriate local port.
 *
 * Fatal Error:			Notifies of the occurrence of a fatal
 *				 non-recoverable error associated with a
 *				 specific local port.
 *				Increments the number of errors associated with
 *				 the local port.
 *				Always logged to the console.
 */
					/* Bit number of field positions     */
#define	ES_SHIFT		 24	/*  Event severity		     */
#define	ESM_SHIFT		 27	/*  Event severity modifiers	     */
#define	EC_SHIFT		 29	/*  Event class			     */
#define	ESC_SHIFT		 30	/*  Event subclass		     */
					/* Event Field Bit Masks and Codes   */
#define	ECODE		0x00FFFFFF	/* Event code bit mask		     */
#define	ESEVERITY	0x07000000	/* Event severity code bit mask	     */
#define	ES_I			0x00	/*  Informational event		     */
#define	ES_W			0x01	/*  Warning condition		     */
#define	ES_RE			0x02	/*  Remote error		     */
#define	ES_E			0x03	/*  Error			     */
#define	ES_SE			0x04	/*  Severe error		     */
#define	ES_FE			0x05	/*  Fatal error			     */
#define	ESEVMOD		0x18000000	/* Event severity mod code bit mask  */
#define	ESM_PC			0x01	/*  Path crash			     */
#define	ESM_LPC			0x02	/*  Local port crash		     */
#define	ECLASS		0x20000000	/* Event class code bit mask	     */
#define	EC_SCS			0x00	/*  SCS class event	  	     */
#define	EC_PD			0x01	/*  PD class event		     */
#define	ESUBCLASS	0x40000000	/* Event subclass code bit mask	     */
#define	ESC_PD			0x00	/*  Port driver event		     */
#define	ESC_PPD			0x01	/*  Port-to-port driver event	     */
#define	ECLALWAYS	0x80000000	/* Override console logging filters  */
#define	SCSI		0x00000000	/* SCS informational event mask	     */
#define	SCSW		0x01000000	/* SCS warning condition mask	     */
#define	SCSE		0x03000000	/* SCS error mask		     */
#define	SCSSE		0x04000000	/* SCS severe error mask	     */
#define	PDI		0x20000000	/* PD informational event mask	     */
#define	PDW		0x21000000	/* PD warning condition mask	     */
#define	PDAW		0xA1000000	/* PD warning( always->console ) mask*/
#define	PDRE		0x22000000	/* PD remote error mask		     */
#define	PDE		0x23000000	/* PD error mask		     */
#define	PDSE		0x24000000	/* PD severe error mask		     */
#define	PDASE		0xA4000000	/* PD severe error mask		     */
#define	PDFE		0xA5000000	/* PD fatal error mask		     */
#define	PPDI		0x60000000	/* PPD informational event mask	     */
#define	PPDW		0x61000000	/* PPD warning condition mask	     */
#define	PPDAW		0xE1000000	/* PPD warning( always->console) mask*/
#define	PPDRE		0x62000000	/* PPD remote error mask	     */
#define	PPDE		0x63000000	/* PPD error mask		     */
#define	PPDSE		0x64000000	/* PPD severe error mask	     */

/* SCA Data Structure Field Definitions.
 */
typedef struct	_cbq	{		/* CB Queue Pointers		     */
    struct _cbq	*flink;			/* Forward queue link		     */
    struct _cbq	*blink;			/* Backward queue link		     */
} cbq;

typedef struct _msibq {			/* MSI Buffer Queue Pointers	     */
    struct _msibq *flink;		/* Forward queue link		     */
    struct _msibq *blink;		/* Backward queue link		     */
} msibq;

typedef struct	_pbq	{		/* PB Queue Pointers		     */
    struct _pbq *flink;			/* Forward queue link		     */
    struct _pbq *blink;			/* Backward queue link		     */
} pbq;

typedef struct	_pccbq	{		/* PCCB Queue Pointers		     */
    struct _pccbq *flink;		/* Forward queue link		     */
    struct _pccbq *blink;		/* Backward queue link		     */
} pccbq;

typedef struct	_sbq	{		/* SB Queue Pointers		     */
    struct _sbq *flink;			/* Forward queue link		     */
    struct _sbq *blink;			/* Backward queue link		     */
} sbq;

typedef struct _scaaddr	{		/* System Addresses		     */
    u_short	low;			/*  Low order word		     */
    u_short	mid;			/*  Middle order word		     */
    u_short	hi;			/*  High order word		     */
} scaaddr;

typedef struct _c_scaaddr	{	/* System Addresses( Compacted )     */
    u_char	val[ 6 ];
} c_scaaddr;

typedef struct _siibq {			/* SIIBUF Queue Pointers	     */
    struct _siibq *flink;		/* Forward queue link		     */
    struct _siibq *blink;		/* Backward queue link		     */
} siibq;

typedef struct	_u_dodec {		/* 12 Byte Field		     */
    u_long	val[ 3 ];
} u_dodec;

typedef struct	_u_quad {		/* 8 Byte Field			     */
    u_long	val[ 2 ];
} u_quad;

typedef	struct _uqbq	{		/* UQ buffer Queue Pointers 	     */
    struct _uqbq *flink;		/* Forward queue link 		     */
    struct _uqbq *blink;		/* Backward queue link		     */
} uqbq;

					/* Generic Vaxport Buffer Header     */
typedef	struct	_gvpbq	{		/*  Queue Pointers		     */
    struct _gvpbq *flink;		/* Forward queue link		     */
    struct _gvpbq *blink;		/* Backward queue link		     */
} gvpbq;

/* SCA Data Structure Definitions.
 */
					/* Console Logging Severity Level    */
typedef struct _clstab {		/*  Table Entry			     */
    u_long	   max_code;		/* Maximum code in formating table   */
    struct _clftab *ftable;		/* Address of formating table	     */
} CLSTAB;

					/* Console Logging Formating Table   */
typedef struct _clftab {		/*  Entry			     */
    u_long	fcode;			/* Format code			     */
    char	*msg;			/* Console logging message	     */
} CLFTAB;

/* SCA Macros.
 */
					/* Event Code Manipulation Macros
					 */
#define	Ecode( event )		( event & ECODE )
#define	Eseverity( event )	(( event & ESEVERITY ) >> ES_SHIFT )
#define	Esevmod( event )		(( event & ESEVMOD ) >> ESM_SHIFT )
#define	Eclass( event )		(( event & ECLASS ) >> EC_SHIFT )
#define	Esubclass( event )	(( event & ESUBCLASS ) >> ESC_SHIFT )
#define	Mask_esevmod( event )	( event & ~ESEVMOD )
#define	Set_lpc_event( event )	{					\
    event |= ( ESM_LPC << ESM_SHIFT );					\
}
#define	Set_pc_event( event )	{					\
    event |= ( ESM_PC << ESM_SHIFT );					\
}
#define	Test_cloverride( event )	( event & ECLALWAYS )
#define	Test_lpc_event( event )	( Esevmod( event ) == ESM_LPC )
#define	Test_scs_event( event )	( Eclass( event ) == EC_SCS )
#define	Test_pd_event( event )						\
    ( Eclass( event ) && Esubclass( event ) == ESC_PD )
#define	Test_ppd_event( event )	( Eclass( event ) && Esubclass( event ))
#define	Test_pc_event( event )	( Esevmod( event ) == ESM_PC )
#define	Test_spc_event( event )						\
    ( Test_pc_event( event ) && Eseverity( event ) == ES_SE )

					/* Field Manipulation Macros
					 */
#define	U_long( target )	*( u_long * )( &target )
#define	U_short( target )	*( u_short * )( &target )

					/* Queue Macros
					 */
#define	Init_queue( q ) {						\
    ( q ).flink = &( q );						\
    ( q ).blink = &( q );						\
}
#define	Remove_entry( entry ) {						\
    ( void )remque( &entry );						\
}
#define	Insert_entry( entry, queue ) {					\
    ( void )insque( &entry, ( queue ).blink );				\
}
					/* Structure Manipulation Macros
				         */
#define	Comp_name( name1, name2 )					\
    ( bcmp( name1, name2, NAME_SIZE ) == 0 )
#define	Comp_node( node1, node2 )					\
    ( U_long( node1[ 0 ]) == U_long( node2[ 0 ]) &&			\
      U_long( node1[ 4 ]) == U_long( node2[ 4 ]))
#define	Comp_quad( q1, q2 )						\
    ((( u_quad * )&q1 )->val[ 0 ] == (( u_quad * )&q2 )->val[ 0 ] &&	\
     (( u_quad * )&q1 )->val[ 1 ] == (( u_quad * )&q2 )->val[ 1 ])
#define	Comp_scaaddr( addr1, addr2 )					\
    ( Scaaddr_low( addr1 ) == Scaaddr_low( addr2 ) &&			\
      Scaaddr_mid( addr1 ) == Scaaddr_mid( addr2 ) &&			\
      Scaaddr_hi( addr1 ) == Scaaddr_hi( addr2 ))
#define	Move_bhandle( from, to ) {					\
    *( u_long * )&to = *( u_long * )&from;				\
    *(( u_long * )&to + 1 ) = *(( u_long * )&from + 1 );		\
    *(( u_long * )&to + 2 ) = *(( u_long * )&from + 2 );		\
}
#define	Move_connid( from, to )		U_long( to ) = U_long( from );
#define	Move_data( from, to )		( void )bcopy( from, to, DATA_SIZE );
#define	Move_name( from, to )		( void )bcopy( from, to, NAME_SIZE );
#define	Move_node( from, to ) {						\
    U_long( to[ 0 ]) = U_long( from[ 0 ]);				\
    U_long( to[ 4 ]) = U_long( from[ 4 ]);				\
}
#define	Move_quad( from, to ) {						\
    (( u_quad * )&to )->val[ 0 ] = (( u_quad * )&from )->val[ 0 ];	\
    (( u_quad * )&to )->val[ 1 ] = (( u_quad * )&from )->val[ 1 ];	\
}
#define	Move_scaaddr( from, to ) {					\
    Scaaddr_low( to ) = Scaaddr_low( from );				\
    Scaaddr_mid( to ) = Scaaddr_mid( from );				\
    Scaaddr_hi( to ) = Scaaddr_hi( from );				\
}
#define	Scaaddr_hi( addr )	(( scaaddr * )&addr )->hi
#define	Scaaddr_low( addr )	(( scaaddr * )&addr )->low
#define	Scaaddr_mid( addr )	(( scaaddr * )&addr )->mid
#define	Test_bhandle( handle ) 						\
    ( *( u_long * )&handle == 0		&&				\
      *(( u_long * )&handle + 1 ) == 0  &&				\
      *(( u_long * )&handle + 2 ) == 0 )
#define	Test_connid( target )	( target.index == 0 && target.seq_num == 0 )
#define	Test_scaaddr( addr )						\
    ((( scaaddr * )&addr )->low == 0 &&					\
     (( scaaddr * )&addr )->mid == 0 &&					\
     (( scaaddr * )&addr )->hi == 0 )
#define	Zero_bhandle( bh ) {						\
    U_long( bh ) = 0;							\
    *(( u_long * )&bh + 1 ) = 0;					\
    *(( u_long * )&bh + 2 ) = 0;					\
}
#define Zero_connid( target ) {						\
    target.index = 0;							\
    target.seq_num = 0;							\
}
#define	Zero_scaaddr( addr ) {						\
    Scaaddr_low( addr ) = 0;						\
    Scaaddr_mid( addr ) = 0;						\
    Scaaddr_hi( addr ) = 0;						\
}
					/* Miscellaneous Macros
					 */
#define	Ctrl_from_name( name )						\
    ((*(( u_char * )&name + 3 ) == ' ' || *(( u_char * )&name + 3 ) == '\0')\
	? (*(( u_char * )&name + 2 ) - '0' ) 				\
	: (((*(( u_char * )&name + 2 ) - '0' ) > 9 ) 			\
	    ? (*(( u_char * )&name + 3 ) - '0' )			\
	    : (((*(( u_char * )&name + 2 ) - '0' ) * 10 ) +		\
	        (*(( u_char * )&name + 3 ) - '0' ))))

#define	Ctrl_from_num( ns, n )					        \
	(u_long)(*(u_char *)ns) |					\
	(u_long)(*((u_char *)ns+1) << 8) |				\
	(u_long)(*((u_char *)ns+2) << 16) | 				\
		     (( u_long )((*(( u_char * )ns + 2 ) == ' ' )       \
				     ? (( n < 10 )			\
				    ? (( n + '0' ) << 16 )		\
				    : (((( n / 10 ) + '0' ) << 16 ) |	\
				       ((( n % 10 ) + '0' ) << 24 )))	\
				     : (( n + '0' ) << 24 )))

#define	Align_name(ns)					        	\
	((u_long)(*(u_char *)ns) |					\
	(u_long)(*((u_char *)ns+1) << 8) |				\
	(u_long)(*((u_char *)ns+2) << 16) | 				\
	(u_long)(*((u_char *)ns+3) << 24))

#define	Kfork( fbp, routine, parm ) {					\
    ( fbp )->arg = ( caddr_t )parm;					\
    ( fbp )->func = ( void (*)() )routine;				\
    ( void )ksched( fbp );						\
}

#define	Incr_seq_num( target )	{					\
    if( ++target.seq_num == 0 ) {					\
	++target.seq_num;						\
    }									\
}

/* IPL_SCS = 21, 0x15; splbio = 0x15; for mips block out device intr. */
#define  Splscs()	splbio()
